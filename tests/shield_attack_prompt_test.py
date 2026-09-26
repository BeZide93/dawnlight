"""Exercise production Shield Attack prompt styling at the screen draw boundary."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/manual_shield_hooks.cpp").read_text()


def function(name):
    start = re.search(r"^[\w:* ]+ " + name + r"\([^;]*?\) \{", source, re.M).start()
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


fixture = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
using u8=uint8_t;using u64=uint64_t;using f32=float;
constexpr u64 tag(const char* s){u64 n=0;while(*s)n=(n<<8)|*s++;return n;}
#define MULTI_CHAR(x) (tag(#x+1)>>8)
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {template<class T>T arg(void* args,int){return static_cast<T>(args);}}
namespace JUtility {struct TColor {int r=0,g=0,b=0,a=0;bool operator==(const TColor&)const=default;};}
struct ResTIMG {} stockA,stockB,hdA,hdB;
struct JUTTexture {const ResTIMG* info=nullptr;auto getTexInfo(){return info;}};
struct J2DPane {
    bool shown=true;int kind=0;J2DPane* child=nullptr;J2DPane* next=nullptr;
    int getTypeID(){return kind;}bool isVisible(){return shown;}
    void show(){shown=true;}void hide(){shown=false;}
    J2DPane* getFirstChildPane(){return child;}J2DPane* getNextChildPane(){return next;}
};
constexpr int BIND15=15,MIRROR0=0;
struct J2DPicture:J2DPane {
    JUTTexture texture;JUtility::TColor black{1,2,3,4},white{5,6,7,8};
    float x=10,y=20,w=30,h=40;
    J2DPicture(){kind=18;}
    JUTTexture* getTexture(int){return &texture;}
    void changeTexture(const ResTIMG* t,int){texture.info=t;}
    auto getBlack(){return black;}auto getWhite(){return white;}
    void setBlackWhite(JUtility::TColor b,JUtility::TColor v){black=b;white=v;}
    void resize(float a,float b){w=a;h=b;}void move(float a,float b){x=a;y=b;}
    void setTexCoord(JUTTexture*,int,int,bool){}
    struct Bounds {struct {float x,y;} i;float w,h;float getWidth()const{return w;}float getHeight()const{return h;}};
    Bounds getBounds(){return {{x,y},w,h};}
};
struct J2DScreen {std::map<u64,J2DPane*> panes;J2DPane* search(u64 t){return panes[t];}};
struct CPaneMgr {J2DPane* pane;J2DPane* getPanePtr(){return pane;}};
struct dMeterButton_c {
    enum {BUTTON_B_e=1};int field_0x4be[2]{BUTTON_B_e,0};
    J2DScreen* mpButtonScreen;CPaneMgr* mpButtonB;
};
struct dMeter2Draw_c {J2DScreen* screen;J2DScreen* getMainScreenPtr(){return screen;}};
struct dMeter2_c {dMeter2Draw_c* draw;auto getMeterDrawPtr(){return draw;}};
dMeter2_c* meter=nullptr;auto dMeter2Info_getMeterClass(){return meter;}
constexpr int BUTTON_STATUS_UNK_129=129,BUTTON_STATUS_SHIELD_ATTACK=58;
int status=BUTTON_STATUS_SHIELD_ATTACK;bool enabled=true;
u8 dComIfGp_getAStatus(){return status;}bool manual_shielding_enabled(){return enabled;}
// PRODUCTION
int main(){
    J2DScreen context,hud,unrelated;
    J2DPicture a,b,ha,hb,letter,glow;
    J2DPane group,actionText;actionText.kind=19;
    group.child=&b;b.next=&letter;letter.next=&glow;glow.next=&actionText;
    context.panes={{MULTI_CHAR('a_btn1'),&a},{MULTI_CHAR('b_btn'),&b}};
    hud.panes={{MULTI_CHAR('a_btn'),&ha},{MULTI_CHAR('b_btn'),&hb}};
    CPaneMgr pm{&group};dMeterButton_c buttons{{1,0},&context,&pm};
    dMeter2Draw_c draw{&hud};dMeter2_c live{&draw};meter=&live;
    ha.texture.info=&hdA;hb.texture.info=&hdB;
    for(int action:{BUTTON_STATUS_SHIELD_ATTACK,BUTTON_STATUS_UNK_129})
      for(bool alreadyStyled:{false,true})for(bool hdFirst:{false,true}){
        status=action;enabled=action==BUTTON_STATUS_SHIELD_ATTACK;
        for(int frame=0;frame<3;++frame){
            a.texture.info=&hdA;b.texture.info=alreadyStyled?&hdB:&stockB;
            b.black={1,2,3,4};b.white={5,6,7,8};
            auto hdStyle=[&]{if(alreadyStyled)b.texture.info=&hdB;letter.hide();glow.hide();};
            if(hdFirst)hdStyle();
            before_meter_button_draw(nullptr,&buttons,nullptr,nullptr);
            if(!hdFirst)hdStyle();
            // Native animation/another layout update after the outer pre-hooks.
            letter.show();glow.hide();
            before_attack_prompt_screen_draw(nullptr,&unrelated,nullptr,nullptr);
            assert(letter.shown);
            before_attack_prompt_screen_draw(nullptr,&context,nullptr,nullptr);
            assert(b.texture.info==&hdB&&b.shown&&!letter.shown&&!glow.shown);
            assert(group.shown&&actionText.shown&&b.x==10&&b.y==20&&b.w==30&&b.h==40);
            after_meter_button_draw(nullptr,nullptr,nullptr,nullptr);
            assert(letter.shown&&!glow.shown&&s_drawingAttackButtons==nullptr);
            assert(b.texture.info==(alreadyStyled?&hdB:&stockB));
            assert(b.black==JUtility::TColor(1,2,3,4)&&b.white==JUtility::TColor(5,6,7,8));
            before_attack_prompt_screen_draw(nullptr,&context,nullptr,nullptr);
            assert(letter.shown); // no stale draw owner
        }
    }
    // Vanilla layout, inactive actions and disabled manual shielding pass through.
    for(int scenario:{0,1,2}){
        a.texture.info=scenario==0?&stockA:&hdA;b.texture.info=&stockB;letter.show();
        status=scenario==1?0:BUTTON_STATUS_SHIELD_ATTACK;enabled=scenario!=2;
        before_meter_button_draw(nullptr,&buttons,nullptr,nullptr);
        before_attack_prompt_screen_draw(nullptr,&context,nullptr,nullptr);
        assert(letter.shown&&b.texture.info==&stockB);
        after_meter_button_draw(nullptr,nullptr,nullptr,nullptr);
    }
}
'''
state = source[source.index("struct AttackPromptTextureState"):source.index("J2DPicture* picture_for")]
names = ["picture_for", "picture_texture", "restore_attack_prompt_texture",
         "suppress_attack_prompt_layers", "dawnlight_attack_prompt_active",
         "apply_twilight_hd_attack_prompt", "before_meter_button_draw",
         "before_attack_prompt_screen_draw", "after_meter_button_draw"]
fixture = fixture.replace("// PRODUCTION", state + "\n".join(function(n) for n in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/"prompt.cpp", Path(tmp)/"prompt"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Shield Attack / Flurry Rush prompts: HD layers, draw order, restoration and vanilla passthrough passed")
