"""Execute production Cave enemy profiles with native-shaped, asset-free fixtures."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
names = ('helmasaur', 'armos', 'guay', 'kargarok', 'chu')
sources = [(root/'src/enemy_slow_motion'/f'{name}.cpp').read_text() for name in names]
fixture = (root/'tests/cave_enemy_slow_fixture.hpp').read_text()
profile = (root/'src/enemy_slow_motion/profile.hpp').read_text()
profile = profile[profile.index('struct EnemySlowProfile;'):profile.index('const EnemySlowProfile& darknut_slow_profile();')]
fixture = fixture.replace('// PRODUCTION_PROFILE', profile)
core = (root/'src/enemy_slow_motion.cpp').read_text()
collision = core[core.index('HookAction before_collision('):core.index('bool owns_chase_float(')]
fixture += '\nnamespace dawnlight { ' + collision + ' }\n'
ids = sorted(set(re.findall(r'\b(?:fpcNm_\w+|Z2SE_\w+)\b', '\n'.join(sources))))
fixture += '\nenum { ' + ', '.join(ids) + ' };\n'
fixture += re.sub(r'^#(?:include|pragma).*\n', '', (root/'src/enemy_slow_motion/integration.hpp').read_text(), flags=re.M)
for name, source in zip(names, sources):
    source = re.sub(r'^#include.*\n', '', source, flags=re.M)
    if name == 'guay':
        fixture += '\n#undef DEFINE_HOOK\n#define DEFINE_HOOK(target, name) struct name:GuayHookSignature<decltype(target)>{}\n'
        fixture += '#undef DEFINE_HOOK_SYMBOL\n#define DEFINE_HOOK_SYMBOL(symbol, signature, name) struct name:GuayHookSignature<signature>{}\n'
    fixture += source.replace('namespace dawnlight {', f'namespace dawnlight::test_{name} {{', 1)
    if name == 'guay':
        fixture += '\n#undef DEFINE_HOOK\n#define DEFINE_HOOK(target, name) struct name{}\n'
        fixture += '#undef DEFINE_HOOK_SYMBOL\n#define DEFINE_HOOK_SYMBOL(symbol, signature, name) struct name{}\n'
fixture += (root/'tests/cave_enemy_slow_cases.hpp').read_text()
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp)/'profiles.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp)/'profiles'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
