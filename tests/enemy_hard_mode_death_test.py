"""Reproduce the Wolf get-up/death race with Dawnlight's actual timer code.

Compiles the production pre-execute guard and per-profile timer adjustments.
The terminal condition expressions are read from the pinned Dusklight actors;
no game assets are needed. Also exercises fractional slow-motion ticks.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/enemy_hard_mode.cpp').read_text()
def balanced(text,start,opening='{',closing='}'):
    first=text.index(opening,start);depth=0
    for i in range(first,len(text)):
        if text[i]==opening:depth+=1
        elif text[i]==closing:
            depth-=1
            if depth==0:return text[first:i+1]
    raise AssertionError('unbalanced source')
def function(signature):
    start=source.index(signature)
    return source[start:source.index('{',start)]+balanced(source,start)

shorten=function('void shorten_attack_interval(')
cases=[]
for name in ['E_RD','E_DN','E_MF','E_OC']:
    a=shorten.index('case fpcNm_'+name+'_e:')
    b=shorten.find('\n    case ',a+1)
    cases.append(shorten[a:b if b!=-1 else shorten.index('\n    default:',a)])
native=[]
for kind,base,timers in [('rd','enemy','timer'),('dn','actor','timer'),('mf','actor','field_0x6c0')]:
    text=(root/f'dusklight/src/d/actor/d_a_e_{kind}.cpp').read_text()
    action=text.index(f'static void e_{kind}_damage(')
    action=balanced(text,action)
    assert re.search(r'ACTION_DAMAGE\s*=\s*21',text)
    down=action[action.index('        case 3:'):action.index('        case 10:')]
    # Search directly from the health condition to avoid incidental body geometry.
    death_at=down.rfind('if (',0,down.index('health <= 0'))
    death=balanced(down,death_at,'(',')')
    getup=balanced(down,down.index('if (daPy_getPlayerActorClass()'),'(',')')
    assert f'{timers}[0] = 80;' in action and f'{timers}[1] = 55;' in action
    native.append(f'''bool dead(e_{kind}_class* i_this) {{
        auto* actor=&i_this->{base};auto* a_this=actor;(void)a_this;
        return {death};
    }}
    bool gets_up(e_{kind}_class* i_this) {{return {getup};}}
    short* timers(e_{kind}_class& a) {{return a.{timers};}}
    fopAc_ac_c* base(e_{kind}_class& a) {{return &a.{base};}}''')
fixture=r'''
#include <cassert>
#include <cstdint>
#include <iostream>
constexpr int fpcNm_E_RD_e=1,fpcNm_E_DN_e=2,fpcNm_E_MF_e=3,fpcNm_E_OC_e=4;
constexpr int kLizardDamageAction=21;
struct fopAc_ac_c {int health=40;};
struct e_rd_class {fopAc_ac_c enemy;short timer[4]{},attack_timer=0,bow_shake_timer=0;void* bow_anm=nullptr;};
struct e_dn_class {fopAc_ac_c actor;short timer[4]{},unk_timer_1=0;};
struct e_mf_class {fopAc_ac_c actor;short field_0x6c0[4]{};};
struct daE_OC_c:fopAc_ac_c {short field_0x6c0=0,field_0x6c2=0,field_0x6c4=0;};
struct Profile {int name;};
struct EnemySlowStep {fopAc_ac_c* actor;Profile* profile;bool timerTick=true;int action=21;};
struct Clock {std::uint8_t phase=0;} clockState;
Clock& cadence_clock_for(fopAc_ac_c*){return clockState;}
bool enabled=true;
bool enemy_hard_mode_applies(int){return enabled;}
struct daPy_py_c {static constexpr int CUT_TYPE_DOWN=99;int getCutType(){return 0;}} player;
daPy_py_c* daPy_getPlayerActorClass(){return &player;}
'''
checks=r'''
void legacy_prepare(EnemySlowStep& step) {
    if(!step.timerTick || !enabled)return;
    clockState.phase=(clockState.phase+1)%3;
    if(clockState.phase<2)shorten_attack_interval(step);
}
template<class Actor> bool simulate(int profile,int recovery,int deathTimer,bool fixed,
                                   int cadence,int divisor=1,int lethalTick=0) {
    Actor a;Profile p{profile};EnemySlowStep step{base(a),&p};
    auto* t=timers(a);t[0]=recovery;t[1]=deathTimer;
    clockState.phase=cadence;
    int tick=0;
    for(int frame=0;frame<300*divisor;++frame) {
        step.timerTick=frame%divisor==0;
        base(a)->health=tick>=lethalTick ? 0 : 40;
        // Slow motion holds owned timers before native execute on non-ticks.
        if(!step.timerTick)for(int i=0;i<2;++i)if(t[i]>0)++t[i];
        if(fixed)prepare_enemy_hard_mode(step);else legacy_prepare(step);
        for(int i=0;i<2;++i)if(t[i]>0)--t[i];
        if(dead(&a))return true;
        if(gets_up(&a))return false;
        if(step.timerTick)++tick;
    }
    assert(false);return false;
}
template<class Actor> void check_profile(int profile) {
    for(int cadence=0;cadence<3;++cadence)for(int divisor : {1,2,4}) {
        assert(!simulate<Actor>(profile,80,55,false,cadence,divisor)); // reproduces black zombie
        assert(simulate<Actor>(profile,80,55,true,cadence,divisor));
        assert(simulate<Actor>(profile,80,55,true,cadence,divisor,20)); // lethal hit after knockdown
        assert(simulate<Actor>(profile,60,35,true,cadence,divisor)); // human sword
        assert(simulate<Actor>(profile,1000,35,true,cadence,divisor)); // completed Wolf takedown
    }
    enabled=false;assert(simulate<Actor>(profile,80,55,false,0));enabled=true;
    Actor a;Profile p{profile};EnemySlowStep step{base(a),&p};
    auto* t=timers(a);t[0]=20;
    // Alive knockdown/takedown retains native timing, ordinary attacks keep 5/3 cadence.
    enabled=true;clockState={};prepare_enemy_hard_mode(step);assert(t[0]==20);
    step.action=3;clockState={};
    for(int i=0;i<3;++i){prepare_enemy_hard_mode(step);--t[0];}
    assert(t[0]==15);
    enabled=false;t[0]=20;clockState={};prepare_enemy_hard_mode(step);assert(t[0]==20);
    enabled=true;step.actor->health=-10;prepare_enemy_hard_mode(step);assert(t[0]==20);
}
int main(){
    check_profile<e_rd_class>(fpcNm_E_RD_e);
    check_profile<e_dn_class>(fpcNm_E_DN_e);
    check_profile<e_mf_class>(fpcNm_E_MF_e);
    daE_OC_c b;Profile p{fpcNm_E_OC_e};EnemySlowStep step{&b,&p};
    b.field_0x6c0=20;clockState={};prepare_enemy_hard_mode(step);assert(b.field_0x6c0==19);
    b.health=0;prepare_enemy_hard_mode(step);assert(b.field_0x6c0==19); // native Bokoblin death
    step.actor=nullptr;prepare_enemy_hard_mode(step);step.actor=&b;step.profile=nullptr;prepare_enemy_hard_mode(step);
    std::cout << "Enemy Hard Mode: Wolf death race reproduced; Bulblin/Lizalfos/Dynalfos fixed, combat cadence preserved\n";
}
'''
code=fixture+'\ntemplate <typename T>\n'+function('void shorten_timer(')
code+='\nvoid shorten_attack_interval(EnemySlowStep& step){switch(step.profile->name){'+ '\n'.join(cases)+'\ndefault:break;}}\n'
code+=function('bool preserve_damage_timers(')+'\n'+function('void prepare_enemy_hard_mode(')+'\n'+'\n'.join(native)+checks
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'death.cpp';binary=Path(tmp)/'death';cpp.write_text(code)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
