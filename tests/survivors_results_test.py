"""Actual result builders: ordering, DPS, escaping, empty runs and navigation."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
Run s_run;Records s_records;unsigned s_map=0;bool s_storeFailed=false;
ModContext* mod_ctx=nullptr;
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const std::string& in){std::string out;for(char c:in)out+=c=='&'?"&amp;":std::string(1,c);return out;}
unsigned treasure_item(ChestReward,const Run&){return 0x43;}
const char* reward_name(ChestReward,const Run&){return "Ball & Chain";}
void close_high_score(ModContext*,void*){}
void request_delete_score(ModContext*,void*){}
void continue_endless(ModContext*,void*){}
void save_results(ModContext*,void*){}void replay(ModContext*,void*){}
std::vector<std::string> texts,classes,labels;std::vector<UiPressedFn> callbacks;std::vector<UiElementHandle> parents;unsigned rows=2;
struct Service{
 ModResult elem_set_class(ModContext*,UiElementHandle,const char* s,bool){classes.emplace_back(s);return MOD_OK;}
 ModResult pane_add_rml(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h)*h=4;texts.emplace_back(s);return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h)*h=4;texts.emplace_back(s);return MOD_OK;}
 ModResult pane_add_row(ModContext*,UiElementHandle,const UiRowDesc*,UiElementHandle* row){*row=++rows;return MOD_OK;}
}ui;auto* svc_ui=&ui;
ModResult button(UiElementHandle parent,const char* label,UiPressedFn callback){parents.push_back(parent);labels.emplace_back(label);callbacks.push_back(callback);return MOD_OK;}
'''
main=r'''
int main(){
 s_run.start(1);s_run.phase=Phase::Victory;s_run.ticks=3600;s_run.kills=200;
 s_run.ranks[unsigned(Upgrade::Bow)]=12;s_run.ranks[unsigned(Upgrade::FireEdge)]=1;
 s_run.damage.dealt[unsigned(DamageSource::Bow)]=900;s_run.damage.dealt[unsigned(DamageSource::FireEdge)]=100;
 assert(build_results(1,2)==MOD_OK&&texts.size()==3&&classes.size()==8);
 // The real window installs this stylesheet outside build_results. Exercise
 // that dependency here too: every assigned class must have a style rule.
 const std::string stylesheet=kResultsStyle;
 for(const auto& name:classes)assert(stylesheet.find("."+name+" {")!=std::string::npos);
 auto table=texts[0];assert(table.find("VICTORY")!=std::string::npos&&table.find("02:00")!=std::string::npos);
 assert(table.find("Twilit Rank 5")!=std::string::npos);
 assert(table.find("Hero's Bow")<table.find("Sword Attack"));
 assert(table.find("90.0%")!=std::string::npos&&table.find("7.5")!=std::string::npos&&table.find("1,000")!=std::string::npos);
 assert(table.find("Fire Edge")==std::string::npos&&texts[1].find("Power-ups")==std::string::npos);
 assert(texts[1].find("Ball &amp; Chain")!=std::string::npos);
 assert((labels==std::vector<std::string>{"Save records","Return to hub","Continue in Endless Mode"}));
 assert(callbacks[0]==save_results&&callbacks[1]==replay);assert(parents[0]!=parents[1]);
 s_run.dark_link=4;s_run.damage.dealt[unsigned(DamageSource::DarkLink)]=250;
 table=results_damage_rml();assert(table.find("Dark Link")!=std::string::npos&&table.find("1,250")!=std::string::npos);
 s_run.ranks[unsigned(Upgrade::Lightning)]=30;s_run.damage.dealt[unsigned(DamageSource::Lightning)]=500;
 table=results_damage_rml();assert(table.find("Lightning Strike")!=std::string::npos&&result_source_level(DamageSource::Lightning)==30);
 s_run.start(2);s_run.phase=Phase::Defeat;table=results_damage_rml();
 assert(table.find("DEFEAT")!=std::string::npos&&table.find("nan")==std::string::npos&&table.find("inf")==std::string::npos);
 assert(table.find("No damage dealt this run.")!=std::string::npos&&table.find("00:00")!=std::string::npos);
 s_storeFailed=true;texts.clear();assert(build_results(1,2)==MOD_OK);
 assert(texts.back().find("could not be stored")!=std::string::npos);
 for(bool cheats:{false,true})for(bool modded:{false,true}) {
  s_run.options.allow_cheats=cheats;s_run.modded=modded;table=results_damage_rml();
  assert((table.find("Cheated")!=std::string::npos)==cheats);
  assert((table.find("Modded")!=std::string::npos)==modded);
  if(cheats||modded)assert(table.find("sr-integrity")<table.find("sr-heading"));
 }
 assert(stylesheet.find(".sr-integrity { font-size: 13dp; color: #ff4545;")!=std::string::npos);
 s_run.start(3);assert(!s_run.modded&&!s_run.options.allow_cheats);
 // An archived result renders only the saved run, with navigation and deletion.
 HighScoreRun archived;archived.run.start(10);archived.run.phase=Phase::Victory;archived.run.modded=true;
 archived.run.damage.dealt[unsigned(DamageSource::DarkLink)]=123456;archived.run.dark_link=4;
 archived.score=87654;archived.meta_level=29;archived.unlock_points=28;
 const auto liveDamage=s_run.damage.dealt;const auto livePhase=s_run.phase;const auto records=s_records.encode();
 texts.clear();labels.clear();callbacks.clear();assert(build_results(1,2,&archived,2)==MOD_OK);
 assert(texts.size()==2&&(labels==std::vector<std::string>{"Back to maps","Delete Score"}));
 assert(callbacks[0]==close_high_score&&callbacks[1]==request_delete_score);
 assert(texts[0].find(arenas[2].name)!=std::string::npos&&texts[0].find("123,456")!=std::string::npos);
 assert(texts[0].find("Modded")!=std::string::npos&&texts[1].find("87,654")!=std::string::npos);
 assert(texts[1].find("<span>29</span>")!=std::string::npos&&texts[1].find("<span>28</span>")!=std::string::npos);
 assert(s_run.phase==livePhase&&s_run.damage.dealt==liveDamage&&s_records.encode()==records);
 s_run.debug_run=true;s_run.kills=100;s_run.phase=Phase::Victory;
 assert(results_damage_rml().find("Debug — no saved scores")!=std::string::npos);
 assert(results_loadout_rml().find("<span>XP earned</span><span>0</span>")!=std::string::npos);
 assert(result_number(1234567890)=="1,234,567,890");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_results.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'include'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Results: damage order, totals, DPS, loadout, empty runs and navigation passed')
