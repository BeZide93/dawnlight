// Enemy bars, chip/fade colors and floating-number presentation adapted from
// F1mmel (@F1mmel), Twilit Essentials src/hp_bars/hp_bars.cpp at 7682060c.
// Survivors feeds its own tracked slots; this module never enumerates actors
// or installs additional game hooks. See docs/features/survivors-enemy-hud.md.
#include "survivors_enemy_hud.hpp"
#include "survivors_enemy_hud_state.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "dusk/config_var.hpp"
#include "f_op/f_op_actor_mng.h"
#include "d/d_com_inf_game.h"
#include "d/d_camera.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_lib.h"
#include "JSystem/J2DGraph/J2DPrint.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <cmath>
#include <cstdio>
#include <string_view>
#include <vector>

namespace dawnlight::enemy_hud {
namespace {
constexpr size_t slot_count=512, visible_limit=64, popup_limit=96;
constexpr unsigned lifetime=62, ray_budget=8;
struct Entry {
    Health health{};
    cXyz anchor{0,0,0};
    float height=0,alpha=0,chip=1;
    bool eligible=false,los=false;
};
struct Popup {cXyz position{0,0,0};unsigned amount=0,age=lifetime;float velocity=4;};
std::array<Entry,slot_count> entries{};
std::array<Popup,popup_limit> popups{};
Options saved{},provider{},effects{};
std::array<ConfigVarHandle,3> config{};
using GetConfigVar=dusk::config::ConfigVarBase* (*)(std::string_view);
GetConfigVar get_config=nullptr;
struct Note {unsigned index;UiElementHandle handle;};
// Callback data has module lifetime. Text handles are pruned when their document
// closes, including simultaneous hub and Dawnlight settings documents.
std::array<unsigned,3> indices{0,1,2};
std::vector<Note> notes_live;
constexpr const char* keys[]{"survivors-hp-bars","survivors-hp-numbers","survivors-damage-numbers"};
constexpr const char* labels[]{"Show HP Bars","Show Exact HP Numbers","Show Damage Numbers"};
constexpr const char* notes[]{"HP bars: managed by Twilit Essentials.","HP numbers: managed by Twilit Essentials.","Damage numbers: managed by Twilit Essentials."};
constexpr const char* help[]{"Shows health bars above nearby Survivors enemies. Twilit Essentials takes priority when its HP bars are enabled.",
    "Shows current/max HP above enemy health bars. Also works with Twilit Essentials HP bars when its exact numbers are off.",
    "Shows floating health-loss numbers in Survivors. Twilit Essentials takes priority when its damage numbers are enabled."};
bool value(Options o,unsigned i){return i==0?o.bars:i==1?o.numbers:o.damage;}
void assign(Options& o,unsigned i,bool v){(i==0?o.bars:i==1?o.numbers:o.damage)=v;}
bool te_bool(const char* key) {
    const auto* var=get_config?get_config(key):nullptr;
    return var&&static_cast<const dusk::config::ConfigVar<bool>*>(var)->getValue();
}
void refresh_options() {
    if(!get_config&&svc_hook&&svc_hook->resolve) {
        void* address=nullptr;
        if(svc_hook->resolve(mod_ctx,"dusk::config::GetConfigVar",&address,nullptr)==MOD_OK)
            get_config=reinterpret_cast<GetConfigVar>(address);
    }
    Options next{};
    if(te_bool("mod.com_dusklight_twilit__essentials.enabled")) {
        next.bars=te_bool("mod.com_dusklight_twilit__essentials.hpBarsEnabled");
        next.numbers=next.bars&&te_bool("mod.com_dusklight_twilit__essentials.hpBarsShowNumbers");
        next.damage=te_bool("mod.com_dusklight_twilit__essentials.damageNumbersEnabled");
    }
    for(unsigned i=0;i<3;++i)if(config[i]) {
        bool v=value(saved,i);if(svc_config->get_bool(mod_ctx,config[i],&v)==MOD_OK)assign(saved,i,v);
    }
    const auto local=local_effects(saved,next);
    if(effects.damage!=local.damage || (effects!=local && !local.bars && !local.numbers))reset(); // No stale damage after a handoff.
    effects=local;provider=next;
}
bool project(const cXyz& world,cXyz& screen) {
    if(!dComIfGd_getView())return false;
    const auto& m=*dComIfGd_getProjViewMtx();
    if(m[3][0]*world.x+m[3][1]*world.y+m[3][2]*world.z+m[3][3]<=1)return false;
    auto position=world;mDoLib_project(&position,&screen);
    return std::isfinite(screen.x)&&std::isfinite(screen.y)&&screen.x>=mDoGph_gInf_c::getMinXF()-40&&
        screen.x<=mDoGph_gInf_c::getMaxXF()+40&&screen.y>=-40&&screen.y<=mDoGph_gInf_c::getHeightF()+40;
}
void text(const char* string,float x,float y,float size,JUtility::TColor color,u8 alpha) {
    auto* font=mDoExt_getSubFont();if(!font)font=mDoExt_getMesgFont();if(!font)return;
    // Same outlined cream/red style as TE, using Dawnlight's native J2D path.
    for(const auto offset:std::array<std::array<float,2>,4>{{{{-1,0}},{{1,0}},{{0,-1}},{{0,1}}}}) {
        J2DPrint outline(font,JUtility::TColor(0,0,0,255),JUtility::TColor(0,0,0,255));
        outline.setFontSize(size,size*1.23f);outline.printReturn(string,180,size*1.5f,HBIND_CENTER,VBIND_TOP,x-90+offset[0],y+offset[1],alpha);
    }
    J2DPrint print(font,color,color);print.setFontSize(size,size*1.23f);
    print.printReturn(string,180,size*1.5f,HBIND_CENTER,VBIND_TOP,x-90,y,alpha);
}
size_t ray_cursor=0;
}
ModResult initialize() {
    for(unsigned i=0;i<3;++i) {
        ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;desc.name=keys[i];desc.type=CONFIG_VAR_BOOL;desc.default_bool=false;
        const auto result=register_owned_setting(&desc,&config[i]);if(result!=MOD_OK)return result;
    }
    refresh_options();return MOD_OK;
}
ModResult build(UiElementHandle pane) {
    refresh_options();auto result=svc_ui->pane_add_section(mod_ctx,pane,"Enemy HP Bars");if(result!=MOD_OK)return result;
    for(unsigned i=0;i<3;++i) {
        UiElementHandle note=0;
        result=svc_ui->pane_add_text(mod_ctx,pane,notes[i],&note);if(result!=MOD_OK)return result;
        notes_live.push_back({i,note});svc_ui->elem_set_visible(mod_ctx,note,value(provider,i));
        UiControlDesc control=UI_CONTROL_DESC_INIT;control.kind=UI_CONTROL_TOGGLE;
        control.label=labels[i];control.help_rml=help[i];control.user_data=&indices[i];
        control.get=[](ModContext*,void* data,UiControlValue* out){const auto i=*static_cast<unsigned*>(data);out->bool_value=value(saved,i)||value(provider,i);};
        control.is_disabled=[](ModContext*,void* data){return disabled(*static_cast<unsigned*>(data),saved,provider);};
        control.set=[](ModContext*,void* data,const UiControlValue* in) {
            const auto i=*static_cast<unsigned*>(data);refresh_options();if(disabled(i,saved,provider))return;
            if(svc_config->set_bool(mod_ctx,config[i],in->bool_value)==MOD_OK){assign(saved,i,in->bool_value);refresh_options();}
        };
        result=svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);if(result!=MOD_OK)return result;
    }
    return MOD_OK;
}
void refresh() {
    refresh_options();
    std::erase_if(notes_live,[](const Note& note) {
        return svc_ui->elem_set_visible(mod_ctx,note.handle,value(provider,note.index))!=MOD_OK;
    });
}
void forget(size_t slot){if(slot<entries.size())entries[slot]={};}
void observe(size_t slot,const fopAc_ac_c* actor,bool eligible) {
    if((!effects.bars&&!effects.numbers&&!effects.damage)||slot>=entries.size()||!actor)return;
    auto& entry=entries[slot];const auto id=fopAcM_GetID(actor);
    if(entry.health.id!=id)entry={};
    const bool wasEligible=entry.eligible;
    const auto loss=entry.health.observe(id,actor->health,actor->field_0x560);
    float height=actor->attention_info.position.y-actor->current.pos.y;
    // Allow Survivors' enlarged elites to keep their real model-height anchor.
    if(height<20||height>800)height=actor->eyePos.y-actor->current.pos.y;
    if(height<20||height>800)height=55;
    entry.height=std::max(entry.height,height);entry.anchor=actor->current.pos+cXyz(0,entry.height+8,0);
    entry.eligible=eligible&&actor->health>0&&!(actor->actor_status&fopAcStts_NODRAW_e)&&!(actor->actor_condition&fopAcCnd_NODRAW_e);
    if(loss&&eligible&&wasEligible&&effects.damage) {
        auto popup=std::max_element(popups.begin(),popups.end(),[](const auto& a,const auto& b){return a.age<b.age;});
        *popup={entry.anchor,loss,0,4};
    }
}
void tick() {
    if(!effects.bars&&!effects.numbers&&!effects.damage)return;
    for(auto& p:popups)if(p.age<lifetime){++p.age;p.position.y+=p.velocity;p.velocity*=.90f;}
    auto* player=dComIfGp_getPlayer(0);if(!player)return;
    std::array<std::pair<float,size_t>,slot_count> nearest{};size_t count=0;
    if(effects.bars||effects.numbers)for(size_t i=0;i<entries.size();++i) {
        auto& e=entries[i];cXyz screen;
        if(!e.eligible){e.alpha=0;e.los=false;continue;}
        const float distance=(e.anchor-player->current.pos).abs();
        if(distance<4500&&project(e.anchor,screen))nearest[count++]={distance,i};
        else {e.alpha=0;e.los=false;}
    }
    const auto limit=std::min(count,visible_limit);
    std::partial_sort(nearest.begin(),nearest.begin()+limit,nearest.begin()+count);
    for(size_t i=limit;i<count;++i){entries[nearest[i].second].alpha=0;entries[nearest[i].second].los=false;}
    auto* camera=dCam_getBody();
    for(size_t n=0;n<std::min<size_t>(limit,ray_budget);++n) {
        const auto i=(ray_cursor+n)%limit;auto& e=entries[nearest[i].second];e.los=true;
        if(camera){auto start=camera->iEye(),end=e.anchor;auto diff=end-start;const float distance=diff.abs();
            if(distance>45){end=start+diff*((distance-45)/distance);e.los=!camera->lineBGCheck(&start,&end,0x40b7);}}
    }
    if(limit)ray_cursor=(ray_cursor+ray_budget)%limit;
    for(size_t i=0;i<limit;++i) {
        auto& e=entries[nearest[i].second];const float target=e.los?std::clamp((4500-nearest[i].first)/800,0.f,1.f):0;
        e.alpha+=(target-e.alpha)*.2f;
        const float ratio=std::clamp(float(e.health.hp)/e.health.maximum,0.f,1.f);
        e.chip=ratio>=e.chip?ratio:e.chip+(ratio-e.chip)*.12f;
    }
}
void draw() {
    if((!effects.bars&&!effects.numbers&&!effects.damage)||dComIfGp_event_runCheck())return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    const auto tl=graf->mColorTL,tr=graf->mColorTR,br=graf->mColorBR,bl=graf->mColorBL;
    const auto box=[&](float x,float y,float width,float height,JUtility::TColor color){
        graf->setup2D();graf->setColor(color);graf->fillBox(JGeometry::TBox2<f32>(x,y,x+width,y+height));
    };
    if(effects.bars||effects.numbers)for(const auto& e:entries) {
        cXyz screen;if(!e.eligible||e.alpha<.005f||!project(e.anchor,screen))continue;
        const auto alpha=[&](unsigned a){return u8(a*e.alpha);};
        const float ratio=std::clamp(float(e.health.hp)/e.health.maximum,0.f,1.f);
        const float x=screen.x-12.5f,y=screen.y-.5f;
        if(effects.bars) {
            box(x-1.5f,y-1.5f,28,4,{0,0,0,alpha(160)});
            box(x-1,y-1,27,3,{170,135,55,alpha(230)});box(x,y,25,1,{12,12,16,alpha(230)});
            const auto rgb=health_color(ratio);box(x,y,25*ratio,1,{rgb[0],rgb[1],rgb[2],alpha(240)});
            if(e.chip>ratio+.001f)box(x+25*ratio,y,25*(e.chip-ratio),1,{236,226,200,alpha(150)});
        }
        if(effects.numbers){char label[32];std::snprintf(label,sizeof(label),"%d/%d",e.health.hp,e.health.maximum);text(label,screen.x,y-13,7,{255,252,240,255},alpha(255));}
    }
    if(effects.damage)for(const auto& p:popups) {
        cXyz screen;if(p.age>=lifetime||!project(p.position,screen))continue;
        const float scale=p.age<7?1.55f-p.age*.08f:1.f;const bool strong=p.amount>=60;
        const float size=(strong?24.f:17.f)*scale;char label[32];
        std::snprintf(label,sizeof(label),strong?"-%u!":"-%u",p.amount);
        const u8 alpha=p.age>lifetime-18?u8((lifetime-p.age)*255/18):255;
        text(label,screen.x,screen.y-size*1.23f-8,size,strong?JUtility::TColor(255,236,150,255):JUtility::TColor(255,150,120,255),alpha);
    }
    graf->setup2D();graf->setColor(tl,tr,br,bl);
}
void reset(){entries={};popups={};ray_cursor=0;}
void shutdown(){reset();notes_live.clear();get_config=nullptr;}
}
