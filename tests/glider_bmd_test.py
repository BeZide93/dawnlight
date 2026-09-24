"""Container rejection and exact/reproducible overlay packaging.
No game assets required. Synthetic containers below are never sent to real J3D.
"""
from pathlib import Path
import importlib.util
import json
import struct
import sys
sys.dont_write_bytecode = True
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('packer', ROOT / 'tools/package_glider_model.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)
sections = []
for tag, size in p.SECTIONS.items():
    b = bytearray(size)
    struct.pack_into('>4sIH', b, 0, tag.encode(), size, 1)
    sections.append(b)
valid = b'J3D2bmd3' + struct.pack('>II', 32 + sum(map(len, sections)), 8) + bytes(16) + b''.join(sections)
p.validate(valid)
invalid = [b'', valid[:31], valid[:-1], valid + b'padding', b'J3D2bdl4' + valid[8:]]
def changed(offset, data):
    return valid[:offset] + data + valid[offset + len(data):]
invalid += [changed(12, struct.pack('>I', 9)), changed(36, bytes(4)),
            changed(36, struct.pack('>I', 0xffffffff)), changed(32, b'JUNK'),
            changed(32 + len(sections[0]), b'INF1')]
joint_offset = 32 + sum(map(len, sections[:4]))
invalid.append(changed(joint_offset + 8, b'\0\0'))
for data in invalid:
    try:
        p.validate(data)
    except ValueError:
        continue
    raise AssertionError('Accepted malformed container')

with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    source = temp / 'model.bmd'
    source.write_bytes(valid)
    one, two = temp / 'one.dusk', temp / 'two.dusk'
    p.package(source, one)
    p.package(source, two)
    assert one.read_bytes() == two.read_bytes()
    with zipfile.ZipFile(one) as z:
        assert set(z.namelist()) == {'mod.json', 'overlay/' + p.DISC_PATH}
        assert z.read('overlay/' + p.DISC_PATH) == valid
        assert json.loads(z.read('mod.json'))['id'] == 'dev.bezide.dawnlight_custom_glider'
    # Exercise the CLI default too: it must produce a loader-compatible manifest.
    subprocess.run([sys.executable, str(ROOT / 'tools/package_glider_model.py'),
                    str(source), str(two)], check=True, capture_output=True)
    assert one.read_bytes() == two.read_bytes()
    for mod_id in ('', 'dev.bezide.glider-model', '.glider', 'glider.',
                   'dev..glider', 'Glider', 'glider\n', 'glidér'):
        try:
            p.package(source, two, mod_id=mod_id)
        except ValueError:
            pass
        else:
            raise AssertionError(f'Accepted invalid mod ID: {mod_id!r}')
        assert one.read_bytes() == two.read_bytes()  # Reject before overwriting.
    p.package(source, two, mod_id='custom_glider.v2')
    with zipfile.ZipFile(two) as z:
        assert json.loads(z.read('mod.json'))['id'] == 'custom_glider.v2'
    fixture = '''
#include "glider_bmd_format.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
int main(int argc,char** argv) {
 for(int i=1;i<argc;++i) {
  std::ifstream f(argv[i],std::ios::binary);
  std::vector<char> b((std::istreambuf_iterator<char>(f)),{});
  assert(dawnlight::valid_glider_bmd(b.data(),b.size())==(i==1));
 }
 assert(!dawnlight::valid_glider_bmd(nullptr,32));
}
'''
    cpp, exe = temp / 'test.cpp', temp / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'src'),
                    str(cpp), '-o', str(exe)], check=True)
    paths = [source]
    for i, data in enumerate(invalid):
        path = temp / f'invalid-{i}.bmd'
        path.write_bytes(data)
        paths.append(path)
    subprocess.run([str(exe), *map(str, paths)], check=True)
print('Glider BMD: C++ and packer reject invalid containers; exact, deterministic overlay packaging passed')
