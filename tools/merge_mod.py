#!/usr/bin/env python3
"""Merge per-platform .dusk bundles into one multi-platform bundle.

Each input bundle contains the same mod content plus its platform's native libraries under
lib/<platform>/. This merges the lib/ trees, verifies that every native library declares the
same mod ABI and service imports/exports (symgen modmeta --check), embeds that verified
metadata into the bundle's mod.json (--update-json), and writes a deterministic zip.

Usage: merge_mod.py -o combined.dusk [--symgen path] input.dusk...
"""

import argparse
import hashlib
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

MOD_LIB_NAMES = ("mod.so", "mod.dll")


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    sys.exit(1)


def entry_names(archive: zipfile.ZipFile) -> list[str]:
    names = [info.filename for info in archive.infolist() if not info.is_dir()]
    for name in names:
        if name.startswith("/") or ".." in name.split("/"):
            fail(f"unsafe entry name in {archive.filename}: {name}")
    return names


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("inputs", nargs="+", type=Path, help="per-platform .dusk bundles")
    parser.add_argument("-o", "--output", required=True, type=Path, help="combined .dusk to write")
    parser.add_argument("--symgen", default="symgen", help="path to the symgen executable")
    args = parser.parse_args()

    # Collect entries: non-lib content must be identical everywhere; lib/<platform>/ trees
    # must come from exactly one input each.
    content_hashes: dict[str, tuple[str, Path]] = {}
    platform_sources: dict[str, Path] = {}
    archives: list[zipfile.ZipFile] = []
    for path in args.inputs:
        if not path.is_file():
            fail(f"input does not exist: {path}")
        archive = zipfile.ZipFile(path)
        archives.append(archive)
        platforms = set()
        for name in entry_names(archive):
            if name.startswith("lib/"):
                parts = name.split("/")
                if len(parts) < 3 or not parts[1]:
                    fail(f"unexpected lib entry in {path}: {name}")
                platforms.add(parts[1])
                continue
            digest = hashlib.sha256(archive.read(name)).hexdigest()
            seen = content_hashes.get(name)
            if seen is None:
                content_hashes[name] = (digest, path)
            elif seen[0] != digest:
                fail(f"'{name}' differs between {seen[1]} and {path}; "
                     "bundles must be built from the same source")
        if not platforms:
            fail(f"{path} contains no native libraries")
        for platform in platforms:
            if platform in platform_sources:
                fail(f"platform '{platform}' provided by both "
                     f"{platform_sources[platform]} and {path}")
            platform_sources[platform] = path

    if "mod.json" not in content_hashes:
        fail("bundles contain no mod.json")

    with tempfile.TemporaryDirectory() as tmp:
        stage = Path(tmp)
        # Non-lib content from the first input (verified identical), lib/ from each source.
        archives[0].extractall(stage, members=[n for n in entry_names(archives[0])
                                               if not n.startswith("lib/")])
        for archive in archives:
            archive.extractall(stage, members=[n for n in entry_names(archive)
                                               if n.startswith("lib/")])

        mod_libs = []
        for platform in sorted(platform_sources):
            libs = [p for p in (stage / "lib" / platform / n for n in MOD_LIB_NAMES)
                    if p.is_file()]
            if len(libs) != 1:
                fail(f"expected exactly one mod library in lib/{platform}/")
            mod_libs.append(libs[0])

        command = [args.symgen, "modmeta", "--check", "--update-json", str(stage / "mod.json"),
                   *map(str, mod_libs)]
        result = subprocess.run(command)
        if result.returncode != 0:
            fail("symgen modmeta verification failed")

        args.output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED) as out:
            for file in sorted(p for p in stage.rglob("*") if p.is_file()):
                info = zipfile.ZipInfo(file.relative_to(stage).as_posix(),
                                       date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                out.writestr(info, file.read_bytes())

    print(f"wrote {args.output} with platforms: {', '.join(sorted(platform_sources))}")


if __name__ == "__main__":
    main()
