"""Compile production mode migration, ground-clearance gate and activation logic."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
config = (root/'src/config.cpp').read_text()
source = (root/'src/bullet_time.cpp').read_text()
def function(signature, text):
    start = text.index(signature + '(')
    return text[start:text.index('\n}', start)+2]
a = config.index('    if (get_int(s_bulletTimeMode, -1, -1, 2)')
b = config.index('    if (!get_bool(s_aimDefaultsMigrated', a)
migration = config[a:b]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
using Clock=std::chrono::steady_clock;
constexpr bool FALSE=false;
constexpr float G_CM3D_F_INF=1e9f;
constexpr int MOD_OK=0,MOD_ERROR=1;
enum class BulletTimeMode {Off,Always,Botw};
int s_bulletTimeMode=1,s_bulletTime=2,setting=-1,writes=0;
bool legacy=true,writeOk=true,hasMode=true,hasLegacy=true;
int get_int(int,int fallback,int low,int high){return hasMode?std::clamp(setting,low,high):fallback;}
bool get_bool(int,bool fallback){return hasLegacy?legacy:fallback;}
bool set_int(int,int v){++writes;if(!writeOk)return false;setting=v;hasMode=true;return true;}
namespace mods {int set_error(void*,int code,const char*){return code;}}
int migrate(){void* error=nullptr;
// MIGRATION
return MOD_OK;}
// CONFIG
struct cXyz {float x=0,y=0,z=0;};
struct daAlink_c {
 struct {cXyz pos;} current;
 float gravity=-3.4f,maxFallSpeed=-200;bool special=true;int changes=0;
 bool checkNoResetFlg3(int){return special;}
 void setSpecialGravity(float,float,bool){++changes;}
};
namespace daPy_py_c {constexpr int FLG3_UNK_4000=1;}
struct JumpDefaults {float mMaxJumpSpeed=26,mJumpSpeedRate=1.3f;short mJumpAngle=9158;float mGravity=-3.4f;};
struct daAlinkHIO_autoJump_c0 {static inline const JumpDefaults m{};};
float cM_ssin(short angle){return std::sin(angle*6.2831853071795864769/65536);}
struct dBgS_LinkGndChk {cXyz pos;void SetPos(const cXyz* v){pos=*v;}};
float groundHeight=0;int queries=0;cXyz queried;
struct World {float GroundCross(dBgS_LinkGndChk* ground){++queries;queried=ground->pos;return groundHeight;}} world;
World& dComIfG_Bgsp(){return world;}
bool s_bulletTimeActive=false,s_bulletTimeUsedForJump=false,stamina=true,s_previousSpecialGravity=false;
float s_previousGravity=0,s_previousMaxFallSpeed=0;
Clock::time_point s_bulletTimeStarted{};
bool stamina_available_for_bullet_time(){return stamina;}
void stop_flurry_rush(){}void clear_combat_time_caches(){}
void sync_bullet_time_gyro_keep_alive(){}void sync_slow_motion_controllers(){}
// HEIGHT
// START
float apex(float multiplier){
 const auto& j=daAlinkHIO_autoJump_c0::m;
 float speed=j.mMaxJumpSpeed*j.mJumpSpeedRate*cM_ssin(j.mJumpAngle)*std::sqrt(multiplier),height=0;
 for(speed+=j.mGravity;speed>0;speed+=j.mGravity)height+=speed;
 return height;
}
int main(){
 for(bool old:{false,true}){
  setting=-1;legacy=old;writes=0;
  assert(migrate()==MOD_OK&&writes==1);assert(setting==(old?1:0));
  legacy=!old;assert(migrate()==MOD_OK&&writes==1);assert(setting==(old?1:0));
 }
 setting=-1;hasLegacy=false;assert(migrate()==MOD_OK&&setting==1);hasLegacy=true;
 setting=2;legacy=false;writes=0;assert(migrate()==MOD_OK&&setting==2&&writes==0);
 setting=-1;writeOk=false;assert(migrate()==MOD_ERROR&&setting==-1);writeOk=true;
 for(int value:{0,1,2}){setting=value;assert(bullet_time_enabled()==(value!=0));assert(int(bullet_time_mode())==value);}
 daAlink_c link;link.current.pos={25,0,-50};
 setting=0;assert(!bullet_time_height_allowed(&link));
 setting=1;assert(bullet_time_height_allowed(&link));assert(queries==0); // Always bypasses only the new gate.
 setting=2;const float threshold=botw_minimum_ground_clearance();
 assert(std::fabs(threshold-2*apex(1))<0.001f);
 assert(apex(1)<threshold&&apex(2)>=threshold);
 for(float height:{0.0f,apex(1),threshold-0.1f}){link.current.pos.y=height;assert(!bullet_time_height_allowed(&link));}
 for(float height:{threshold,apex(2),apex(6)}){link.current.pos.y=height;assert(bullet_time_height_allowed(&link));}
 // Same launch height: dropping off a ledge changes the floor underneath Link.
 link.current.pos.y=apex(1);groundHeight=0;assert(!bullet_time_height_allowed(&link));
 groundHeight=-200;assert(bullet_time_height_allowed(&link));assert(queried.x==25&&queried.z==-50);
 groundHeight=link.current.pos.y;assert(!bullet_time_height_allowed(&link)); // Raised/moving platform.
 groundHeight=-G_CM3D_F_INF;assert(!bullet_time_height_allowed(&link));
 groundHeight=std::numeric_limits<float>::quiet_NaN();assert(!bullet_time_height_allowed(&link));
 groundHeight=0;link.current.pos.y=threshold;
 link.gravity=-0.1f;assert(bullet_time_height_allowed(&link)); // Runtime slow-motion gravity cannot move the threshold.
 assert(!bullet_time_height_allowed(nullptr));
 // Failed height checks neither start nor consume the once-per-jump activation.
 link.current.pos.y=apex(1);start_bullet_time(&link);assert(!s_bulletTimeActive&&!s_bulletTimeUsedForJump&&link.changes==0);
 link.current.pos.y=apex(2);start_bullet_time(&link);assert(s_bulletTimeActive&&s_bulletTimeUsedForJump&&link.changes==1);
 link.current.pos.y=0;start_bullet_time(&link);assert(s_bulletTimeActive&&link.changes==1); // Existing active duration retained.
 s_bulletTimeActive=s_bulletTimeUsedForJump=false;setting=1;start_bullet_time(&link);assert(s_bulletTimeActive);
 s_bulletTimeActive=s_bulletTimeUsedForJump=false;setting=0;start_bullet_time(&link);assert(!s_bulletTimeActive);
 setting=2;link.current.pos.y=apex(6);stamina=false;start_bullet_time(&link);assert(!s_bulletTimeActive);
}
'''
fixture = fixture.replace('// MIGRATION', migration)
fixture = fixture.replace('// CONFIG', function('BulletTimeMode bullet_time_mode',config)+'\n'+function('bool bullet_time_enabled',config))
fixture = fixture.replace('// HEIGHT', function('float botw_minimum_ground_clearance',source)+'\n'+function('bool bullet_time_height_allowed',source))
fixture = fixture.replace('// START',function('void start_bullet_time',source))
assert 'jump_height_multiplier' not in function('float botw_minimum_ground_clearance',source)
assert 'gale_height_bonus' not in function('float botw_minimum_ground_clearance',source)
assert 'kBulletTimeOptions[] = {"Off", "Always", "BOTW"}' in (root/'src/ui.cpp').read_text()
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Bullet Time modes passed: legacy migration, fixed native-height threshold, ledges, activation and unchanged Always behavior')
