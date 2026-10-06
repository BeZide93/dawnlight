"""Round-trip every real registered setting, including touch layout strings."""
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else root / "build").resolve()
ninja = (build / "build.ninja").read_text()
block = ninja.split("build CMakeFiles/dawnlight_lite.dir/src/config.cpp.o:", 1)[1].split("\nbuild ", 1)[0]
args = ["c++", "-ffunction-sections", "-fdata-sections", "-I" + str(root / "src"), "-I" + str(root / "include")]
for key in ("DEFINES", "FLAGS", "INCLUDES"):
    line = next(x for x in block.splitlines() if x.startswith("  " + key + " = "))
    args += shlex.split(line.split(" = ", 1)[1])
args = [x for x in args if x != "-DNDEBUG"]
with tempfile.TemporaryDirectory() as tmp:
    exe = Path(tmp) / "test"
    subprocess.run(args + [str(root / "tests/settings_io_test.cpp"),
                   str(root / "src/config.cpp"), str(root / "src/touch_buttons.cpp"),
                   "-Wl,--gc-sections", "-o", str(exe)], cwd=build, check=True)
    subprocess.run([str(exe), str(Path(tmp) / "data")], check=True)
print("All settings passed: complete registry, touch strings, round-trip, validation, dependencies and rollback")
