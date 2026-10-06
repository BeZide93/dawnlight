"""Exercise configurable gains and damage against the production hook callbacks."""
from pathlib import Path
import subprocess
import tempfile
from fierce_deity_lifecycle_test import fixture, state, preload_state, callbacks, function, root, source

fixture = fixture.replace('#include <map>', '#include <map>\n#include <vector>\n#include <type_traits>')
fixture = fixture.replace('return reinterpret_cast<T>(static_cast<void**>(args)[index]);', '''
    if constexpr(std::is_pointer_v<T>) return reinterpret_cast<T>(static_cast<void**>(args)[index]);
    else return static_cast<T>(reinterpret_cast<intptr_t>(static_cast<void**>(args)[index]));''')
fixture += '\nfloat pendingLife=0;float dComIfGp_getItemLifeCount(){return pendingLife;}\n'
start = source.index('struct DamageSample')
end = source.index('\n', source.index('std::vector<DamageSample> s_damageSamples;', start))
production = source[start:end] + '\n' + ''.join(function(n) for n in (
    'before_received_damage', 'after_received_damage', 'after_attack_power_check'))
checks = r'''
int main(){
 daAlink_c link;currentLink=&link;reset_for_link(&link);enabled=true;paused=false;
 auto set=[](DarkLinkSetting s,int v){settings[static_cast<size_t>(s)]=v;};
 set(DarkLinkSetting::Gauge,200);set(DarkLinkSetting::SwordGain,17);set(DarkLinkSetting::DamageGain,9);
 Collider collider;dCcU_AtInfo attack{&link,&collider};fopAc_ac_c enemy;
 void* swordArgs[]={&enemy,&attack};
 after_damage_check(nullptr,swordArgs,nullptr,nullptr);assert(s_state.meter==17);
 attack.mAttackPower=0;after_damage_check(nullptr,swordArgs,nullptr,nullptr);assert(s_state.meter==17);
 attack.mAttackPower=1;set(DarkLinkSetting::SwordGain,0);
 after_damage_check(nullptr,swordArgs,nullptr,nullptr);assert(s_state.meter==17);
 void* damageArgs[]={&link,reinterpret_cast<void*>(1),nullptr,nullptr,nullptr};
 auto damage=[&](float delta){
  before_received_damage(nullptr,damageArgs,nullptr,nullptr);pendingLife+=delta;
  after_received_damage(nullptr,damageArgs,nullptr,nullptr);
 };
 damage(-2);assert(s_state.meter==26);
 damage(0);damage(2);assert(s_state.meter==26); // armor/invulnerability/healing
 set(DarkLinkSetting::DamageGain,0);damage(-1);assert(s_state.meter==26);
 set(DarkLinkSetting::DamageGain,9);s_state.meter=198;damage(-1);assert(s_state.meter==200);
 s_state.meter=0;
 for(int gate=0;gate<8;++gate){
  if(gate==0)s_state.active=true;
  if(gate==1)paused=true;
  if(gate==2)enabled=false;
  if(gate==3)link.wolf=true;
  if(gate==4)link.dead=true;
  if(gate==5)link.sceneChange=true;
  if(gate==6)damageArgs[4]=reinterpret_cast<void*>(1); // prior-scene damage
  if(gate==7)damageArgs[1]=nullptr;
  damage(-1);assert(s_state.meter==0);
  s_state.active=false;paused=false;enabled=true;link.wolf=link.dead=link.sceneChange=false;
  damageArgs[4]=nullptr;damageArgs[1]=reinterpret_cast<void*>(1);
 }
 daAlink_c other;damageArgs[0]=&other;damage(-1);assert(s_state.meter==0);damageArgs[0]=&link;
 // Nested calls must not credit their damage again to the enclosing hook.
 before_received_damage(nullptr,damageArgs,nullptr,nullptr);
 void* nestedArgs[]={&link,reinterpret_cast<void*>(1),nullptr,nullptr,nullptr};
 before_received_damage(nullptr,nestedArgs,nullptr,nullptr);pendingLife-=1;
 after_received_damage(nullptr,nestedArgs,nullptr,nullptr);
 after_received_damage(nullptr,damageArgs,nullptr,nullptr);assert(s_state.meter==9&&s_damageSamples.empty());
 void* powerArgs[]={&attack};s_state.active=true;
 for(int percent:{0,50,100,200,350,1000}){
  set(DarkLinkSetting::DamageMultiplier,percent);attack.mAttackPower=3;
  after_attack_power_check(nullptr,powerArgs,nullptr,nullptr);
  assert(attack.mAttackPower==(3*percent+99)/100);
 }
 attack.mAttackPower=65535;after_attack_power_check(nullptr,powerArgs,nullptr,nullptr);assert(attack.mAttackPower==65535);
 s_state.active=false;attack.mAttackPower=3;after_attack_power_check(nullptr,powerArgs,nullptr,nullptr);assert(attack.mAttackPower==3);
 s_state.active=true;collider.type=0;after_attack_power_check(nullptr,powerArgs,nullptr,nullptr);assert(attack.mAttackPower==3);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture+state+preload_state+callbacks+production+checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Dark Link settings passed: configurable gains, real health damage, gates, nesting and bounded sword multiplier')
