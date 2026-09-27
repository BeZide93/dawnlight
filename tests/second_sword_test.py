"""Production second-sword resource reload and collection artwork, without game files."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(file, name):
    src = (root/'src'/file).read_text()
    start = re.search(r'^[\w:*<> ,]+ '+name+r'\([^;]*?\) \{',src,re.M).start()
    end=src.index('{',start)+1;depth=1
    while depth:
        depth+=(src[end]=='{')-(src[end]=='}');end+=1
    return src[start:end]

fixture=r'''
#include "dual_wield_sword.hpp"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
using namespace dawnlight;
using u8=uint8_t;using u32=uint32_t;
struct daAlink_c {};
struct J3DModel {std::string name;bool environment;};
struct Heap {
    static Heap root;
    std::vector<J3DModel*> models;
    static Heap* create(void*,u32,Heap*,bool){return new Heap;}
    static Heap* getRootHeap(){return &root;}
    Heap* becomeCurrentHeap(){return &root;}
    void destroy(){for(auto* model:models)delete model;delete this;}
} Heap::root;
using JKRExpHeap=Heap;using JKRHeap=Heap;
struct State {
    daAlink_c* owner=nullptr;Heap* heap=nullptr;std::unique_ptr<u8[]> storage;
    J3DModel *sword=nullptr,*sheath=nullptr;
    SecondSword swordType=SecondSword::Ordon;
    bool seedBlade=false;float guard=0;int pose=0;
} s;
SecondSword selected=SecondSword::Ordon;
SecondSword second_sword(){return selected;}
int forgotten=0,mounts=0;
void forget(J3DModel*){++forgotten;}
auto s_forget=forget;
void model_failure(const char*,const char*){}
struct Log {void info(void*,const char*){}} logService;
auto* svc_log=&logService;void* mod_ctx=nullptr;
std::string mounted,failModel;
bool failMount=false;
struct ResTIMG {int id;};
ResTIMG wood{0},ordon{1},master{2};
bool missingIcon=false;
struct JKRArchive {
    enum {MOUNT_MEM,MOUNT_DIRECTION_HEAD};
    static JKRArchive* mount(const char* path,int,Heap*,int){
        mounted=path;++mounts;static JKRArchive archive;return failMount?nullptr:&archive;
    }
    void unmount(){}
    void* getResource(u32,const char* name){
        if(missingIcon)return nullptr;
        if(std::string(name)=="im_kinobou_48.bti")return &wood;
        if(std::string(name)=="tt_kokirinoken_s3_tc.bti")return &ordon;
        if(std::string(name)=="ni_mastersword_48.bti")return &master;
        assert(false);return nullptr;
    }
} collect;
auto dComIfGp_getCollectResArchive(){return &collect;}
struct J2DPicture {
    const ResTIMG* art=&ordon;bool shown=true;int changes=0;
    void changeTexture(const ResTIMG* p,int){art=p;++changes;}
    void hide(){shown=false;}void show(){shown=true;}
} picture;
struct {J2DPicture* icon=&picture;} s_menu;
const ResTIMG* texture(J2DPicture* p){return p->art;}
J3DModel* copy_model(JKRArchive*,const char* name,bool environment){
    if(name==failModel)return nullptr;
    auto* model=new J3DModel{name,environment};s.heap->models.push_back(model);return model;
}
void release();
// FUNCTIONS
void release(){release_models();s=State{};}
int main(){
    daAlink_c link,other;
    assert(selected==SecondSword::Ordon);
    assert(prepare(&link));assert(mounted=="/res/Object/Alink.arc");
    assert(s.sword->name=="al_swa.bmd" && s.sheath->name=="al_poda.bmd");
    s.guard=.75f;s.pose=42;
    for(auto choice:{SecondSword::Wooden,SecondSword::Master,SecondSword::Ordon,
                     SecondSword::Master,SecondSword::Wooden}) {
        selected=choice;const auto assets=second_sword_assets(choice);
        const int oldMounts=mounts;assert(prepare(&link));
        assert(mounts==oldMounts+1 && mounted==assets.archive);
        assert(s.sword->name==assets.sword && s.sword->environment==assets.environmentMapped);
        assert(bool(s.sheath)==bool(assets.sheath));
        if(s.sheath)assert(s.sheath->name==assets.sheath);
        assert(s.guard==.75f && s.pose==42 && s.seedBlade);
        auto* previous=s.sword;assert(prepare(&link));
        assert(s.sword==previous && mounts==oldMounts+1); // cached between frames
        update_sword_icon();assert(picture.art->id==static_cast<int>(choice)&&picture.shown);
        const int changes=picture.changes;update_sword_icon();assert(picture.changes==changes);
    }
    assert(forgotten>0);
    // A failed load drops incomplete private equipment; changing the selection
    // can recover, and an optional Wooden Sword scabbard is never requested.
    selected=SecondSword::Master;failModel="al_podm.bmd";
    assert(!prepare(&link));assert(!models_ready()&&!s.heap&&!s.storage);
    selected=SecondSword::Wooden;assert(prepare(&link)&&!s.sheath);
    failModel.clear();failMount=true;selected=SecondSword::Ordon;
    assert(!prepare(&link));assert(!s.heap&&!s.storage);failMount=false;
    assert(prepare(&link));
    assert(prepare(&other));assert(s.owner==&other && s.guard==0 && s.pose==0);
    missingIcon=true;update_sword_icon();assert(!picture.shown);
    missingIcon=false;selected=SecondSword::Master;update_sword_icon();assert(picture.shown&&picture.art==&master);
    release();assert(!s.sword&&!s.sheath&&!s.heap&&!s.storage);
}
'''
functions='\n'.join(function('dual_wield.cpp',n) for n in ('release_models','models_ready','prepare'))
functions+='\n'+'\n'.join(function('collection_dual_wield.cpp',n) for n in ('second_sword_icon','update_sword_icon'))
fixture=fixture.replace('// FUNCTIONS',functions)
config=(root/'src/config.cpp').read_text();ui=(root/'src/ui.cpp').read_text()
assert 'register_int("dual-wield-second-sword", static_cast<int>(SecondSword::Ordon), s_secondSword)' in config
assert '"Wooden Sword", "Ordon Sword", "Master Sword"' in ui
assert ui.index('left, "Dual Wield"')<ui.index('left, "2nd Sword"')<ui.index('left, "Arrow Modes"')
assert 'second_sword_assets(second_sword()).name' in function('collection_dual_wield.cpp','show_name')
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-multichar',
                    '-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Second sword passed: native resources, live reload/cache/failure, preserved poses, menu artwork and defaults')
