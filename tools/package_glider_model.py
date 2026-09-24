#!/usr/bin/env python3
"""Package a TP BMD3 as a platform-independent Dawnlight glider .dusk."""
import argparse
import json
from pathlib import Path
import re
import struct
import zipfile

DISC_PATH = 'res/Object/DawnlightGlider.bmd'
DEFAULT_MOD_ID = 'dev.bezide.dawnlight_custom_glider'
MAX_BYTES = 8 * 1024 * 1024
SECTIONS = {'INF1': 24, 'VTX1': 64, 'EVP1': 28, 'DRW1': 20,
            'JNT1': 24, 'SHP1': 44, 'MAT3': 132, 'TEX1': 20}


def validate(data):
    """Container preflight only; the BMD must still be a valid exporter output."""
    if not 32 <= len(data) <= MAX_BYTES or data[:8] != b'J3D2bmd3':
        raise ValueError('Expected an uncompressed TP BMD3, at most 8 MiB')
    size, count = struct.unpack_from('>II', data, 8)
    if size != len(data) or count != len(SECTIONS):
        raise ValueError('Invalid BMD file size or section count')
    offset, seen = 32, set()
    for _ in range(count):
        if offset + 8 > size:
            raise ValueError('Truncated BMD section header')
        tag = data[offset:offset + 4].decode('ascii', errors='replace')
        length = struct.unpack_from('>I', data, offset + 4)[0]
        if (tag not in SECTIONS or tag in seen or length < SECTIONS[tag] or
                length % 4 or offset + length > size):
            raise ValueError(f'Invalid BMD section: {tag!r}')
        if tag in ('JNT1', 'SHP1', 'MAT3') and data[offset + 8:offset + 10] == b'\0\0':
            raise ValueError(f'Empty BMD section: {tag}')
        seen.add(tag)
        offset += length
    if offset != size:
        raise ValueError('Unexpected data after BMD sections')


def package(source, output, mod_id=DEFAULT_MOD_ID, name='Dawnlight Glider Model'):
    # Match Dusklight's is_valid_mod_id: nonempty dot-separated segments.
    if re.fullmatch(r'[a-z0-9_]+(?:\.[a-z0-9_]+)*', mod_id) is None:
        raise ValueError('Invalid mod ID: use lowercase letters, digits, or underscores '
                         'in nonempty dot-separated segments (no hyphens)')
    if source.resolve() == output.resolve():
        raise ValueError('The output must not overwrite the input BMD')
    data = source.read_bytes()
    validate(data)
    metadata = {'id': mod_id, 'name': name, 'version': '1.0.0',
                'description': 'Custom BMD glider for Dawnlight. Enable alongside Dawnlight and restart the game.'}
    # Stable zip metadata makes repeated packaging byte-identical.
    entries = {'mod.json': (json.dumps(metadata, indent=2) + '\n').encode(),
               'overlay/' + DISC_PATH: data}
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
        for path, content in entries.items():
            info = zipfile.ZipInfo(path, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            bundle.writestr(info, content)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--id', default=DEFAULT_MOD_ID)
    parser.add_argument('--name', default='Dawnlight Glider Model')
    args = parser.parse_args()
    if args.output.suffix != '.dusk':
        parser.error('Output must have a .dusk extension')
    try:
        print(package(args.model, args.output, args.id, args.name))
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
