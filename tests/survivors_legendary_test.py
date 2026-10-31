"""Legendary eligibility, independent burns, melee coverage and max-HP aura."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
ns={'__file__':str(root/'tests/survivors_status_test.py')}
exec((root/'tests/survivors_status_test.py').read_text().split("main = r'''")[0],ns)
# This fixture intentionally omits native defense hooks, the sole user of LIGHT.
ns['fixture']=ns['fixture'].replace(',dCcD_MTRL_LIGHT=1','')
main=r'''
int main(){
 const std::array powers{Upgrade::Hurricane,Upgrade::FireBomb,Upgrade::BombArrow,Upgrade::FireEdge,Upgrade::DarkAura,Upgrade::Shockwave};
 for(auto power:powers){
  Run r;r.start(1);
  if(power!=Upgrade::FireEdge)assert(!r.eligible(power));
  r.ranks[unsigned(Upgrade::Bow)]=r.ranks[unsigned(Upgrade::Boomerang)]=r.ranks[unsigned(Upgrade::Bombs)]=r.ranks[unsigned(Upgrade::ShieldAttack)]=1;r.shield=r.dark_link=1;
  assert(r.eligible(power)&&upgrade_cap(power)==1);
  r.phase=Phase::Upgrade;r.offers[0]=power;
  for(unsigned rarity=0;rarity<4;++rarity){r.offer_rarities[0]=Rarity(rarity);assert(!r.choose(0)&&upgrade_units(power,Rarity(rarity))==0);}
  r.offer_rarities[0]=Rarity::Legendary;assert(r.choose(0)&&r.rank(power)==1&&!r.eligible(power));
  assert(upgrade_units(power,Rarity::Legendary)==1&&r.upgrade_rupees(power,Rarity::Legendary)==20);
 }
 // Rounded cumulative tenths, including low weapon damage, exactly five ticks.
 for(unsigned base:{1u,3u,7u,10u,21u,1200u,32767u}){
  LegendaryBurn burn;burn.ignite(base);unsigned total=0;
  for(unsigned frame=1;frame<=150;++frame){const auto hit=burn.tick();if(frame%30)assert(!hit);total+=hit;}
  assert(total==(base*5+5)/10&&!burn.remaining&&!burn.tick());
 }
 LegendaryBurn refreshed;refreshed.ignite(100);for(int i=0;i<29;++i)assert(!refreshed.tick());
 refreshed.ignite(100);assert(refreshed.tick()==10); // No repeated-hit starvation.
 s_run.start(1);s_player=player.id=1;player.current.pos={0,0,0};
 std::array<e_ms_class,3> targets{};
 for(unsigned i=0;i<targets.size();++i){auto& a=targets[i];a.id=2+i;a.health=2000;a.field_0x560=1000;
  a.current.pos=cXyz(50+50*i,0,0);a.eyePos=a.current.pos+cXyz(0,50,0);actors[a.id]=&a;
  auto& e=s_enemies[i];e.id=a.id;e.profile=fpcNm_E_MS_e;e.initialized=true;e.dormant=true;}
 auto& effect=effect_for(s_enemies[0]);
 s_run.ranks[unsigned(Upgrade::Bombs)]=18;s_run.ranks[unsigned(Upgrade::FireBomb)]=1;
 s_attacks[0].weapon=unsigned(Upgrade::Bombs);s_attacks[0].collider.owner=&player;
 dCcU_AtInfo bomb{&s_attacks[0].collider,20};
 for(auto& a:targets)record_status_hit(&a,&bomb);
 for(int i=0;i<150;++i)update_enemy_effects();
 for(unsigned i=0;i<3;++i)assert(s_effects[i].fixedPending==10); // 20 * 10% * 5.
 clear_enemy_effects();s_run.ranks[unsigned(Upgrade::FireEdge)]=1;s_run.ranks[unsigned(Upgrade::Power)]=17;
 const unsigned melee=s_run.melee_power();
 for(auto action:{SwordAction::Normal,SwordAction::Jump}){
  s_swordAction=action;player.mAtCps[0].owner=&player;dCcU_AtInfo hit{&player.mAtCps[0],melee};
  record_status_hit(&targets[0],&hit);assert(effect.legendaryBurns[1].tenths==melee);
 }
 for(auto weapon:{Upgrade::Spin,Upgrade::ShieldAttack}){
  clear_enemy_effects();s_attacks[1].weapon=unsigned(weapon);s_attacks[1].collider.owner=&player;
  dCcU_AtInfo hit{&s_attacks[1].collider,50};record_status_hit(&targets[0],&hit);
  assert(effect_for(s_enemies[0]).legendaryBurns[1].tenths==melee);
 }
 clear_enemy_effects();s_run.dual_wield=30;s_swordAction=SwordAction::Normal;++s_meleeSerial;
 dCcU_AtInfo sword{&player.mAtCps[0],melee};record_status_hit(&targets[0],&sword);
 assert(effect_for(s_enemies[1]).legendaryBurns[1].tenths==melee);
 assert(effect_for(s_enemies[2]).legendaryBurns[1].tenths==melee);
 update_enemy_effects();assert(s_fireEdgeEmitters[0]&&s_fireEdgeEmitters[1]&&s_fireEdgeEmitters[2]);
 for(auto key:s_fireEdgeEmitters)assert(emitters[key].effect==bow_fire_effect());
 // Sheathed sword keeps burning at its model, independent of combat endpoints.
 player.drawn=false;swordModel.origin={10,20,30};update_fire_edge();
 assert(emitters[s_fireEdgeEmitters[2]].position.x==110);
 player.master=true;swordModel.origin={50,60,70};update_fire_edge();
 assert(emitters[s_fireEdgeEmitters[2]].position.x==170&&emitters[s_fireEdgeEmitters[0]].position.y==60);
 const auto expired=s_fireEdgeEmitters[1];emitters[expired].deleted=true;update_fire_edge();assert(s_fireEdgeEmitters[1]!=expired);
 player.mSwordModel=nullptr;update_fire_edge();for(auto key:s_fireEdgeEmitters)assert(!key);
 player.mSwordModel=&swordModel;player.drawn=true;player.master=false;update_fire_edge();assert(s_fireEdgeEmitters[0]);
 clear_enemy_effects();for(auto key:s_fireEdgeEmitters)assert(!key);
 // Aura respects form, range and walls; exact max-HP damage bypasses Dark's x2.
 s_run.start(2);s_run.ranks[unsigned(Upgrade::DarkAura)]=1;s_run.dark_link=1;
 update_enemy_effects();assert(!effect_for(s_enemies[0]).legendaryBurns[2].remaining);
 s_run.dark_link=30;targets[1].current.pos={500,0,0};targets[2].eyePos.x=999;
 for(int i=0;i<30;++i)update_enemy_effects();
 auto& aura=effect_for(s_enemies[0]);assert(aura.fixedPending==100&&aura.legendaryBurns[2].remaining);
 assert(!s_effects[1].fixedPending&&!s_effects[2].fixedPending);
 for(auto key:aura.emitters)assert(emitters[key].black);
 prepare_effect_hit(&targets[0],s_enemies[0]);assert(aura.fixedDamage==100);
 dCcU_AtInfo info{&aura.collider,0};Args args{&info,nullptr};after_power(nullptr,&args,nullptr,nullptr);assert(info.mAttackPower==100);
 finish_effect_hit(&targets[0],fpcNm_E_MS_e);
 targets[0].current.pos={500,0,0};
 for(int i=0;i<150;++i)update_enemy_effects();
 assert(aura.fixedPending==500&&!aura.legendaryBurns[2].remaining);
 for(auto key:aura.emitters)assert(!key);
 // Regular and fixed damage combine once at the native boundary.
 aura.damage=12;aura.fixedDamage=100;info={&aura.collider,0};after_power(nullptr,&args,nullptr,nullptr);assert(info.mAttackPower==124);
 clear_enemy_effects();assert(!aura.fixedPending&&!aura.fixedDamage);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(ns['fixture']+ns['helpers']()+ns['function']('native_sword_collider')+(root/'src/survivors_status.inc').read_text()+ns['function']('after_power')+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors Legendary: one-rank gates, exact burns, refresh, melee coverage, aura range/LOS/form and damage isolation passed')
