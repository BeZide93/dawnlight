"""Production equipment storage, inventory UI, failures and hub input using SDK descriptors."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
runtime=(root/'src/survivors_gear_runtime.inc').read_text()
ui=(root/'src/survivors_gear_ui.inc').read_text()
# Use the host's real validation, so descriptor errors cannot hide behind mocks.
host=(root/'dusklight/src/dusk/mods/svc/ui.cpp').read_text()
a=host.index('constexpr size_t kUiControlStringSetModeSize =')
validator=host[a:host.index('constexpr size_t kUiControlOptionsSize =',a)]
a=host.index('bool valid_color_preset(')
validator+=host[a:host.index('bool copy_list_items(',a)]
fixture=r'''
#include <cassert>
#include <cmath>
#include <climits>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "survivors_rules.hpp"
#include "mods/svc/ui.h"
namespace ui {const char* material_icon(const char*){return "delete";}}
// VALIDATOR
using namespace dawnlight::survivors;
using s16=int16_t;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){} cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}float absXZ()const{return std::hypot(x,z);}};
struct daAlink_c{struct{cXyz pos;}current;bool checkWolf(){return false;}}player;
unsigned fopAcM_GetID(daAlink_c*){return 1;}
struct dBgS_LinChk{void Set(cXyz*,cXyz*,daAlink_c*){}};
bool wallBlocked=false;struct Bg{bool LineCross(dBgS_LinChk*){return wallBlocked;}}bg;Bg& dComIfG_Bgsp(){return bg;}
constexpr int BUTTON_STATUS_OPEN=1,PAD_1=0,PAD_BUTTON_A=1;
void dComIfGp_setDoStatusForce(int,int){}
namespace mDoCPd_c{struct Pad{unsigned mPressedButtonFlags=0;}pad;bool getTrigA(int){return pad.mPressedButtonFlags&PAD_BUTTON_A;}Pad& getCpadInfo(int){return pad;}}
unsigned nativeMaxLife=15,nativeLife=12;
unsigned dComIfGs_getMaxLife(){return nativeMaxLife;}unsigned dComIfGs_getLife(){return nativeLife;}
void dComIfGs_setMaxLife(unsigned value){nativeMaxLife=value;}void dComIfGs_setLife(unsigned value){nativeLife=value;}
Run s_run;bool s_active=true,s_gameStarted=true,s_loaded=true,s_hub=true,s_restart=false,s_hubDeparture=false,s_progressSaving=false,s_progressSaved=true,s_hubSaveRequested=false;
cXyz chestActorPosition;bool chestActorReady=false;bool survivors_equipment_chest_anchor(cXyz&);
bool modal=false,transition=false,visible=false;unsigned s_player=1;
bool save_modal_active(){return modal;}bool transitioning(){return transition;}bool ui_visible(){return visible;}bool playable(){return !modal&&!transition&&!visible;}
ModContext* mod_ctx=nullptr;
struct Log{void warn(ModContext*,const char*){}}logService;auto* svc_log=&logService;
std::map<std::string,std::vector<uint8_t>> blobs;bool failSave=false;unsigned writes=0;
struct Save{
 ModResult get_blob(ModContext*,const char* key,void* out,size_t* size){auto it=blobs.find(key);if(it==blobs.end())return MOD_UNAVAILABLE;if(*size<it->second.size())return MOD_INVALID_ARGUMENT;std::copy(it->second.begin(),it->second.end(),static_cast<uint8_t*>(out));*size=it->second.size();return MOD_OK;}
 ModResult set_blob(ModContext*,const char* key,const void* data,size_t size){if(failSave)return MOD_UNAVAILABLE;++writes;auto* p=static_cast<const uint8_t*>(data);blobs[key]={p,p+size};return MOD_OK;}
}save;auto* svc_save=&save;
UiService api{};const UiService* svc_ui=&api;
std::map<UiElementHandle,std::string> rml,labels;std::map<UiElementHandle,UiControlDesc> controls;std::map<UiElementHandle,UiElementHandle> parents;
UiElementHandle serial=10;UiWindowClosedFn onClosed=nullptr;bool failPush=false,failDialog=false;
std::array<UiTabDesc,2> tabs{};
std::array<UiDialogAction,2> dialogActions{};
UiDialogActionFn dialogDismiss=nullptr;
UiDialogHandle openDialog=0;std::string dialogBody,dialogTitle;
void activate_tab(unsigned i){controls.clear();rml.clear();labels.clear();parents.clear();assert(tabs[i].build(nullptr,99,1,2,tabs[i].user_data,nullptr)==MOD_OK);}
void dialog_action(unsigned i){const auto h=openDialog;assert(h);auto a=dialogActions[i];if(a.is_disabled&&a.is_disabled(nullptr,a.user_data))return;a.on_pressed(nullptr,h,a.user_data);if(!a.keep_open&&openDialog==h)openDialog=0;}
void dismiss_dialog(){auto h=openDialog;assert(h);dialogDismiss(nullptr,h,nullptr);openDialog=0;}
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const std::string& s){return s;}
void initialize_ui(){
 api.pane_add_row=[](ModContext*,UiElementHandle p,const UiRowDesc*,UiElementHandle* out){*out=++serial;parents[*out]=p;return MOD_OK;};
 api.pane_add_rml=[](ModContext*,UiElementHandle p,const char* text,UiElementHandle* out){auto id=++serial;rml[id]=text;parents[id]=p;if(out)*out=id;return MOD_OK;};
 api.pane_add_control=[](ModContext*,UiElementHandle p,const UiControlDesc* d,UiElementHandle* out){assert(valid_control_desc(*d));auto id=++serial;controls[id]=*d;labels[id]=d->label;parents[id]=p;if(out)*out=id;return MOD_OK;};
 api.elem_set_class=[](ModContext*,UiElementHandle,const char*,bool){return MOD_OK;};
 api.elem_set_rml=[](ModContext*,UiElementHandle id,const char* text){rml[id]=text;return MOD_OK;};
 api.control_set_tooltip=[](ModContext*,UiElementHandle,const char*){return MOD_OK;};
 api.window_push=[](ModContext*,const UiWindowDesc* d,UiWindowHandle* out){if(failPush)return MOD_UNAVAILABLE;assert(d->tab_count==2);std::copy_n(d->tabs,2,tabs.begin());*out=99;onClosed=d->on_closed;controls.clear();rml.clear();labels.clear();parents.clear();auto result=tabs[0].build(nullptr,*out,1,2,tabs[0].user_data,nullptr);visible=result==MOD_OK;return result;};
 api.dialog_push=[](ModContext*,const UiDialogDesc* d,UiDialogHandle* out){if(failDialog){*out=0;return MOD_UNAVAILABLE;}assert(d->struct_size==sizeof(UiDialogDesc)&&d->action_count==2&&d->on_dismiss);assert(d->actions[0].struct_size==sizeof(UiDialogAction)&&d->actions[1].struct_size==sizeof(UiDialogAction));assert(std::string(d->actions[0].label)=="Cancel"&&std::string(d->actions[1].label)=="Delete");dialogBody=d->body_rml;dialogTitle=d->title;std::copy_n(d->actions,2,dialogActions.begin());dialogDismiss=d->on_dismiss;*out=openDialog=++serial;return MOD_OK;};
 api.dialog_close=[](ModContext*,UiDialogHandle h){assert(h==openDialog);openDialog=0;return MOD_OK;};
 api.dialog_set_body=[](ModContext*,UiDialogHandle h,const char* body){assert(h==openDialog);dialogBody=body;return MOD_OK;};
 api.window_close=[](ModContext*,UiWindowHandle h){visible=false;if(onClosed)onClosed(nullptr,h,nullptr);return MOD_OK;};
}
'''
# SDK uses a named callback typedef; tolerate the exact installed SDK spelling.
fixture=fixture.replace('UiWindowClosedFn onClosed=nullptr;','void (*onClosed)(ModContext*,UiWindowHandle,void*)=nullptr;')
main=r'''
int main(){
 initialize_ui();s_run.phase=Phase::Ready;load_gear();assert(s_gearWritable&&s_gear.weapon==GearInventory::none);
 GearItem sword{GearKind::Sword,2,3,4},tunic{GearKind::Tunic,5,1,0};
 GearInventory next;next.add(sword);next.add(tunic);assert(store_gear(next));
 auto stored=blobs[kGearBlob];s_gear={};load_gear();assert(s_gear==next);
 // Production loader accepts both 92-byte legacy formats with a larger
 // destination buffer and maps the old unequipped sentinel correctly.
 for(uint8_t version:{1,2}) {
  std::vector<uint8_t> legacy(92);std::copy_n(stored.begin(),8,legacy.begin());for(unsigned i=0;i<20;++i)for(unsigned j=0;j<4;++j)legacy[8+4*i+j]=stored[8+5*i+j];legacy[4]=version;legacy[5]=legacy[6]=20;
  uint32_t hash=2166136261u;for(unsigned i=0;i<88;++i)hash=(hash^legacy[i])*16777619u;
  for(unsigned i=0;i<4;++i)legacy[88+i]=uint8_t(hash>>(8*i));
  blobs[kGearBlob]=legacy;load_gear();assert(s_gearWritable&&s_gear==next);
  assert(store_gear(s_gear)&&blobs[kGearBlob].size()==212&&blobs[kGearBlob][4]==4);
 }

 blobs[kGearBlob][3]^=1;load_gear();assert(!s_gearWritable&&!store_gear(next));assert(blobs[kGearBlob][3]!=stored[3]);
 blobs[kGearBlob]=stored;load_gear();failSave=true;auto bad=next;bad.erase(0);assert(!store_gear(bad)&&s_gear==next);failSave=false;
 open_gear_window();assert(s_gearWindow&&visible&&controls.size()==24);
 for(unsigned i=0;i<20;++i){assert(s_gearCards[i]&&s_gearButtons[i]);assert(labels[s_gearButtons[i]]=="Slot "+std::to_string(i+1));if(i%5)assert(parents[parents[s_gearButtons[i]]]==parents[parents[s_gearButtons[i-1]]]);}
 select_gear(nullptr,&s_gearSlots[0]);select_gear(nullptr,&s_gearSlots[1]);assert(s_gear.weapon==0&&s_gear.tunic==1);
 assert(rml[s_gearCards[0]].find("Equipped")!=std::string::npos);
 // Real control callbacks model touch/mouse activation, including repeat
 // activation of a selected card. Unequipping keeps the inventory item.
 auto tap=[&](unsigned i){const auto c=controls.at(s_gearButtons[i]);assert(!c.is_disabled(nullptr,c.user_data));c.on_pressed(nullptr,c.user_data);};
 tap(0);assert(s_gear.weapon==40&&s_gear.items[0]==sword&&s_gear.tunic==1);
 assert(rml[s_gearDetails]==gear_rml(sword));
 tap(0);assert(s_gear.weapon==0);failSave=true;tap(0);assert(s_gear.weapon==0);failSave=false;
 tap(1);assert(s_gear.tunic==40&&s_gear.items[1]==tunic);tap(1);assert(s_gear.tunic==1);
 // Every delete is modal and explicit. Selecting alone, Cancel, Escape/B,
 // dialog failure and failed save must leave both item and equipment intact.
 s_gearBanish=true;auto before=s_gear;auto beforeWrites=writes;tap(0);
 assert(s_gearDeleteDialog&&s_gear==before&&writes==beforeWrites&&dialogBody.find("Hero's Sword")!=std::string::npos&&dialogBody.find("equipped")!=std::string::npos);
 select_gear(nullptr,&s_gearSlots[1]);assert(s_gear==before);dialog_action(0);assert(!s_gearDeleteDialog&&!openDialog&&s_gear==before);
 tap(0);dismiss_dialog();assert(!s_gearDeleteDialog&&s_gear==before);
 failDialog=true;tap(0);assert(!s_gearDeleteDialog&&!openDialog&&s_gear==before);failDialog=false;
 tap(0);failSave=true;dialog_action(1);assert(s_gearDeleteDialog&&openDialog&&s_gear==before&&dialogBody.find("Nothing was deleted")!=std::string::npos);failSave=false;
 dialog_action(1);assert(!s_gearDeleteDialog&&!openDialog&&s_gear.weapon==40&&s_gear.items[0].kind==GearKind::Empty&&s_gear.tunic==1);
 // Lifecycle and stale-slot validation prevent a delayed confirmation from
 // deleting an item after run start, a reload or a window/page change.
 tap(1);s_run.phase=Phase::Running;confirm_delete_gear(nullptr,openDialog,nullptr);assert(s_gear.items[1]==tunic);s_run.phase=Phase::Ready;dismiss_dialog();
 tap(1);s_gear.items[1].first=4;confirm_delete_gear(nullptr,openDialog,nullptr);assert(s_gear.items[1].first==4);s_gear.items[1]=tunic;dismiss_dialog();
 tap(1);auto staleHandle=openDialog;auto callback=dialogActions[1].on_pressed;
 activate_tab(1);assert(!openDialog&&!s_gearDeleteDialog);callback(nullptr,staleHandle,nullptr);assert(s_gear.items[1]==tunic);
 for(unsigned i=0;i<40;++i)assert((bool)s_gearButtons[i]==(i>=20));
 assert(controls.size()==24&&labels[s_gearButtons[20]]=="Slot 21"&&labels[s_gearButtons[39]]=="Slot 40");
 s_gear.items[20]=sword;s_gear.items[39]=tunic;s_gearBanish=false;refresh_gear_ui();
 tap(20);assert(s_gear.weapon==20&&s_gear.tunic==1);tap(20);assert(s_gear.weapon==40&&s_gear.items[20]==sword);tap(20);
 tap(39);assert(s_gear.tunic==39);activate_tab(0);tap(1);assert(s_gear.tunic==1&&s_gear.weapon==20);
 activate_tab(1);assert(rml[s_gearCards[20]].find("Equipped")!=std::string::npos);
 s_gearBanish=true;tap(39);assert(s_gear.items[39]==tunic);dialog_action(1);assert(s_gear.items[39]==GearItem{}&&s_gear.weapon==20&&s_gear.tunic==1);
 auto persisted=s_gear;assert(store_gear(persisted));load_gear();assert(s_gear==persisted);
 activate_tab(0);
 auto saved=s_gear;s_run.phase=Phase::Running;select_gear(nullptr,&s_gearSlots[1]);assert(s_gear==saved);s_run.phase=Phase::Ready;
 s_gearBanish=true;tap(1);auto closeHandle=openDialog;auto closeCallback=dialogActions[1].on_pressed;
 onClosed(nullptr,s_gearWindow,nullptr);visible=false;closeCallback(nullptr,closeHandle,nullptr);assert(!openDialog&&s_gear.items[1]==tunic);assert(!s_gearWindow&&s_hubSaveRequested);
 s_hubSaveRequested=false;failPush=true;open_gear_window();assert(!s_gearWindow);failPush=false;
 chestActorPosition={0,0,100};mDoCPd_c::pad.mPressedButtonFlags=1;
 assert(!hub_gear_interaction(&player)&&!s_gearWindow);chestActorReady=true;
 wallBlocked=true;assert(!hub_gear_interaction(&player)&&!s_gearWindow);wallBlocked=false;
 assert(hub_gear_interaction(&player)&&s_gearWindow&&!mDoCPd_c::pad.mPressedButtonFlags);close_gear_window();
 chestActorPosition={0,0,500};assert(!hub_gear_interaction(&player));
 s_run.start(123);s_run.debug_run=true;auto rng=s_run.random;drop_boss_gear();assert(s_run.random==rng);
 // The override awards equipment only in a Debug-enabled build. Stale or
 // injected Debug state must not enable it in the release configuration.
 s_run.options.debug.max_equipment_drop_rate=1;
 for(unsigned i=0;i<100;++i){
  s_gear={};s_gearNoticeTicks=0;const auto beforeRandom=s_run.random,beforeWrites=writes;
  drop_boss_gear();
  if(debug_enabled)assert(s_gearNoticeTicks&&s_gear.items[0].kind!=GearKind::Empty);
  else assert(!s_gearNoticeTicks&&s_gear==GearInventory{}&&s_run.random==beforeRandom&&writes==beforeWrites);
 }
 assert(s_run.earned_meta_xp()==0);
 s_run.options.debug.max_equipment_drop_rate=0;
 // Legendary-only preserves the drop chance; together both overrides guarantee a Legendary.
 s_run.options.debug.always_legendary_equipment=1;
 for(bool maximum:{false,true}) {
  s_run.options.debug.max_equipment_drop_rate=maximum;unsigned legendaryDrops=0;bool sawWeapon=false,sawTunic=false;
  for(unsigned i=0;i<2000;++i) {
   s_gear={};s_gearNoticeTicks=0;drop_boss_gear();
   if(s_gearNoticeTicks) {
    ++legendaryDrops;const auto item=s_gear.items[0];assert(item.valid()&&item.rarity()==4);
    sawWeapon|=item.is_weapon();sawTunic|=!item.is_weapon();
    if(item.is_weapon())assert(item.perk!=GearPerk::None);
    GearInventory savedDrop;assert(savedDrop.decode(blobs[kGearBlob])&&savedDrop.items[0]==item);
   }
  }
  if(debug_enabled){assert(sawWeapon&&sawTunic);assert(maximum?legendaryDrops==2000:legendaryDrops>300&&legendaryDrops<500);}
  else assert(!legendaryDrops);
 }
 unsigned drops=0,swords=0,tunics=0;std::array<unsigned,unsigned(GearKind::Count)> kinds{};
 s_run.debug_run=false;s_run.options.debug.max_equipment_drop_rate=1;for(unsigned i=0;i<30000;++i){s_gear={};s_gearNoticeTicks=0;drop_boss_gear();if(s_gearNoticeTicks){++drops;assert(s_gearNoticeTicks==240);++kinds[unsigned(s_gear.items[0].kind)];if(s_gear.items[0].is_weapon())++swords;else ++tunics;}}
 assert(drops>5600&&drops<6400&&swords>4500&&swords<5100&&tunics>1000&&tunics<1400);
 for(unsigned k=1;k<unsigned(GearKind::Count);++k)assert(kinds[k]>0);
 // The production drop path uses the immutable run talents, including rarity.
 s_run.talents.ranks[unsigned(Talent::EquipmentDropRate)]=10;
 s_run.talents.ranks[unsigned(Talent::EquipmentRarity)]=10;
 unsigned boosted=0,topStats=0,allStats=0;
 for(unsigned i=0;i<30000;++i){s_gear={};s_gearNoticeTicks=0;drop_boss_gear();if(s_gearNoticeTicks){
  ++boosted;const auto item=s_gear.items[0];const auto values=item.stats();
  for(unsigned j=0;j<values.size();++j)if(item.definition().affixes[j].stat!=GearStat::None&&!item.definition().affixes[j].count){++allStats;topStats+=values[j]==5;}
 }}
 assert(boosted>20500&&boosted<21500);
 assert(topStats*100>allStats*9&&topStats*100<allStats*11);
 s_run.talents={};
 // Production boss/elite path honors rank 25, even without either gear talent.
 s_run.options.enemy_hp=s_run.options.boss_hp=s_run.options.enemy_damage=s_run.options.enemy_speed=s_run.options.horde=5;
 unsigned topLegendary=0;
 for(unsigned i=0;i<4000;++i){s_gear={};clear_gear_notices();drop_boss_gear();
  assert(s_gearNoticeTicks&&s_gear.items[0].valid()&&s_gear.items[0].rarity()>=3);
  topLegendary+=s_gear.items[0].rarity()==4;
 }
 assert(topLegendary>880&&topLegendary<1120);s_run.options={};
 // Eight active seconds per immutable drop, FIFO when bosses die together.
 clear_gear_notices();
 const GearItem firstNotice{GearKind::Sword,5,5,5,GearPerk::FireEdge},secondNotice{GearKind::Tunic,2,1,0};
 show_gear_notice({firstNotice,"Weapon acquired"});show_gear_notice({secondNotice,"Tunic acquired"});
 assert(s_gearNoticeTicks==240&&s_gearNoticeItem==firstNotice&&s_gearNoticeQueue.size()==1);
 for(unsigned i=0;i<239;++i)update_gear_notice();
 assert(s_gearNoticeTicks==1&&s_gearNoticeItem==firstNotice);
 update_gear_notice();assert(s_gearNoticeTicks==240&&s_gearNoticeItem==secondNotice&&s_gearNoticeQueue.empty());
 for(unsigned i=0;i<240;++i)update_gear_notice();assert(!s_gearNoticeTicks);
 show_gear_notice({{},"Inventory Full!"});assert(s_gearNoticeItem.kind==GearKind::Empty);
 show_gear_notice({secondNotice,"Tunic acquired"});clear_gear_notices();
 assert(!s_gearNoticeTicks&&s_gearNoticeQueue.empty()&&s_gearNoticeItem.kind==GearKind::Empty);
 // Equip exactly the visible item/slot; persist first and leave the replaced
 // item in inventory. Identical queued items still have distinct slot identities.
 s_gear={};s_run.start(123);nativeMaxLife=15;nativeLife=12;
 s_gear.add(sword);s_gear.equip(0);s_run.equip_gear(s_gear);
 const auto replacement=s_gear.add(firstNotice),pending=s_gear.add(secondNotice);
 show_gear_notice({firstNotice,"Weapon acquired",replacement});
 show_gear_notice({secondNotice,"Tunic acquired",pending});
 const auto savedGear=s_gear;failSave=true;equip_gear_notice();
 assert(s_gear==savedGear&&s_run.gear_weapon==sword&&s_gearNoticeSlot==replacement&&s_gearNoticeQueue.size()==1);
 failSave=false;equip_gear_notice();
 assert(s_gear.weapon==replacement&&s_run.gear_weapon==firstNotice&&s_gear.items[0]==sword);
 assert(s_run.rank(Upgrade::FireEdge)==1&&gear_description(firstNotice).find("Bonus: Fire Edge")!=std::string::npos);
 GearInventory savedPerk;assert(savedPerk.decode(blobs[kGearBlob])&&savedPerk.items[replacement].perk==GearPerk::FireEdge);
 assert(s_gearNoticeSlot==pending&&s_gearNoticeTicks==240&&s_gearNoticeQueue.empty());
 equip_gear_notice();assert(s_gear.tunic==pending&&s_run.gear_tunic==secondNotice&&!s_gearNoticeTicks);
 assert(nativeMaxLife==25&&nativeLife==12); // More capacity, no free healing.
 const auto untouched=s_gear;
 show_gear_notice({firstNotice,"Weapon acquired",pending});
 assert(!gear_notice_equip_available());equip_gear_notice();assert(s_gear==untouched);clear_gear_notices();
 // Preserve earned hearts when swapping at the cap; lower capacity clamps life.
 s_run.maximum_hearts=20;s_run.gear_hidden_hearts=0;s_run.gear_tunic={};
 const GearItem bigTunic{GearKind::Tunic,5,5,0},smallTunic{GearKind::Tunic,1,1,0};
 s_run.equip_drop(bigTunic);assert(s_run.maximum_hearts==20&&s_run.gear_hidden_hearts==5);
 s_run.equip_drop(smallTunic);assert(s_run.maximum_hearts==20&&s_run.gear_hidden_hearts==1);
 s_run.gear_tunic=bigTunic;s_run.maximum_hearts=10;s_run.gear_hidden_hearts=0;nativeMaxLife=50;nativeLife=40;
 const auto smallSlot=s_gear.add(smallTunic);show_gear_notice({smallTunic,"Tunic acquired",smallSlot});
 equip_gear_notice();assert(s_run.maximum_hearts==6&&nativeMaxLife==30&&nativeLife==24&&!s_gearNoticeTicks);
 // Every weapon has its English variant name, family icon and translated
 // numeric values in the actual rendered grid and detail text.
 const std::array<std::pair<GearKind,const char*>,10> named{{
  {GearKind::Sword,"Hero's Sword"},{GearKind::Greatsword,"Greatsword"},
  {GearKind::HerosBow,"Hero's Bow"},{GearKind::SharpshooterBow,"Sharpshooter Bow"},
  {GearKind::StormBoomerang,"Storm Boomerang"},{GearKind::HurricaneBoomerang,"Hurricane Boomerang"},
  {GearKind::BombBag,"Bomb Bag"},{GearKind::SiegeBombs,"Siege Bombs"},
  {GearKind::IronBall,"Iron Ball"},{GearKind::BreakerBall,"Breaker Ball"}}};
 s_run.phase=Phase::Ready;open_gear_window();
 for(const auto& [kind,name]:named) {
  GearItem item{kind,5,5,5};if(kind==GearKind::SharpshooterBow)item.third=2;if(kind==GearKind::HurricaneBoomerang)item.second=2;
  s_gear={};s_gear.add(item);refresh_gear_ui();select_gear(nullptr,&s_gearSlots[0]);
  assert(s_gear.weapon==0&&rml[s_gearCards[0]].find(name)!=std::string::npos);
  assert(rml[s_gearCards[0]].find(std::string("item://item/")+gear_icon(item))!=std::string::npos);
  const auto description=gear_description(item);assert(description.find(name)!=std::string::npos&&description.find("+5 ")!=std::string::npos);
  if(kind==GearKind::SharpshooterBow)assert(description.find("+2 additional pierced targets")!=std::string::npos&&description.find("+25% arrow range")!=std::string::npos);
  if(kind==GearKind::BombBag)assert(description.find("+10% explosion radius")!=std::string::npos);
 }
 close_gear_window();

 // The top toolbar uses the same host button callbacks for every input method.
 s_run.phase=Phase::Ready;clear_gear_notices();s_gear={};
 const GearItem legendarySword{GearKind::Sword,5,5,5},legendaryTunic{GearKind::Tunic,5,5,0},commonBow{GearKind::HerosBow,1,1,1};
 s_gear.items[0]=commonBow;s_gear.items[7]=s_gear.items[31]=legendarySword;s_gear.items[20]=legendaryTunic;
 s_gear.equip(31);s_gear.equip(20);assert(store_gear(s_gear));
 show_gear_notice({legendarySword,"Weapon acquired",31});show_gear_notice({legendarySword,"Weapon acquired",7});
 open_gear_window();
 auto toolbar=[&](const char* label){
  auto it=std::find_if(labels.begin(),labels.end(),[&](const auto& entry){return entry.second==label;});assert(it!=labels.end());
  assert(it->first<s_gearButtons[0]||!s_gearButtons[0]); // Added above all grid controls.
  const auto c=controls.at(it->first);assert(c.kind==UI_CONTROL_BUTTON);
  if(!c.is_disabled||!c.is_disabled(nullptr,c.user_data))c.on_pressed(nullptr,c.user_data);
 };
 auto unordered=s_gear;const auto unorderedBlob=blobs[kGearBlob];
 failSave=true;toolbar("Sort by Rarity");assert(s_gear==unordered&&blobs[kGearBlob]==unorderedBlob&&s_gearNoticeSlot==31&&s_gearNoticeQueue[0].slot==7);failSave=false;
 toolbar("Sort by Rarity");
 assert(s_gear.items[0]==legendarySword&&s_gear.items[1]==legendarySword&&s_gear.items[2]==legendaryTunic&&s_gear.items[3]==commonBow);
 assert(s_gear.weapon==1&&s_gear.tunic==2&&s_gearNoticeSlot==1&&s_gearNoticeQueue[0].slot==0);
 auto sorted=s_gear;toolbar("Sort by Rarity");assert(s_gear==sorted); // Stable ties.
 toolbar("Sort by Type");assert(s_gear.items[2]==commonBow&&s_gear.items[3]==legendaryTunic&&s_gear.tunic==3);
 for(unsigned i=4;i<40;++i)assert(s_gear.items[i].kind==GearKind::Empty);
 // Invalid permutations cannot overwrite data.
 auto invalid=GearInventory::slot_order();invalid[0]=invalid[1];sorted=s_gear;
 assert(!reorder_gear(invalid)&&s_gear==sorted);
 invalid=GearInventory::slot_order();invalid[0]=40;assert(!reorder_gear(invalid)&&s_gear==sorted);
 toolbar("Move Items");tap(1);assert(s_gearMoveSlot==1&&s_gear.weapon==1);
 activate_tab(1);assert(s_gearMove&&s_gearMoveSlot==1); // Source survives page rebuild.
 failSave=true;tap(39);assert(s_gear==sorted&&s_gearMoveSlot==1&&s_gearNoticeSlot==1);failSave=false;
 tap(39);assert(s_gear.items[1].kind==GearKind::Empty&&s_gear.weapon==39&&s_gearNoticeSlot==39&&gear_notice_equip_available());
 tap(39);activate_tab(0);tap(3); // Swap equipped items across pages.
 assert(s_gear.weapon==3&&s_gear.tunic==39&&s_gear.items[3]==legendarySword&&s_gear.items[39]==legendaryTunic);
 assert(s_gearNoticeSlot==3&&s_gearNoticeQueue[0].slot==0);
 auto moved=s_gear;tap(3);tap(3);assert(s_gearMoveSlot==40&&s_gear==moved);
 tap(3);toolbar("Move Items");assert(!s_gearMove&&s_gearMoveSlot==40&&s_gear==moved);
 // Sorting cannot race modal deletion or mutate during run/transition.
 s_gearBanish=true;tap(3);assert(s_gearDeleteDialog);toolbar("Sort by Type");assert(s_gear==moved);dismiss_dialog();s_gearBanish=false;
 s_run.phase=Phase::Running;sort_gear(false);assert(s_gear==moved);s_run.phase=Phase::Ready;
 toolbar("Move Items");tap(3);close_gear_window();assert(!s_gearMove&&s_gearMoveSlot==40);
 load_gear();assert(s_gear==moved);clear_gear_notices();
 for(auto& item:s_gear.items)item=sword;auto full=s_gear;while(true){s_gearNoticeTicks=0;drop_boss_gear();if(s_gearNoticeTicks)break;}
 assert(s_gear==full&&std::string(s_gearNotice)=="Inventory Full!");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture.replace('// VALIDATOR',validator)+runtime+'\nbool survivors_equipment_chest_anchor(cXyz& p){p=chestActorPosition;return chestActorReady;}\n'+ui+main)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors equipment UI: Debug enabled/disabled, touch unequip, two-page sorting/moves, identity preservation, safe deletion confirmation, native descriptors, grid slots, equip/delete, save rollback, hub input and boss drops passed')
