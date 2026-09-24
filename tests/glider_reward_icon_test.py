"""Exercise production icon hooks and verify the packaged GX RGBA8 resource."""
from pathlib import Path
import importlib.util
import subprocess
import tempfile
import sys
sys.dont_write_bytecode = True
from PIL import Image
root = Path(__file__).resolve().parents[1]
source = (root/'src/glider_reward_icon.cpp').read_text()
reward = (root/'src/glider_reward.cpp').read_text()
def function(source, signature):
    start = source.index(signature+'(')
    return source[start:source.index('\n}',start)+2]
spec = importlib.util.spec_from_file_location('icon_generator',root/'tools/generate_glider_item_icon.py')
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)
with Image.open(root/'art/glider/glider-item-icon.png') as image:
    assert image.mode == 'RGBA' and image.getchannel('A').getextrema() == (0,255)
    expected = image.resize((128,128),Image.Resampling.LANCZOS)
    pixels = gen.encode(image)
assert pixels == (root/'res/glider-item-icon.rgba8').read_bytes()
assert len(pixels) == 128*128*4
for by in range(0,128,4):
    for bx in range(0,128,4):
        offset=((by//4)*32+bx//4)*64
        for i in range(16):
            a,r = pixels[offset+i*2:offset+i*2+2]
            g,b = pixels[offset+32+i*2:offset+34+i*2]
            assert (r,g,b,a) == expected.getpixel((bx+i%4,by+i//4))
fixture = r'''
#include <cassert>
using f32=float;
struct ModContext{};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct ResTIMG{};
namespace JUtility {
struct TColor{
    int r=0,g=0,b=0,a=0;
    TColor()=default;TColor(int R,int G,int B,int A):r(R),g(G),b(B),a(A){}
    bool operator==(const TColor&) const=default;
};
}
struct JUTTexture{const ResTIMG* info=nullptr;const ResTIMG* getTexInfo(){return info;}};
struct J2DPicture {
    JUTTexture texture;JUtility::TColor black{1,2,3,4},white{5,6,7,8};
    JUTTexture* getTexture(int){return &texture;}
    const ResTIMG* changeTexture(const ResTIMG* t,int){auto* old=texture.info;texture.info=t;return old;}
    auto getBlack(){return black;}auto getWhite(){return white;}
    void setBlackWhite(JUtility::TColor b,JUtility::TColor w){black=b;white=w;}
};
struct dMsgScrnItem_c {
    J2DPicture* mpItemPane[3]{};
    f32 field_0x178=70,field_0x17c=90,field_0x170=48,field_0x174=48;
    int mItemIndex=0x43;bool field_0x19e=true;
};
namespace mods{template<class T>T arg(void* a,int){return static_cast<T>(a);}}
struct {ResTIMG header;} s_icon;
bool s_ready=true;
unsigned s_drawDepth=0;
struct {bool started=false;} s_reward;
struct Message {bool live=true;operator bool() const{return live;}unsigned id(){return 9000;}} s_message;
bool messageObject=true;
unsigned messageId=9000;
void* dMsgObject_getMsgObjectClass(){return messageObject?&s_message:nullptr;}
unsigned dMsgObject_getMessageID(){return messageId;}
// ACTIVE
// STATE
// HOOKS
int main(){
    ResTIMG nativeTexture;
    J2DPicture first,second,third;first.texture.info=&nativeTexture;
    dMsgScrnItem_c screen;screen.mpItemPane[0]=&first;screen.mpItemPane[1]=&second;screen.mpItemPane[2]=&third;
    const auto black=first.black,white=first.white;
    auto restored=[&]{
        assert(first.texture.info==&nativeTexture&&first.black==black&&first.white==white);
        assert(screen.mpItemPane[0]==&first&&screen.mpItemPane[1]==&second&&screen.mpItemPane[2]==&third);
        assert(screen.mItemIndex==0x43&&screen.field_0x178==70&&screen.field_0x17c==90&&screen.field_0x19e);
        assert(!s_draw.screen&&s_drawDepth==0);
    };
    auto draw=[&]{assert(before_icon_draw(nullptr,&screen,nullptr,nullptr)==HOOK_CONTINUE);};
    auto after=[&]{after_icon_draw(nullptr,&screen,nullptr,nullptr);};
    draw();after();restored(); // Normal bow/item dialogs are untouched.
    s_reward.started=true;
    for(int frame=0;frame<3;++frame){
        draw();
        assert(first.texture.info==&s_icon.header&&first.black==JUtility::TColor(0,0,0,0));
        assert(first.white==JUtility::TColor(255,255,255,255));
        assert(screen.mItemIndex==-1&&screen.field_0x178==48&&screen.field_0x17c==48&&!screen.field_0x19e);
        assert(screen.mpItemPane[1]==nullptr&&screen.mpItemPane[2]==nullptr);
        // Nested callbacks cannot restore the outer draw's texture early.
        draw();after();assert(first.texture.info==&s_icon.header&&s_drawDepth==1);
        after();restored();
    }
    messageId=0xa8;draw();after();restored(); // Even during the reward, another message keeps its icon.
    messageId=9000;s_message.live=false;draw();after();restored();s_message.live=true;
    messageObject=false;draw();after();restored();messageObject=true;
    s_ready=false;draw();after();restored();s_ready=true;
    before_icon_draw(nullptr,nullptr,nullptr,nullptr);after();restored();
    screen.mpItemPane[0]=nullptr;draw();after();assert(!s_draw.screen&&s_drawDepth==0);
    screen.mpItemPane[0]=&first;first.texture.info=nullptr;draw();after();
    assert(!s_draw.screen&&s_drawDepth==0&&first.texture.info==nullptr);
}
'''
state=source[source.index('struct IconDrawState'):source.index('HookAction before_icon_draw')]
fixture=fixture.replace('// ACTIVE',function(reward,'bool glider_reward_message_active'))
fixture=fixture.replace('// STATE',state)
fixture=fixture.replace('// HOOKS','\n'.join(function(source,n) for n in ['HookAction before_icon_draw','void after_icon_draw']))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Glider icon: RGBA/alpha round-trip, exact-message scope, native layout, nested draws and restoration passed')
