"""Validate Survivors vertex packets against native J2D font/rectangle setup."""
from pathlib import Path
import re
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
source = (root / 'src/survivors.cpp').read_text()
graf = (sdk / 'libs/JSystem/src/J2DGraph/J2DGrafContext.cpp').read_text()
font = (sdk / 'libs/JSystem/src/JUtility/JUTResFont.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)


native = '\n'.join([
    function(graf, 'void J2DGrafContext::setup2D()'),
    function(graf, 'void J2DGrafContext::fillBox('),
    function(font, 'void JUTResFont::setGX()'),
])
# Only vertex descriptors and byte emission are emulated. Execute the actual
# native setup/fill functions, so the test does not invent a safe fillBox API.
constants = sorted(set(re.findall(r'\bGX_\w+', native)) - {'GX_MAX_TEVSTAGE'})
tracked = {'GXClearVtxDesc', 'GXSetVtxDesc', 'GXBegin', 'GXEnd',
           'GXPosition3f32', 'GXColor1u32', 'GXSetNumTexGens'}
noops = sorted(set(re.findall(r'\b(GX\w+)\(', native)) - tracked)
prefix = r'''
bool s_victory=false,s_deathPending=false;unsigned s_victoryTicks=0;

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include "survivors_roster.hpp"
#include "survivors_rules.hpp"
#include "survivors_hud_layout.hpp"
using namespace dawnlight::survivors;
using u8=unsigned char;
#define MULTI_CHAR(x) 1
using f32=float; using GXTevStageID=int; using GXBlendMode=int; using GXBlendFactor=int;
using Mtx=float[3][4];
void MTXIdentity(Mtx){}
#define ZoneScoped
'''
prefix += 'enum { GX_MAX_TEVSTAGE=16, ' + ', '.join(constants) + ' };\n'
prefix += '\n'.join('template<class... T> void '+name+'(T...){}' for name in noops)
stubs = r'''
unsigned packetBytes=0, expectedBytes=0, boxes=0, labels=0, texGens=0;
bool textureCoordinates=false;std::vector<std::array<float,2>> vertices;
void GXClearVtxDesc(){textureCoordinates=false;}
void GXSetVtxDesc(int attr,int type){if(attr==GX_VA_TEX0)textureCoordinates=type==GX_DIRECT;}
void GXSetNumTexGens(unsigned n){texGens=n;}
void GXBegin(int,int,unsigned vertices){
    packetBytes=0;
    // fillBox uses F32 XYZ + RGBA8. Font TEX0 is two direct U16 coordinates.
    expectedBytes=vertices*(12+4+(textureCoordinates?4:0));
}
void GXPosition3f32(float x,float y,float){packetBytes+=12;vertices.push_back({x,y});}
void GXColor1u32(unsigned){packetBytes+=4;}
void GXEnd(){assert(packetBytes==expectedBytes && "GX vertex packet length mismatch");++boxes;}
namespace JGeometry {
template<class T> struct TBox2 {
    struct Point {T x,y;} i,f;
    TBox2(T x,T y,T right,T bottom):i{x,y},f{right,bottom}{}
};
}
namespace JUtility {
struct TColor {
    unsigned value;
    TColor(unsigned v=0):value(v){}
    TColor(unsigned r,unsigned g,unsigned b,unsigned a):value((r<<24)|(g<<16)|(b<<8)|a){}
    operator unsigned() const {return value;}
};
}
struct J2DGrafContext {
    Mtx mPosMtx{}; int mLineWidth=6;
    struct {int mType=0,mSrcFactor=0,mDstFactor=0;} mBoxPart;
    JUtility::TColor mColorTL{1},mColorTR{2},mColorBR{3},mColorBL{4};
    void setup2D(); void fillBox(const JGeometry::TBox2<f32>&);
    void setColor(JUtility::TColor c){setColor(c,c,c,c);}
    void setColor(JUtility::TColor a,JUtility::TColor b,JUtility::TColor c,JUtility::TColor d){
        mColorTL=a;mColorTR=b;mColorBR=c;mColorBL=d;
    }
} context;
unsigned skulls=0,skullLookups=0;bool hasMapArchive=true,hasSkullTexture=true;
struct ResTIMG{} skullTexture,rupeeTexture,gearTexture;
unsigned gearIcons=0;std::array<float,4> gearGeometry{};
bool hasGearTexture=true;
struct ItemArchive{void* getIdxResource(unsigned){return hasGearTexture?&gearTexture:nullptr;}} itemArchive;
ItemArchive* dComIfGp_getItemIconArchive(){return &itemArchive;}
unsigned gear_collection_texture(GearItem){return 0x70;}
std::string number(unsigned n){return std::to_string(n);}
struct JUTTexture{const ResTIMG* getTexInfo(){return &rupeeTexture;}unsigned getWidth(){return 12;}unsigned getHeight(){return 24;}} rupee;
unsigned rupeeIcons=0,keys=0;unsigned chest_keys(){return keys;}
std::array<float,4> rupeeGeometry{};
bool hasRupeePane=true,hasRupeeTexture=true;

struct MapArchive{} mapArchive;
MapArchive* dComIfGp_getDmapResArchive(){return hasMapArchive?&mapArchive:nullptr;}
void* JKRGetNameResource(const char* name,MapArchive* archive){
 assert(archive==&mapArchive&&std::string(name)=="tt_map_icon_boss_ci8_32_00.bti");
 ++skullLookups;return hasSkullTexture?&skullTexture:nullptr;
}
std::array<float,4> skullGeometry{};
struct J2DPicture {
 const ResTIMG* image;
 explicit J2DPicture(const ResTIMG* resource):image(resource){assert(image==&skullTexture||image==&rupeeTexture||image==&gearTexture);}
 JUtility::TColor getBlack(){return JUtility::TColor(0);}JUtility::TColor getWhite(){return JUtility::TColor(0xffffffff);}
 void setAlpha(u8){}
 void setBlackWhite(JUtility::TColor b,JUtility::TColor w){assert(unsigned(b)==0&&unsigned(w)==0xffffffff);}
 JUTTexture* getTexture(int index){assert(index==0);return hasRupeeTexture?&rupee:nullptr;}
 void draw(float x,float y,float w,float h,bool a,bool b,bool c){
  assert(!a&&!b&&!c);
  if(image==&gearTexture){gearGeometry={x,y,w,h};++gearIcons;}else if(image==&rupeeTexture){rupeeGeometry={x,y,w,h};++rupeeIcons;}else{skullGeometry={x,y,w,h};++skulls;}
  textureCoordinates=true;
 }
};
struct JUTResFont {void setGX();} font,ruby;
unsigned rubyLabels=0;std::vector<std::string> printed;unsigned textColor=0;
bool hasFont=true, arena=true, transition=false, window=false, hasGraf=true;
JUTResFont* mDoExt_getMesgFont(){return hasFont?&font:nullptr;}
JUTResFont* mDoExt_getRubyFont(){return hasFont?&ruby:nullptr;}
enum {HBIND_CENTER,HBIND_LEFT,VBIND_TOP};
struct DrawText {std::string text;float x,y,size;};std::vector<DrawText> textPositions;
struct J2DPrint {
    JUTResFont* face;float size=0;
    J2DPrint(JUTResFont* f,JUtility::TColor c,JUtility::TColor):face(f){textColor=c;f->setGX();}
    void setFontSize(float x,float){size=x;}
    struct TSize{float field_0x0=0,field_0x4=0;};
    void initchar(){}
    float parse(const u8*,int count,int,void*,TSize& bounds,int,bool draw){assert(!draw);bounds.field_0x0=count*size*.5f;return 0;}

    void printReturn(const char* t,float width,float,int align,int,float x,float y,int){textPositions.push_back({t,x+(align==HBIND_CENTER?width*.5f:0),y,size});printed.emplace_back(t);++labels;if(face==&ruby)++rubyLabels;}
};

using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz {float x=0,y=0,z=0;float abs2(cXyz p)const{return (x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z);}};
struct Actor {int health=100,field_0x560=100;struct{cXyz pos;}current;};
std::array<Actor,8> actors;
Actor player;bool hasPlayer=true;
Actor* daAlink_getAlinkActorClass(){return hasPlayer?&player:nullptr;}
Actor* actor_by_id(ActorId id){return id<actors.size()?&actors[id]:nullptr;}
struct Enemy {ActorId id=kNone;bool elite=false,initialized=true,dormant=false,retiring=false,credited=false;
 int eliteSlot=-1;unsigned species=0;};
std::array<Enemy,8> s_enemies;
J2DPicture nativeRupee(&rupeeTexture);
struct Screen{J2DPicture* search(int){return hasRupeePane?&nativeRupee:nullptr;}} screen;
struct dMeter2Draw_c {Screen* mpScreen=&screen;} meter;
namespace mods {template<class T>T arg(void*,unsigned){return &meter;}}
unsigned bossBars=0;float bossPercentage=0;bool barAvailable=true;std::array<float,4> bossGeometry{};
bool draw_boss_health_meter(dMeter2Draw_c*,float percentage,float x,float y,float w,float h){
 bossGeometry={x,y,w,h};
 if(!barAvailable)return false;
 ++bossBars;bossPercentage=percentage;textureCoordinates=true;return true;
}
struct ModContext {};
unsigned s_hudEditorWindow=0,nativeWindow=0,nativePause=0;
bool pauseFlag=false,scenePause=false;
unsigned dMeter2Info_getWindowStatus(){return nativeWindow;}
unsigned dMeter2Info_getPauseStatus(){return nativePause;}
bool dComIfGp_isPauseFlag(){return pauseFlag;}
struct dScnPly_c {static bool isPause(){return scenePause;}};
auto hudValues=hud_defaults;
HudPlacement hud_position(HudPart part,float center){return hud_placement(center,hudValues[unsigned(part)]);}

int potion_slot(){return -1;}
Run s_run;bool s_chestsExhausted=false; unsigned s_spawnFailures=0;
bool saving=false;bool save_modal_active(){return saving;}
bool in_arena(){return arena;} bool transitioning(){return transition;} bool ui_visible(){return window;}
J2DGrafContext* dComIfGp_getCurrentGrafPort(){return hasGraf?&context:nullptr;}
struct mDoGph_gInf_c {static float getMinXF(){return 0;} static float getWidthF(){return 608;} static float getHeightF(){return 448;}};
'''
main = r'''
int main(){
    // First visible HUD after the entrance, ordinary runs, and both messages.
    for(auto phase:{Phase::Running,Phase::Upgrade,Phase::Treasure,Phase::Victory}){
        for(bool availableFont:{false,true})for(unsigned ticks:{0u,duration})for(unsigned failures:{0u,10u})for(unsigned horde:{0u,450u}){
            s_run.phase=phase;s_run.ticks=ticks;s_run.horde_remaining=horde;s_run.xp=4;s_spawnFailures=failures;hasFont=availableFont;
            boxes=labels=0;textureCoordinates=true;
            draw_hud(nullptr,nullptr,nullptr,nullptr);
            assert(boxes==2u && !textureCoordinates && texGens==0);
            assert(context.mColorTL==1 && context.mColorTR==2 && context.mColorBR==3 && context.mColorBL==4);
            assert((labels!=0)==availableFont);
        }
    }

    // Banner starts outside the right edge, arrives at the right margin and
    // exits right during its final 12 ticks. Icon/stats use the saved item copy.
    s_gearNoticeItem={GearKind::Sword,5,4,3};hasFont=true;
    for(unsigned remain:{240u,234u,228u,120u,12u,6u,1u}) {
        s_gearNoticeTicks=remain;vertices.clear();printed.clear();gearIcons=0;
        draw_gear_notice();assert(gearIcons==1&&vertices.size()==8&&!textureCoordinates);
        const float left=vertices[0][0];
        if(remain==240)assert(left==608);
        if(remain==228||remain==120||remain==12)assert(left==320);
        assert(left>=320&&left<=608&&vertices[0][1]==171);
        assert(gearGeometry[0]==left+10&&gearGeometry[2]==44);
        assert(printed.size()==6&&printed.back()=="Call Midna to Equip"&&printed[1]=="Hero's Sword"&&printed[2]=="+5 sword damage");
        assert(context.mColorTL==1&&context.mColorBR==3);
    }
    s_gearNoticeItem={GearKind::IronBall,5,5,5,GearPerk::ChainSpin};
    s_gearNoticeTicks=120;printed.clear();vertices.clear();draw_gear_notice();
    assert(printed[5]=="Bonus: Chain Spin"&&printed.back()=="Call Midna to Equip");
    assert(vertices[0][1]==163&&vertices[2][1]==285);
    s_gearNoticeItem={GearKind::Sword,5,4,3};
    hasGearTexture=false;gearIcons=0;draw_gear_notice();assert(!gearIcons);hasGearTexture=true;
    // Custom position and size apply to live drops and text-only errors too.
    hudValues[unsigned(HudPart::ItemBanner)]={-200,110,50};
    s_gearNoticeTicks=240;vertices.clear();draw_gear_notice();assert(vertices[0][0]>=608);
    s_gearNoticeTicks=120;vertices.clear();draw_gear_notice();
    assert(vertices[0][0]==34&&vertices[0][1]==83.5f&&gearGeometry[2]==22);
    const auto noticeBefore=s_gearNoticeItem;
    vertices.clear();draw_gear_notice(true);assert(s_gearNoticeTicks==120&&s_gearNoticeItem==noticeBefore);
    assert(vertices[0][0]==34&&vertices[0][1]==83.5f);
    s_gearNoticeItem={};s_gearNotice="Inventory Full!";printed.clear();gearIcons=0;
    draw_gear_notice();assert(!gearIcons&&printed.size()==1&&printed[0]=="Inventory Full!");
    s_gearNoticeItem=noticeBefore;s_gearNotice="Weapon acquired";hudValues=hud_defaults;
    s_gearNoticeTicks=0;
    // Rank 5 is the baseline; all five controls contribute through Rank 25.
    s_run.start(42);hasFont=true;printed.clear();rubyLabels=boxes=skulls=skullLookups=0;
    draw_run_rank_hud(304);assert(rubyLabels==2&&boxes==0&&skulls==1&&skullLookups==1&&printed[1]=="5");
    assert(printed[0].find("00:00")!=std::string::npos);
    s_run.options.enemy_hp=2;printed.clear();rubyLabels=boxes=0;
    draw_run_rank_hud(304);assert(rubyLabels==2&&boxes==0&&skulls==2&&printed[1]=="6");
    hasMapArchive=false;draw_run_rank_hud(304);assert(skulls==2&&skullLookups==2);hasMapArchive=true;
    hasSkullTexture=false;draw_run_rank_hud(304);assert(skulls==2&&skullLookups==3);hasSkullTexture=true;
    draw_run_rank_hud(304);assert(skulls==3&&skullLookups==4);
    s_run.start(42);s_run.options.enemy_hp=5;s_run.options.enemy_damage=5;s_run.options.boss_hp=5;
    s_run.options.enemy_speed=5;s_run.options.horde=5;s_run.phase=Phase::Victory;s_run.continue_endless();
    printed.clear();rubyLabels=0;hasFont=true;draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert(rubyLabels==2&&printed[0].find("Endless")!=std::string::npos&&printed[1]=="25");
    s_run.start(42);
    // Victory uses the boss font, a red ceiling-rounded 10-second timer,
    // and restores GX vertex descriptors for the next native HUD element.
    s_run.phase=Phase::Running;s_victory=true;hasFont=true;
    for(unsigned tick:{0u,1u,29u,30u,270u,299u,300u}) {
        printed.clear();rubyLabels=boxes=0;s_victoryTicks=tick;
        draw_hud(nullptr,nullptr,nullptr,nullptr);
        assert(rubyLabels==3&&printed[0]=="Victory"&&printed[1]=="Victory");
        assert(printed.back()==std::to_string(tick>=300?0:(300-tick+29)/30));
        assert(textColor==0xff4137ffu&&!boxes&&!textureCoordinates&&texGens==0);
    }
    s_victory=false;s_deathPending=true;s_run.phase=Phase::Defeat;boxes=labels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);assert(!boxes&&!labels);s_deathPending=false;
    s_run.phase=Phase::Running;saving=true;boxes=labels=rubyLabels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);assert(!boxes&&!labels&&!rubyLabels);saving=false;
    s_gearNoticeTicks=120;
    // All native menu opening/active/closing states hide every overlay, even
    // when the UI service reports no document (Collection is native).
    for(bool inHub:{false,true})for(unsigned state:{1u,2u,3u,4u,5u,10u}) {
        hub=inHub;nativeWindow=state;boxes=labels=prices=feedback=hubLabels=0;
        draw_hud(nullptr,nullptr,nullptr,nullptr);
        assert(!boxes&&!labels&&!prices&&!feedback&&!hubLabels);nativeWindow=0;
    }
    hub=false;
    for(unsigned reason=0;reason<3;++reason) {
        nativePause=reason==0;pauseFlag=reason==1;scenePause=reason==2;
        boxes=labels=prices=feedback=0;draw_hud(nullptr,nullptr,nullptr,nullptr);
        assert(!boxes&&!labels&&!prices&&!feedback);
    }
    nativePause=0;pauseFlag=scenePause=false;
    s_gearNoticeTicks=0;
    // Preview works behind its transparent UI, in the hub and with rank 0,
    // without emitting world labels or changing run/encounter state.
    s_hudEditorWindow=1;window=hub=true;s_run.phase=Phase::Ready;
    const auto ticksBefore=s_run.ticks;const auto targetBefore=s_bossHudTarget;
    boxes=labels=prices=feedback=0;draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert(boxes==4&&labels&&bossPercentage==65&&!prices&&!feedback);
    assert(s_run.phase==Phase::Ready&&s_run.ticks==ticksBefore&&s_bossHudTarget==targetBefore);
    nativeWindow=3;boxes=labels=0;draw_hud(nullptr,nullptr,nullptr,nullptr);assert(!boxes&&!labels);nativeWindow=0;
    s_hudEditorWindow=0;window=hub=false;s_run.phase=Phase::Running;
    // Only live, initialized elites appear. Prefer the nearest initially and
    // keep it stable as another elite walks closer; timed bosses take priority.
    assert(!boss_hud_target());
    s_enemies[0].id=0;s_enemies[0].elite=true;s_enemies[0].dormant=true;
    assert(!boss_hud_target());s_enemies[0].dormant=false;
    assert(boss_hud_target()==&s_enemies[0]);
    s_enemies[1]=s_enemies[0];s_enemies[1].id=1;
    actors[0].current.pos.x=100;assert(boss_hud_target()==&s_enemies[0]);
    s_enemies[1].eliteSlot=0;s_enemies[1].species=unsigned(Species::Darknut);
    actors[1].health=25;assert(boss_hud_target()==&s_enemies[1]);
    hasFont=true;s_run.phase=Phase::Running;bossBars=rubyLabels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert(bossBars==1&&bossPercentage==25&&rubyLabels==4&&!textureCoordinates);
    assert(!std::strcmp(boss_hud_name(s_enemies[1].species),"Darknut"));
    for(const auto& roster:map_rosters)for(auto species:roster.elites)
        assert(std::strcmp(boss_hud_name(unsigned(species)),"Elite"));
    // Missing meter/font is safe; a missing font does not suppress the HP bar.
    hasFont=false;assert(draw_boss_hud(&meter,304));
    barAvailable=false;assert(!draw_boss_hud(&meter,304));barAvailable=true;
    for(int reason=0;reason<7;++reason){
        s_enemies[1].initialized=reason!=0;s_enemies[1].retiring=reason==1;
        s_enemies[1].credited=reason==2;s_enemies[1].id=reason==3?99:1;
        actors[1].health=reason==4?0:100;actors[1].field_0x560=reason==5?0:100;
        s_enemies[1].elite=reason!=6;
        assert(boss_hud_target()==&s_enemies[0]);
    }
    s_enemies[1]={};actors[0].health=0;assert(!boss_hud_target()&&s_bossHudTarget==kNone);
    actors[0].health=100;hasPlayer=false;assert(!boss_hud_target());hasPlayer=true;
    for(int reason=0;reason<6;++reason){
        arena=reason!=0;transition=reason==1;window=reason==2;hasGraf=reason!=3;
        s_run.phase=reason==4?Phase::Loading:reason==5?Phase::Ready:Phase::Running;boxes=labels=0;
        draw_hud(nullptr,nullptr,nullptr,nullptr);assert(boxes==0 && labels==0);
    }
    arena=hasGraf=hasFont=true;transition=window=false;s_hudEditorWindow=1;
    hudValues[unsigned(HudPart::BossBar)]={20,200,150};
    hudValues[unsigned(HudPart::Rank)]={-100,170,200};
    draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert((bossGeometry==std::array<float,4>{84,200,480,36}));
    assert((skullGeometry==std::array<float,4>{140,170,40,40}));
    // Every requested element uses its own anchor and uniform scale in production drawing.
    s_run.start(4);s_run.phase=Phase::Ready;
    for(unsigned i=0;i<hudValues.size();++i)hudValues[i]={10+20*i,200+10*i,100+10*i};
    vertices.clear();textPositions.clear();draw_hud(nullptr,nullptr,nullptr,nullptr);
    const auto near=[](float a,float b){assert(std::abs(a-b)<.001f);};
    const auto checkText=[&](HudPart part,const char* prefix,float size,float offset=0.f,float xOffset=0.f){
        const auto p=hud_position(part,304);
        auto found=std::find_if(textPositions.rbegin(),textPositions.rend(),[&](const auto& t){return t.text.starts_with(prefix);});
        assert(found!=textPositions.rend());near(found->x,p.x+xOffset*p.scale);near(found->y,p.y+offset*p.scale);near(found->size,size*p.scale);
    };
    checkText(HudPart::Timer,"00:00",18);checkText(HudPart::Rank,"5",18,0,10.5f);
    checkText(HudPart::Level,"Level",14);checkText(HudPart::Score,"Score",14);checkText(HudPart::ExpText,"XP",14);
    checkText(HudPart::Info,"Next chest:",14,54,-6.5f);checkText(HudPart::BossName,"Darknut",20);
    const auto xpPos=hud_position(HudPart::ExpBar,304);
    assert(vertices.size()==16);near(vertices[8][0],xpPos.x-160*xpPos.scale);near(vertices[8][1],xpPos.y);
    near(vertices[10][0],xpPos.x+160*xpPos.scale);near(vertices[10][1],xpPos.y+6*xpPos.scale);
    const auto banner=hud_position(HudPart::ItemBanner,304);
    near(vertices[0][0],banner.x-140*banner.scale);near(vertices[0][1],banner.y-53*banner.scale);
    near(vertices[2][0],banner.x+140*banner.scale);near(vertices[2][1],banner.y+53*banner.scale);
    near(gearGeometry[2],44*banner.scale);
    checkText(HudPart::ItemBanner,"Hero's Sword",13,-28,-76);
    checkText(HudPart::ItemBanner,"Call Midna to Equip",11,58);
    // Inline icon keeps its native 1:2 aspect and shares Info text transforms.
    const auto info=hud_position(HudPart::Info,304);
    near(rupeeGeometry[2],9*info.scale);near(rupeeGeometry[3],18*info.scale);
    near(rupeeGeometry[1],info.y+52*info.scale);
    assert(rupeeIcons>0&&!textureCoordinates);
    auto countBefore=rupeeIcons;keys=2;printed.clear();draw_next_chest(&meter,304,0);
    assert(rupeeIcons==countBefore&&printed.back()=="Next chest: 1 key  (Keys: 2)");keys=0;
    for(unsigned reason=0;reason<3;++reason){
        hasRupeePane=reason!=0;hasRupeeTexture=reason!=1;
        draw_next_chest(reason==2?nullptr:&meter,304,0);assert(rupeeIcons==countBefore);
    }
    hasRupeePane=hasRupeeTexture=true;draw_next_chest(&meter,304,0);assert(rupeeIcons==countBefore+1);
    for(bool save:{false,true}){nativeWindow=save?0:3;saving=save;countBefore=rupeeIcons;draw_hud(nullptr,nullptr,nullptr,nullptr);assert(rupeeIcons==countBefore);}
    saving=false;nativeWindow=0;
    hudValues=hud_defaults;
    std::cout<<"Survivors HUD: native font/box vertex packets, messages and render gates passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(prefix + stubs + '\nunsigned s_gearNoticeTicks=0;const char* s_gearNotice="Weapon acquired";GearItem s_gearNoticeItem{};bool gear_notice_equip_available(){return s_gearNoticeTicks&&s_gearNoticeItem.kind!=GearKind::Empty;}\n' + '\nbool hub=false;unsigned hubLabels=0,prices=0,feedback=0;bool survivors_hub_active(){return hub;} void draw_hub_labels(){++hubLabels;} void draw_chest_prices(){++prices;} void draw_combat_feedback(){++feedback;}\n' + native + '\n' + function(source, 'void label(') + '\n' +
                   (root/'src/survivors_boss_hud.inc').read_text() + '\n' + (root/'src/survivors_chest_hud.inc').read_text() + '\n' + (root/'src/survivors_gear_notice.inc').read_text() + '\n' + function(source, 'void draw_hud(') + '\n' + main)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
