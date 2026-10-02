"""Run production boss movement/timer/history callbacks without game assets."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def read(path):
    return (root / path).read_text()
def strip(source):
    return re.sub(r'^#(?:include|pragma).*\n', '', source, flags=re.M)
def section(source, start, end):
    return source[source.index(start):source.index(end)]
fixture = read('tests/cave_enemy_slow_fixture.hpp')
profile = section(read('src/enemy_slow_motion/profile.hpp'), 'struct EnemySlowProfile;', 'const EnemySlowProfile& darknut_slow_profile();')
fixture = fixture.replace('// PRODUCTION_PROFILE', profile)
fixture = fixture.replace('static const cXyz Zero;', 'cXyz& operator*=(float f){x*=f;y*=f;z*=f;return *this;} static const cXyz Zero;')
fixture += r'''
bool eventRunning = false;
bool dComIfGp_event_runCheck(){return eventRunning;}
#define VREG_F(x) 0.0f
enum {fpcNm_B_OB_e, fpcNm_B_YOI_e};
enum {OB_ACTION_CORE_START, OB_ACTION_CORE_HOOK, OB_ACTION_CORE_END,
      OB_ACTION_FISH_END, OB_ACTION_CORE_CHANCE, OB_ACTION_FISH_NORMAL};
struct b_ob_class:fopAc_ac_c {
    mDoExt_McaMorfSO* mpCoreMorf=nullptr;
    struct Part {mDoExt_McaMorfSO *mpMorf=nullptr,*mpFinMorf=nullptr,
        *mpFinUnkMorf=nullptr,*mpFinBMorf=nullptr,*mpFinCMorf=nullptr;} mBodyParts[20];
    mDoExt_brkAnm *mpSuiBtk=nullptr,*mpSuiBrk=nullptr;
    int mDemoAction=0,mAction=OB_ACTION_FISH_NORMAL,field_0x2320=0;
    bool mCoreBattleMode=false,mFishBattleMode=true;
    float field_0x479c=0,field_0x47c0=0,field_0x5d04=0,mBossLightScale=0,mColsetBlend=0,mSuiBrkFrame=0;
    s16 field_0x476a=0,field_0x47ac=0,field_0x47ae=0,field_0x47bc=0,mBlureRate=0;
    csXyz mMoveAngle;dBgS_Acch mAcch;s16 mTimers[5]{},mHitIFrameTimer=0,field_0x4794=0,mAttnOffTimer=0;
    cXyz field_0x2324[512]{};csXyz field_0x3b24[512]{};
};
struct daB_YOI_c:fopAc_ac_c {
    Model* mpModel=nullptr;void* mpBlizzeta=nullptr;
    float mCrackAlpha=0,mScaleF=0,mYoseSpeed=0;s16 mAngleSpeedY=0;
    s16 mTimer1=0,mTimer2=0,mIFrameTimer=0,mDeleteTimer=0;
};
struct daE_VA_c:fopAc_ac_c {dBgS_Acch mMagicAcch[2];cXyz mMagicPos[2],mMagicOldPos[2];};
namespace dawnlight {
float enemy_hard_mode_chase_scale(const EnemySlowStep&, float*) {return 1.5f;}
}
'''
fixture += strip(read('src/enemy_slow_motion/integration.hpp'))
fixture += strip(read('src/enemy_slow_motion/boss.hpp'))
core = read('src/enemy_slow_motion.cpp')
fixture += '\nnamespace dawnlight {\n'
fixture += section(core, 'HookAction before_move(', 'HookAction before_collision(')
fixture += section(core, 'HookAction before_collision(', 'HookAction before_chase_zero(')
fixture += section(core, 'void after_move(', 'bool owns_animation(')
fixture += section(read('src/enemy_slow_motion/boss.cpp'), 'template <typename T, std::size_t N>', 'bool has_complete_symbol(')
fixture += section(read('src/enemy_slow_motion/death_sword.cpp'), 'void before_magic_collision(', '\n}\nconst EnemySlowProfile&')
fixture += '\n}\n'
for name in ('blizzeta_ice', 'morpheel'):
    fixture += strip(read(f'src/enemy_slow_motion/{name}.cpp')).replace('namespace dawnlight {', f'namespace dawnlight::test_{name} {{', 1)
fixture += read('tests/boss_enemy_slow_cases.hpp')
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'boss.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp) / 'boss'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
