"""Find musically repeating and click-resistant loop points in 16-bit PCM WAV files."""

import argparse
import math
import wave

import numpy as np


def read_wav(path: str):
    with wave.open(path, "rb") as wav:
        channels = wav.getnchannels()
        sample_rate = wav.getframerate()
        sample_width = wav.getsampwidth()
        sample_count = wav.getnframes()
        if sample_width != 2:
            raise ValueError(f"Expected 16-bit PCM, got {sample_width * 8}-bit")
        audio = np.frombuffer(wav.readframes(sample_count), dtype="<i2")
    audio = audio.reshape(-1, channels).astype(np.float32) / 32768.0
    return audio, sample_rate


def spectral_features(audio: np.ndarray, sample_rate: int, hop_seconds: float = 0.1):
    mono = audio.mean(axis=1)
    window_size = 4096
    hop = round(sample_rate * hop_seconds)
    starts = np.arange(0, len(mono) - window_size + 1, hop)
    window = np.hanning(window_size).astype(np.float32)
    frequencies = np.fft.rfftfreq(window_size, 1.0 / sample_rate)
    edges = np.geomspace(45.0, min(12000.0, sample_rate / 2 - 1), 49)
    bins = [
        np.flatnonzero((frequencies >= edges[index]) & (frequencies < edges[index + 1]))
        for index in range(len(edges) - 1)
    ]
    features = np.empty((len(starts), len(bins)), dtype=np.float32)

    for index, start in enumerate(starts):
        frame = mono[start:start + window_size] * window
        spectrum = np.abs(np.fft.rfft(frame)) ** 2
        bands = np.array(
            [math.log1p(float(spectrum[group].sum())) if len(group) else 0.0 for group in bins],
            dtype=np.float32,
        )
        bands -= bands.mean()
        bands /= max(float(np.linalg.norm(bands)), 1e-8)
        features[index] = bands
    return features, hop


def rank_periods(features: np.ndarray, hop: int, sample_rate: int, duration: float):
    hop_seconds = hop / sample_rate
    margin = round(4.0 / hop_seconds)
    results = []
    maximum = min(190.0, duration - 8.0)

    for lag in range(round(20.0 / hop_seconds), round(maximum / hop_seconds) + 1):
        count = len(features) - lag - 2 * margin
        if count < 20:
            continue
        lhs = features[margin:margin + count]
        rhs = features[margin + lag:margin + lag + count]
        similarities = np.sum(lhs * rhs, axis=1)
        results.append((float(np.mean(similarities)), lag * hop_seconds))

    results.sort(reverse=True)
    selected = []
    for result in results:
        if all(abs(result[1] - previous[1]) > 0.8 for previous in selected):
            selected.append(result)
        if len(selected) == 8:
            break
    return selected


def window_similarity(mono: np.ndarray, start: int, end: int, context: int):
    lhs = mono[start - context:start + context:12]
    rhs = mono[end - context:end + context:12]
    lhs = lhs - lhs.mean()
    rhs = rhs - rhs.mean()
    denominator = float(np.linalg.norm(lhs) * np.linalg.norm(rhs))
    return float(np.dot(lhs, rhs) / denominator) if denominator > 1e-12 else -1.0


def scan_period(audio: np.ndarray, sample_rate: int, period_seconds: float):
    mono = audio.mean(axis=1)
    period = round(period_seconds * sample_rate)
    context = round(1.5 * sample_rate)
    step = round(0.01 * sample_rate)
    results = []

    for start in range(context, len(audio) - period - context, step):
        end = start + period
        correlation = window_similarity(mono, start, end, context)
        results.append((correlation, start, end))

    results.sort(reverse=True)
    selected = []
    for result in results:
        if all(abs(result[1] - previous[1]) > round(0.35 * sample_rate) for previous in selected):
            selected.append(result)
        if len(selected) == 8:
            break
    return selected


def refine_boundary(audio: np.ndarray, sample_rate: int, start: int, end: int):
    period = end - start
    radius = round(0.6 * sample_rate)
    candidates = []

    for refined_start in range(start - radius, start + radius + 1):
        refined_end = refined_start + period
        if refined_start < 3 or refined_end + 2 >= len(audio):
            continue
        value_jump = float(np.linalg.norm(audio[refined_end - 1] - audio[refined_start]))
        slope_before = audio[refined_end - 1] - audio[refined_end - 2]
        slope_after = audio[refined_start + 1] - audio[refined_start]
        slope_jump = float(np.linalg.norm(slope_before - slope_after))
        curvature_before = (
            audio[refined_end - 1] - 2 * audio[refined_end - 2] + audio[refined_end - 3]
        )
        curvature_after = (
            audio[refined_start + 2] - 2 * audio[refined_start + 1] + audio[refined_start]
        )
        curvature_jump = float(np.linalg.norm(curvature_before - curvature_after))
        score = value_jump + 0.35 * slope_jump + 0.08 * curvature_jump
        candidates.append(
            (score, refined_start, refined_end, value_jump, slope_jump, curvature_jump)
        )

    return min(candidates)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("wav", help="16-bit PCM WAV to analyze")
    parser.add_argument("--period", type=float, help="Analyze this loop period in seconds")
    args = parser.parse_args()

    audio, sample_rate = read_wav(args.wav)
    duration = len(audio) / sample_rate
    print(
        f"sample_rate={sample_rate} channels={audio.shape[1]} "
        f"samples={len(audio)} duration={duration:.6f}s"
    )

    features, hop = spectral_features(audio, sample_rate)
    periods = rank_periods(features, hop, sample_rate, duration)
    print("\nStrongest musical periods:")
    for similarity, seconds in periods:
        print(f"  {seconds:9.3f}s  similarity={similarity:.6f}")

    period = args.period if args.period is not None else periods[0][1]
    print(f"\nBoundary candidates for {period:.3f}s:")
    for correlation, start, end in scan_period(audio, sample_rate, period):
        print(
            f"  {start / sample_rate:12.6f}s -> {end / sample_rate:12.6f}s  "
            f"correlation={correlation:.6f}"
        )

    correlation, start, end = scan_period(audio, sample_rate, period)[0]
    score, start, end, jump, slope, curvature = refine_boundary(
        audio, sample_rate, start, end
    )
    print("\nRecommended sample-aligned boundary:")
    print(f"  start: {start} samples ({start / sample_rate:.9f}s)")
    print(f"  end:   {end} samples ({end / sample_rate:.9f}s)")
    print(f"  period: {(end - start) / sample_rate:.6f}s")
    print(
        f"  correlation={correlation:.6f} click_score={score:.9f} "
        f"value_jump={jump:.9f} slope_jump={slope:.9f} curvature_jump={curvature:.9f}"
    )


if __name__ == "__main__":
    main()
