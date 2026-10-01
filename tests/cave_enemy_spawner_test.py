"""Execute production spawn selection; verify native variants and failure guards."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root/'src/enemy_spawner.cpp').read_text()
header = (root/'src/enemy_spawner.hpp').read_text()
labels = header[header.index('inline constexpr'):header.index('// Scene ownership')]
tables = source[source.index('constexpr std::array<ProfileName'):source.index('DEFINE_HOOK(')]
function = source[source.index('ModResult spawn_enemy_for_testing('):source.index('\n}  // namespace dawnlight', source.index('ModResult spawn_enemy_for_testing('))]
ids = sorted(set(re.findall(r'\bfpcNm_\w+', tables+function)))
stubs = r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <cstdint>
using s8=int8_t;using s16=int16_t;using u32=uint32_t;using ProfileName=short;using ActorId=unsigned;
enum ModResult{MOD_OK,MOD_UNAVAILABLE,MOD_INVALID_ARGUMENT};
struct cXyz{float x,y,z;};struct csXyz{s16 x,y,z;};
struct daAlink_c{csXyz shape_angle{};struct{cXyz pos{};}current;};
struct ActorSpawnParams{u32 parameters;s8 argument;int room_num;cXyz position;csXyz angle;cXyz scale;void* create_function;};
void available(){}
struct Service{void (*create_actor)()=&available;}service;Service* svc_actor=&service;
daAlink_c player;daAlink_c* link=&player;
daAlink_c* daAlink_getAlinkActorClass(){return link;}
bool blocked=false;bool enemy_spawner_blocked_in_bossrush_hub(){return blocked;}
ModResult install_test_hooks(){return MOD_OK;}
float cM_ssin(s16 a){return std::sin(a*3.14159265359f/32768);}
float cM_scos(s16 a){return std::cos(a*3.14159265359f/32768);}
struct dBgS_ObjGndChk{void SetPos(cXyz*){}};
float groundY=25;struct Bg{float GroundCross(dBgS_ObjGndChk*){return groundY;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
int fopAcM_GetRoomNo(daAlink_c*){return 7;}
constexpr ActorId fpcM_ERROR_PROCESS_ID_e=~0u;
std::unordered_set<ActorId> s_testActors;
ActorSpawnParams captured{};ProfileName capturedProfile=0;int calls=0;
ModResult create_test_actor_in_player_layer(daAlink_c* p,ProfileName profile,const ActorSpawnParams& params,ActorId& id){
assert(p==link);captured=params;capturedProfile=profile;id=++calls;return MOD_OK;}
'''
cases = r'''
int main(){
    const char* names[]{"Helmasaur","Helmasaurus","Kargarok","Guay","Armos","Red Chu","Blue Chu","Yellow Chu","Purple Chu","Black Chu"};
    const ProfileName profiles[]{fpcNm_E_MM_e,fpcNm_E_MM_e,fpcNm_E_KR_e,fpcNm_E_GE_e,fpcNm_E_AI_e,fpcNm_E_SM2_e,fpcNm_E_SM2_e,fpcNm_E_SM2_e,fpcNm_E_SM2_e,fpcNm_E_SM2_e};
    const u32 params[]{0xff00,0xff00,0xffffff00,0xffff01,0xff000a,0xffff0010,0xffff0020,0xffff0030,0xffff0040,0xffff0060};
    for(int i=0;i<10;++i){
        assert(std::strcmp(kEnemySpawnerProfileLabels[33+i],names[i])==0);
        assert(spawn_enemy_for_testing(33+i)==MOD_OK);
        assert(capturedProfile==profiles[i]&&captured.parameters==params[i]);
        assert(captured.argument==(i==1?1:-1));
        assert(captured.position.y==((i==2||i==3)?225:25));
        assert(captured.position.z==300&&captured.room_num==7);
        assert(captured.angle.z==(i==2?255:0));assert(s_testActors.contains(calls));
    }
    // Old persisted indices still identify the same boundary entries.
    assert(std::strcmp(kEnemySpawnerProfileLabels[0],"Darknut")==0);
    assert(std::strcmp(kEnemySpawnerProfileLabels[32],"Stalfos")==0);
    assert(spawn_enemy_for_testing(-1)==MOD_INVALID_ARGUMENT);
    assert(spawn_enemy_for_testing(43)==MOD_INVALID_ARGUMENT);
    int oldCalls=calls;groundY=-1e9f;assert(spawn_enemy_for_testing(35)==MOD_UNAVAILABLE);
    groundY=25;blocked=true;assert(spawn_enemy_for_testing(35)==MOD_UNAVAILABLE);
    blocked=false;link=nullptr;assert(spawn_enemy_for_testing(35)==MOD_UNAVAILABLE);
    assert(calls==oldCalls);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'spawner.cpp'
    cpp.write_text(stubs+'\nenum { '+', '.join(ids)+' };\n'+labels+tables+function+cases)
    binary=Path(tmp)/'spawner'
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Cave enemy spawner tests passed')
