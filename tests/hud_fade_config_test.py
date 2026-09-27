"""Run the production config round-trip test with a configured Ninja SDK build."""
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'build').resolve()
ninja = (build / 'build.ninja').read_text()
block = ninja.split('build CMakeFiles/dawnlight_mod.dir/src/config.cpp.o:', 1)[1].split('\nbuild ', 1)[0]
args = ['c++', '-ffunction-sections', '-fdata-sections']
for key in ('DEFINES', 'FLAGS', 'INCLUDES'):
    line = next(x for x in block.splitlines() if x.startswith('  ' + key + ' = '))
    args += shlex.split(line.split(' = ', 1)[1])
# The fixture uses assertions even when testing a release configuration.
args = [x for x in args if x != '-DNDEBUG']
with tempfile.TemporaryDirectory() as tmp:
    exe = Path(tmp) / 'test'
    subprocess.run(args + [str(root / 'tests/hud_fade_config_test.cpp'),
                   str(root / 'src/config.cpp'), '-Wl,--gc-sections', '-o', str(exe)],
                   cwd=build, check=True)
    subprocess.run([str(exe), str(Path(tmp) / 'data')], check=True)
print('HUD fade configuration passed: defaults, all preset copies/reset, export/import and legacy import')
