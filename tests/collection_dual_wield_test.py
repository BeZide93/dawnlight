"""Production collection callbacks with consumable input and per-save storage."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/collection_dual_wield.cpp").read_text()


def function(name):
    import re
    start = re.search(r"^[\w* ]+ " + name + r"\([^;]*?\) \{", source, re.M).start()
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


fixture = r'''
#include "collection_dual_wield_state.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using namespace dawnlight;
using u8=uint8_t; using u16=uint16_t;
struct ModContext {};
ModContext* mod_ctx=nullptr;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
constexpr int MOD_OK=0,dItemNo_NONE_e=255,PAD_1=0;
using ConfigVarHandle=int;
struct ConfigVarValue {bool bool_value;};
bool enabled=true,boss=false,pressA=false,pageLeft=false,pageRight=false;
bool dual_wield_enabled(){return enabled;}
bool save_state_boss_rush_active(){return boss;}
bool dMw_A_TRIGGER(){return pressA;}
struct mDoCPd_c {
    static bool getTrigL(int){return pageLeft;}
    static bool getTrigR(int){return pageRight;}
};
// Like the host, directional queries consume the repeat state. Rollback must
// preserve both timer state and whether checkTrigger has advanced this frame.
struct STControl {
    int direction=-1,ticks=0,delay=0;
    void checkTrigger(){++ticks;if(delay>0)--delay;}
    bool consume(int d){if(direction!=d||delay)return false;delay=3;return true;}
    bool checkLeftTrigger(){return consume(0);}
    bool checkRightTrigger(){return consume(1);}
    bool checkUpTrigger(){return consume(2);}
    bool checkDownTrigger(){return consume(3);}
};
struct CPaneMgr {
    float left,top,width,height,x=0,y=0;
    void translate(float a,float b){x=a;y=b;}
};
struct J2DScreen {};
struct dSelect_cursor_c {
    CPaneMgr* mpPaneMgr=nullptr;J2DScreen* mpScreen=nullptr;
    int artwork=91;float pulse=.73f;
};
struct dMenu_Collect2D_c {
    J2DScreen* mpScreen=nullptr;
    u8 mCursorX=4,mCursorY=1,field_0x259=0,field_0x25a=0;
    bool mIsWolf=false;
    int field_0x22d[7][6]{};
    CPaneMgr panes[7][6]{};CPaneMgr* mpSelPm[7][6]{};
    STControl stick;STControl* mpStick=&stick;
    dSelect_cursor_c* mpDrawCursor=nullptr;
    int restored=0,cursorUpdates=0;
    void setItemNameString(int,int){++restored;}
    void cursorPosSet(){++cursorUpdates;}
    void changeShield();
};
struct JAISoundID {int id;bool operator!=(int other) const{return id!=other;}};
namespace mods {template<class T>T arg(void* p,int){
    if constexpr(std::is_same_v<T,JAISoundID>)return *static_cast<JAISoundID*>(p);
    else return static_cast<T>(p);
}}
struct Player {
    int timer=0,changes=0;
    int getShieldChangeWaitTimer(){return timer;}
    void setShieldChange(){++changes;}
} player;
auto* daAlink_getAlinkActorClass(){return &player;}
int shield=42,vibrations=0;
u8 dComIfGs_getSelectEquipShield(){return shield;}
void dMeter2Info_setShield(u8 id,bool remove){assert(!remove);shield=id;}
void dMeter2Info_set2DVibration(){++vibrations;}
constexpr int Z2SE_SY_CURSOR_ITEM=1,Z2SE_SY_CURSOR_OPTION=2,Z2SE_SY_ITEM_SET_X=3,Z2SE_SY_ITEM_COMBINE_OFF=4;
std::vector<int> sounds;
HookAction before_equip_sound(ModContext*,void*,void*,void*);
void mDoAud_seStart(int id,void*,int,int){
    JAISoundID sound{id};bool result=true;
    if(before_equip_sound(nullptr,&sound,&result,nullptr)==HOOK_CONTINUE)sounds.push_back(id);
}
struct Save {
    int slot=0;
    std::map<std::pair<int,std::string>,std::vector<unsigned char>> blobs;
    int get_blob(ModContext*,const char* key,void* out,size_t* size){
        auto it=blobs.find({slot,key});if(it==blobs.end())return -1;
        auto limit=*size;*size=it->second.size();
        if(limit<*size)return -1;
        std::memcpy(out,it->second.data(),*size);return MOD_OK;
    }
    int set_blob(ModContext*,const char* key,const void* data,size_t size){
        const auto* p=static_cast<const unsigned char*>(data);
        blobs[{slot,key}]={p,p+size};return MOD_OK;
    }
} saves;
auto* svc_save=&saves;
struct Log {void warn(ModContext*,const char*){}} logger;
auto* svc_log=&logger;
collection::Selection s_selection;
bool s_changingToSword=false;
struct Menu {
    dMenu_Collect2D_c* owner=nullptr;
    bool focused=false,visible=true,drawing=false,drawn=false;
    u8 anchorX=0,anchorY=0;
    std::vector<collection::Cell> cells;
    collection::Placement placement;
    CPaneMgr iconBounds{};CPaneMgr* iconPane=&iconBounds;
    void restore_tints(){drawing=drawn=false;}
} s_menu;
void layout(dMenu_Collect2D_c*){}
int equipmentPresentations=0;
void present_equipment(dMenu_Collect2D_c*){++equipmentPresentations;}
void show_name(dMenu_Collect2D_c*){}
int context=-1,hover=-1;
float pointerX=-999,pointerY=-999;bool pendingClick=false;
struct Pointer {
    void (*begin)(int)=[](int c){context=c;};
    bool (*hit)(CPaneMgr*,float)=[](CPaneMgr* pane,float pad){
        return pane && pointerX>=pane->left-pad&&pointerX<=pane->left+pane->width+pad&&
            pointerY>=pane->top-pad&&pointerY<=pane->top+pane->height+pad;
    };
    void (*hover)(u16)=[](u16 id){::hover=id;};
    bool (*click)()=[](){bool out=pendingClick;pendingClick=false;return out;};
} s_pointer;
void draw_icon();
// PRODUCTION
int shieldChanges=0,customShield=-1,savedCustomShield=-1,customSword=5,customTunic=7;
bool essentials=false;
void dMenu_Collect2D_c::changeShield(){
    ++shieldChanges;
    assert(mCursorX==3&&mCursorY==1); // explicit native cleanup, never custom neighbor
    before_shield(nullptr,this,nullptr,nullptr);
    if(essentials){
        // Essentials' changeShield hook clears and saves its custom shield,
        // then selects/toggles the native backing item and plays a sound.
        const bool wasCustom=customShield>=0;
        customShield=savedCustomShield=-1;
        shield=(!wasCustom&&shield==42)?dItemNo_NONE_e:42;
        mDoAud_seStart(shield==dItemNo_NONE_e?Z2SE_SY_ITEM_COMBINE_OFF:Z2SE_SY_ITEM_SET_X,nullptr,0,0);
    }else{
        shield=42;
        mDoAud_seStart(Z2SE_SY_ITEM_SET_X,nullptr,0,0);
    }
    player.setShieldChange();
}
J2DScreen iconScreen;
int iconDraws=0;
void draw_icon(){
    if(!s_menu.visible||s_menu.drawn)return;
    // Our own screen draw re-enters the global hook. It must pass through.
    assert(before_cursor_screen_draw(nullptr,&iconScreen,nullptr,nullptr)==HOOK_CONTINUE);
    ++iconDraws;s_menu.drawn=true;
}
int main(){
    // Native slots, remapped Essentials indices, then the HD HUD shield row.
    // Hylian's unavailable reserved position must still precede our sword.
    for(auto cells:std::vector<std::vector<collection::Cell>>{
        {{3,1,100,80,44,44,true},{4,1,160,80,44,44,true}},
        {{3,1,100,80,44,44,true},{4,1,160,80,44,44,true},
         {5,1,220,80,44,44,true},{2,1,280,80,44,44,true},{1,1,340,80,44,44,true}},
        {{3,1,237,122,46,46,true},{4,1,297,122,46,46,true},
         {3,0,455,122,46,46,true},{4,2,315,57,46,46,true}},
        {{3,1,100,80,44,44,true},{-1,1,160,80,44,44,false}}
    }){
        s_selection.chosen=false;s_menu.cells=cells;s_menu.placement=collection::append_shield(cells);
        auto p=s_menu.placement;
        for(const auto& c:cells)if(c.y==1)assert(p.left>c.left+c.width);
        int last=-1;float right=-1;
        for(size_t i=0;i<cells.size();++i)if(cells[i].y==1&&cells[i].available&&cells[i].left>right){last=int(i);right=cells[i].left;}
        dMenu_Collect2D_c menu;s_menu.owner=&menu;s_menu.visible=true;s_menu.focused=false;
        menu.mCursorX=cells[last].x;menu.stick.direction=1;
        s_menu.iconBounds={p.left,p.top,p.size,p.size};
        for(auto c:cells)if(c.x>=0){
            menu.panes[c.x][c.y]={c.left,c.top,c.width,c.height};
            menu.mpSelPm[c.x][c.y]=&menu.panes[c.x][c.y];
        }
        // Entry takes precedence over an HD/Essentials navigator that would skip
        // native navigation; no second query may consume the same trigger.
        assert(before_navigate(nullptr,&menu,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(own_focus(&menu)&&menu.stick.ticks==1&&menu.stick.delay==3);
        assert(sounds.back()==Z2SE_SY_CURSOR_ITEM);
        const auto soundCount=sounds.size();focus(&menu);assert(sounds.size()==soundCount);
        menu.stick={0,0,0};
        assert(before_navigate(nullptr,&menu,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(!s_menu.focused&&menu.mCursorX==cells[last].x&&menu.mCursorY==1);
        // A non-owned direction remains consumable by the following mod.
        menu.stick={2,0,0};
        assert(before_navigate(nullptr,&menu,nullptr,nullptr)==HOOK_CONTINUE);
        assert(menu.stick.ticks==0&&menu.stick.delay==0);
        menu.stick.checkTrigger();assert(menu.stick.checkUpTrigger());
        // Held-right repeat delay must advance once, not twice.
        menu.stick={1,0,3};
        assert(before_navigate(nullptr,&menu,nullptr,nullptr)==HOOK_CONTINUE);
        assert(menu.stick.delay==3);menu.stick.checkTrigger();assert(menu.stick.delay==2);
        // Pointer: own hit consumes only its click; foreign hit passes through.
        pointerX=p.left+p.size*.5f;pointerY=p.top+p.size*.5f;pendingClick=true;
        bool result=false;
        assert(before_pointer(nullptr,&menu,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result&&!pendingClick&&own_focus(&menu)&&context==4&&hover==0xda01);
        assert(sounds.back()==Z2SE_SY_ITEM_SET_X);
        assert(dual_wield_equipped());
        pointerX=cells[last].left+cells[last].width*.5f;pendingClick=true;
        assert(before_pointer(nullptr,&menu,&result,nullptr)==HOOK_CONTINUE);
        assert(!s_menu.focused&&pendingClick);
        // Shield actions and unavailable/wolf/timer-blocked selections.
        menu.field_0x22d[menu.mCursorX][1]=1;player.timer=0;
        menu.mIsWolf=true;before_click(nullptr,&menu,nullptr,nullptr);assert(dual_wield_equipped());
        menu.mIsWolf=false;player.timer=2;before_click(nullptr,&menu,nullptr,nullptr);assert(dual_wield_equipped());
        player.timer=0;assert(before_click(nullptr,&menu,nullptr,nullptr)==HOOK_CONTINUE);assert(!dual_wield_equipped());
        focus(&menu);pressA=true;
        assert(before_wait(nullptr,&menu,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);assert(dual_wield_equipped());
        // Shoulder input must reach the page owner, even alongside A.
        pageRight=true;
        assert(before_wait(nullptr,&menu,nullptr,nullptr)==HOOK_CONTINUE&&!s_menu.focused);
        pageRight=pressA=false;
        // A stale anchor never hijacks another mod's selection.
        focus(&menu);++menu.mCursorY;assert(!own_focus(&menu));
        s_menu.owner=nullptr;
    }
    // Selection is opt-in, scoped to both slot and mode, and tolerant of absent,
    // corrupt, oversized or future-version metadata.
    saves.blobs.clear();saves.slot=0;boss=false;load_selection(nullptr,0,nullptr);
    assert(!dual_wield_equipped());select_sword(true);assert(dual_wield_equipped());
    saves.slot=1;load_selection(nullptr,1,nullptr);assert(!dual_wield_equipped());
    saves.slot=0;load_selection(nullptr,0,nullptr);assert(dual_wield_equipped());
    boss=true;assert(!dual_wield_equipped());load_selection(nullptr,0,nullptr);assert(!dual_wield_equipped());
    select_sword(true);boss=false;assert(!dual_wield_equipped());
    load_selection(nullptr,0,nullptr);assert(dual_wield_equipped());
    enabled=false;assert(!dual_wield_equipped());ConfigVarValue off{false};
    setting_changed(nullptr,0,&off,nullptr,nullptr);enabled=true;assert(!dual_wield_equipped());
    for(auto data:std::vector<std::vector<unsigned char>>{{},{1},{1,1,1},{2,1},{1,2}}){
        saves.blobs[{0,"dual-wield"}]=data;load_selection(nullptr,0,nullptr);assert(!dual_wield_equipped());
    }
    // Feature disabled: navigation and foreign input remain untouched.
    dMenu_Collect2D_c menu;s_menu.owner=&menu;s_menu.visible=false;
    assert(before_navigate(nullptr,&menu,nullptr,nullptr)==HOOK_CONTINUE&&menu.stick.ticks==0);
    // Reproduce the full draw sequence: native/HD code restores the shield
    // target *after* earlier positioning, then renders the cursor screen.
    // The final placement cannot depend on update() calling a hooked entry.
    CPaneMgr cursorRoot{};J2DScreen cursorScreen,foreignScreen;
    dSelect_cursor_c cursor{&cursorRoot,&cursorScreen};
    menu.mpDrawCursor=&cursor;s_menu.visible=true;s_menu.focused=true;
    s_menu.anchorX=menu.mCursorX;s_menu.anchorY=menu.mCursorY;
    s_menu.placement={200,100,40,0};
    J2DScreen menuScreen;menu.mpScreen=&menuScreen;s_menu.drawing=true;
    before_cursor_screen_draw(nullptr,&menuScreen,nullptr,nullptr);
    assert(s_menu.drawing&&equipmentPresentations==1&&!s_menu.drawn);
    for(int frame=0;frame<4;++frame){
        s_menu.drawing=true;s_menu.drawn=false;
        cursorRoot.translate(220,120); // an earlier correction is not enough
        cursorRoot.translate(160,120); // native draw/update or foreign alignment
        assert(before_cursor_screen_draw(nullptr,&foreignScreen,nullptr,nullptr)==HOOK_CONTINUE);
        assert(cursorRoot.x==160&&iconDraws==frame);
        assert(before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr)==HOOK_CONTINUE);
        // These are the values consumed by the actual cursor screen renderer.
        assert(cursorRoot.x==220&&cursorRoot.y==120&&iconDraws==frame+1);
        assert(cursor.artwork==91&&cursor.pulse==.73f&&menu.mCursorX==s_menu.anchorX);
        before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
        assert(iconDraws==frame+1); // one icon, even with another render pass
    }
    // Leaving the virtual cell restores ordinary targeting on the next draw.
    s_menu.focused=false;cursorRoot.translate(10,30);
    before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
    assert(cursorRoot.x==10&&cursorRoot.y==30);
    s_menu.focused=true;s_menu.drawing=false;
    before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
    assert(cursorRoot.x==10&&cursorRoot.y==30);
    s_menu.drawing=true;++menu.mCursorY; // stale anchor / another mod's page
    before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
    assert(cursorRoot.x==10&&cursorRoot.y==30);
    --menu.mCursorY;s_menu.visible=false;
    before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
    assert(cursorRoot.x==10&&cursorRoot.y==30);
    s_menu.owner=nullptr;
    before_cursor_screen_draw(nullptr,&cursorScreen,nullptr,nullptr);
    // Equipment ownership is exclusive, including the custom mod's persisted
    // state. Test native, custom, and a native shield that toggles off on reuse.
    s_menu.owner=&menu;s_menu.visible=true;menu.mCursorX=2;menu.mCursorY=1;
    s_menu.anchorX=2;s_menu.anchorY=1;shieldChanges=0;
    for(int variant=0;variant<3;++variant){
        essentials=variant!=0;
        customShield=savedCustomShield=variant==1?9:-1;
        shield=variant==1?43:42;s_selection.chosen=false;player.timer=0;
        const auto count=sounds.size();const int backing=shield;
        activate(&menu);
        assert(dual_wield_equipped()&&customShield==-1&&savedCustomShield==-1);
        assert(customSword==5&&customTunic==7&&shield==backing);
        assert(menu.mCursorX==2&&menu.mCursorY==1&&own_focus(&menu));
        assert(!s_changingToSword&&sounds.size()==count+1&&sounds.back()==Z2SE_SY_ITEM_SET_X);
        assert(shieldChanges==variant+1);
        activate(&menu);assert(shieldChanges==variant+1&&sounds.size()==count+1);
        // Selecting a foreign shield afterwards leaves Dual Wield normally.
        before_shield(nullptr,&menu,nullptr,nullptr);assert(!dual_wield_equipped());
    }
    customShield=savedCustomShield=9;
    const auto soundCount=sounds.size();const int changes=shieldChanges;
    menu.mIsWolf=true;activate(&menu);menu.mIsWolf=false;
    player.timer=2;activate(&menu);player.timer=0;
    shield=dItemNo_NONE_e;activate(&menu);
    assert(!dual_wield_equipped()&&customShield==9&&savedCustomShield==9);
    assert(shieldChanges==changes&&sounds.size()==soundCount);
    // Only the internal equip/unequip feedback is suppressed, never other SFX.
    s_changingToSword=true;JAISoundID unrelated{99};bool played=true;
    assert(before_equip_sound(nullptr,&unrelated,&played,nullptr)==HOOK_CONTINUE&&played);
    JAISoundID equip{Z2SE_SY_ITEM_SET_X};
    assert(before_equip_sound(nullptr,&equip,&played,nullptr)==HOOK_SKIP_ORIGINAL&&!played);
    s_changingToSword=false;
    assert(before_equip_sound(nullptr,&equip,&played,nullptr)==HOOK_CONTINUE);
}
'''

names = ["restore_name", "blob_name", "load_selection", "select_sword", "setting_changed",
         "dual_wield_equipped", "own_focus", "focus", "can_equip", "clear_previous_shield", "activate",
         "before_shield", "before_equip_sound",
         "before_wait", "before_navigate", "before_pointer", "before_click", "before_cursor_screen_draw"]
fixture = fixture.replace("// PRODUCTION", "\n".join(function(name) for name in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / "test.cpp", Path(tmp) / "test"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-I" + str(root / "src"),
                    str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Collection Dual Wield passed: layouts, input, pointer, pages, save isolation, exclusive shield handoff and equip sound")

# Exercise render-only tint ownership separately from input. The screen's
# original colors must be restored for foreign equip/unequip handlers.
presentation = r'''
#include "collection_dual_wield_state.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace dawnlight;
namespace JUtility {
struct TColor {int r,g,b,a;bool operator==(const TColor&)const=default;};
}
using JUtility::TColor;
struct ResTIMG {} artwork,ornamentArt;
constexpr uint64_t tag(const char* s){uint64_t n=0;while(*s)n=(n<<8)|*s++;return n;}
#define MULTI_CHAR(x) tag(#x+1)>>8
using J2DMirror=int;
constexpr int BIND15=15;
struct J2DPane {
    uint64_t mInfoTag=0;bool shown=true;
    J2DPane* parent=nullptr;J2DPane* child=nullptr;J2DPane* next=nullptr;
    collection::Cell rect{};
    virtual int getTypeID(){return 0;}
    J2DPane* getParentPane(){return parent;}
    J2DPane* getFirstChildPane(){return child;}
    J2DPane* getNextChildPane(){return next;}
    bool isVisible(){return shown;}
    void show(){shown=true;}void hide(){shown=false;}
    void move(float x,float y){rect.left=x;rect.top=y;}
    void resize(float w,float h){rect.width=w;rect.height=h;}
};
struct J2DPicture: J2DPane {
    TColor white{255,255,0,255},black{0,0,0,0};
    ResTIMG const* art=&artwork;int mirror=0,alpha=255;
    int getTypeID() override{return 18;}
    TColor getWhite(){return white;}TColor getBlack(){return black;}
    void setWhite(TColor v){white=v;}
    void setBlackWhite(TColor a,TColor b){black=a;white=b;}
    void changeTexture(ResTIMG const* a,int){art=a;}
    void* getTexture(int){return this;}
    void setTexCoord(void*,int binding,int m,bool tumble){assert(binding==15&&!tumble);mirror=m;}
    TColor corner(int){return {255,255,255,255};}
    void setCornerColor(TColor,TColor,TColor,TColor){}
    int getAlpha(){return alpha;}void setAlpha(int a){alpha=a;}
};
struct dMenu_Collect2D_c {J2DPane* mpScreen=nullptr;};
J2DPicture hylian,foreignFrame,ownFrame,ownTL,ownBR;
J2DPicture* picture(J2DPane*,uint64_t){return &hylian;}
const ResTIMG* texture(J2DPicture* p){return p->art;}
void shield_frames(dMenu_Collect2D_c*,J2DPane*,const ResTIMG*,std::vector<J2DPicture*>& out){out={&hylian,&foreignFrame};}
collection::Cell bounds(dMenu_Collect2D_c*,J2DPane* p,int,int,bool){return p->rect;}
bool equipped=true;
bool dual_wield_equipped(){return equipped;}
struct Menu {
    bool visible=true,drawing=true,drawn=false;
    J2DPicture* frame=&ownFrame;
    J2DPicture* flourishes[2]{&ownTL,&ownBR};
    collection::Placement placement{200,100,46,0};
    struct FrameTint {J2DPicture* pane;TColor color;};
    std::vector<FrameTint> frameTints;
    struct DecorationVisibility {J2DPane* pane;bool shown;};
    std::vector<DecorationVisibility> decorations;
    // RESTORE
} s_menu;
// PRESENT
int main(){
    J2DPane root;dMenu_Collect2D_c menu{&root};
    J2DPicture tl,br,otherRow;
    root.child=&tl;tl.next=&br;br.next=&otherRow;
    for(auto* p:{&tl,&br,&otherRow}){p->parent=&root;p->mInfoTag=MULTI_CHAR('hd_cef00');p->art=&ornamentArt;}
    hylian.rect={0,1,37,97,52,52,true};foreignFrame.rect={1,1,97,97,52,52,true};
    tl.rect={0,0,88,88,24,24,false};br.rect={0,0,128,128,24,24,false};
    otherRow.rect={0,0,88,28,24,24,false};
    const TColor high{246,244,198,255},low{132,134,104,255};
    tl.white=br.white=high;tl.alpha=br.alpha=173;
    for(bool sourceShown:{false,true}){
        hylian.white=low;foreignFrame.white=high;s_menu.drawing=true;
        tl.shown=br.shown=sourceShown;ownTL.hide();ownBR.hide();
        present_equipment(&menu);
        assert(ownFrame.white==high&&foreignFrame.white==low&&hylian.white==low);
        assert(s_menu.frameTints.size()==2);
        // The regression: hidden source ornaments must still produce our pair.
        assert(ownTL.shown&&ownBR.shown&&!tl.shown&&!br.shown&&otherRow.shown);
        assert(ownTL.art==&ornamentArt&&ownBR.art==&ornamentArt);
        assert(ownTL.mirror==0&&ownBR.mirror==3&&ownTL.white==high&&ownBR.alpha==173);
        assert(ownTL.rect.left==188&&ownTL.rect.top==88&&ownTL.rect.width==24);
        assert(ownBR.rect.left==228&&ownBR.rect.top==128);
        assert(s_menu.decorations.size()==2&&tl.rect.left==88&&br.rect.left==128);
        s_menu.restore_tints();
        assert(hylian.white==low&&foreignFrame.white==high);
        assert(tl.shown==sourceShown&&br.shown==sourceShown&&s_menu.decorations.empty());
        assert(!ownTL.shown&&!ownBR.shown);
        assert(s_menu.frameTints.empty()&&!s_menu.drawing);
        equipped=false;s_menu.drawing=true;present_equipment(&menu);
        assert(ownFrame.white==low&&foreignFrame.white==high&&s_menu.frameTints.empty());
        assert(!ownTL.shown&&!ownBR.shown&&s_menu.decorations.empty());
        equipped=true;
    }
    // HD scaling is inherited from the source frame and each ornament's bounds.
    s_menu.placement={200,100,98,0};s_menu.drawing=true;present_equipment(&menu);
    assert(ownTL.rect.left==179&&ownTL.rect.top==79&&ownTL.rect.width==48);
    assert(ownBR.rect.left==259&&ownBR.rect.top==159);
    s_menu.restore_tints();
    // Missing HD artwork / a hidden page leaves a plain frame, never stale art.
    root.hide();s_menu.drawing=true;present_equipment(&menu);
    assert(!ownTL.shown&&!ownBR.shown&&s_menu.decorations.empty());
    s_menu.restore_tints();root.show();root.child=nullptr;
    s_menu.drawing=true;present_equipment(&menu);
    assert(!ownTL.shown&&!ownBR.shown);
    assert(collection::flourish_corner(hylian.rect,tl.rect)==-1);
}
'''
presentation = presentation.replace("// RESTORE", function("restore_tints"))
presentation = presentation.replace("// PRESENT", "\n".join(function(n) for n in
    ["visible", "transfer_equipped_flourishes", "present_equipment"]))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / "presentation.cpp", Path(tmp) / "presentation"
    cpp.write_text(presentation)
    subprocess.run(["c++", "-std=c++20", "-I" + str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Collection presentation passed: native/HD equipped tint and foreign frame/decoration restoration")

# Release optimization can remove seemingly available source-level helpers.
# Verify the real Android host when supplied, without requiring game archives.
import os
if apk := os.environ.get("DUSKLIGHT_APK"):
    import ctypes
    import ctypes.util
    import re
    import struct
    import zipfile
    with zipfile.ZipFile(apk) as archive:
        binary = archive.read("lib/arm64-v8a/libmain.so")
    offset = binary.index(b"SYMGEN\0\0")
    _, version, compression, ulen, clen, bidlen, bid, count = struct.unpack_from("<8sIIQQI32sI", binary, offset)
    assert version == 2 and compression in (0, 1) and bidlen <= 32
    payload = binary[offset + 72:offset + 72 + clen]
    if compression:
        zstd = ctypes.CDLL(ctypes.util.find_library("zstd"))
        zstd.ZSTD_decompress.restype = ctypes.c_size_t
        zstd.ZSTD_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        output = ctypes.create_string_buffer(ulen)
        assert zstd.ZSTD_decompress(output, ulen, payload, clen) == ulen
        payload = output.raw
    strings = payload[count * 24:]
    entries = {}
    for _, rva, name_offset, flags in struct.iter_unpack("<QQII", payload[:count * 24]):
        name = strings[name_offset:strings.index(0, name_offset)].decode()
        entries[name] = (rva, flags)
    required = re.findall(r"DEFINE_HOOK\(&([^,]+),", source)
    # The MSVC fallback is an alternative for Windows, not an Android dependency.
    required += [name for name in re.findall(r'resolve\("([^"]+)"', source) if not name.startswith("?")]
    for name in required:
        assert name in entries, f"Missing host symbol: {name}"
        rva, flags = entries[name]
        assert rva and flags & 1 and not flags & 16, f"Not uniquely hookable: {name}"
    print(f"Android host contract passed: {len(required)} collection symbols, build {bid[:bidlen].hex()}")
