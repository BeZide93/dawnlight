#include "survivors_roundtable.hpp"
#include "generated/roundtable_layout.hpp"
#include "service_imports.hpp"
#include "process_actor.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/d_bg_w.h"
#include "d/d_particle.h"
#include "d/actor/d_a_bg.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRSolidHeap.h"
#include "mods/svc/resource.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <new>
#include <vector>

namespace dawnlight {
namespace {
constexpr ActorId kNoRoom = fpcM_ERROR_PROCESS_ID_e;
// Aurora has a shared 5 MiB per-frame vertex arena. Keep the authored RTH2
// floats on disk, but cache/send fixed-point vertices (18 instead of 32 bytes).
// Position error <= 1/16 world unit; UV error <= 1/128 texel in the 1024 atlas.
struct Vertex { s16 x,y,z; u16 u,v,lu,lv; u8 r,g,b,a; };
static_assert(sizeof(Vertex)==18);
constexpr size_t kMaxVertices=180000;
static_assert(kMaxVertices*sizeof(Vertex) < 5*1024*1024 - 1536*1024);
std::vector<Vertex> s_vertices;
std::vector<u8> s_collision;
alignas(32) u8 s_texture[1024*1024*2];
GXTexObj s_textureObject{},s_shadowObject{};
alignas(32) u8 s_shadow[1024*1024];
bool s_ready=false;

bool resource(const char* name, std::vector<u8>& out) {
    ResourceBuffer buffer=RESOURCE_BUFFER_INIT;
    if(svc_resource->load(mod_ctx,name,&buffer)!=MOD_OK)return false;
    if(buffer.data && buffer.size && buffer.size<=6*1024*1024)
        out.assign(static_cast<const u8*>(buffer.data),static_cast<const u8*>(buffer.data)+buffer.size);
    svc_resource->free(mod_ctx,&buffer);
    return !out.empty();
}
u32 le32(const u8* p) { return u32(p[0])|(u32(p[1])<<8)|(u32(p[2])<<16)|(u32(p[3])<<24); }
bool load_assets() {
    if(s_ready)return true;
    std::vector<u8> mesh,texture,collision,shadow;
    if(!resource("survivors_roundtable/hall.mesh",mesh) ||
       !resource("survivors_roundtable/hall.dzb",collision) ||
       !resource("survivors_roundtable/materials.rgb565",texture) ||
       !resource("survivors_roundtable/shadows.i8",shadow))return false;
    if(mesh.size()<8 || std::memcmp(mesh.data(),"RTH2",4) ||
       shadow.size()!=sizeof(s_shadow) || texture.size()!=sizeof(s_texture) || collision.size()<52)return false;
    const u32 count=le32(mesh.data()+4);
    if(count==0 || count%3 || count>kMaxVertices || mesh.size()!=8+size_t(count)*32)return false;
    std::vector<Vertex> vertices(count);
    for(size_t i=0;i<count;++i) {
        const auto* p=mesh.data()+8+i*32;
        float values[7];
        for(unsigned j=0;j<7;++j) {
            const u32 bits=le32(p+j*4);std::memcpy(&values[j],&bits,4);
            if(!std::isfinite(values[j]))return false;
        }
        for(unsigned j=0;j<3;++j)
            if(values[j]<-4096.f || values[j]>4095.875f)return false;
        for(unsigned j=3;j<7;++j)
            if(values[j]<0.f || values[j]>=1.f)return false;
        const auto position=[](float v){return static_cast<s16>(std::lround(v*8.f));};
        const auto uv=[](float v){return static_cast<u16>(std::min(65535l,std::lround(v*65536.f)));};
        vertices[i]={position(values[0]),position(values[1]),position(values[2]),
            uv(values[3]),uv(values[4]),uv(values[5]),uv(values[6]),p[28],p[29],p[30],p[31]};
    }
    // Validate every DZB table before native pointer relocation. The bundled
    // format is also checked by the generator test, including tree ancestry.
    const auto be=[](const u8* p){return (u32(p[0])<<24)|(u32(p[1])<<16)|(u32(p[2])<<8)|p[3];};
    constexpr size_t sizes[]{12,10,2,20,52,16};
    for(unsigned i=0;i<6;++i) {
        const size_t n=be(collision.data()+i*8),offset=be(collision.data()+i*8+4);
        if(!n || n>65535 || offset<52 || offset>collision.size() ||
           n>(collision.size()-offset)/sizes[i])return false;
    }
    s_vertices=std::move(vertices);s_collision=std::move(collision);
    std::memcpy(s_texture,texture.data(),sizeof(s_texture));
    GXInitTexObj(&s_textureObject,s_texture,1024,1024,GX_TF_RGB565,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXInitTexObjLOD(&s_textureObject,GX_LINEAR,GX_LINEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);
    std::memcpy(s_shadow,shadow.data(),sizeof(s_shadow));
    GXInitTexObj(&s_shadowObject,s_shadow,1024,1024,GX_TF_I8,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXInitTexObjLOD(&s_shadowObject,GX_LINEAR,GX_LINEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);
    s_ready=true;
    return true;
}

// Draw after opaque actors: layered, depth-tested black planes obscure only
// actors inside the passage. Back-face culling leaves the view into the hall
// clear when a native entrance camera is behind the shutter.
J3DDrawBuffer* portal_list() {
    return g_dComIfG_gameInfo.drawlist.mDrawBuffers[dDlst_list_c::DB_XLU_LIST];
}
class PortalShade : public J3DPacket {
public:
    ActorId owner=kNoRoom;
    void draw() override {
        auto* actor=static_cast<fopAc_ac_c*>(fopAcM_SearchByID(owner));
        if(!actor || fopAcM_GetName(actor)!=fpcNm_BG_e)return;
        j3dSys.reinitGX(); // Also disables fog so the passage stays black.
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0);GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);GXSetCullMode(GX_CULL_BACK);
        constexpr float depths[]{60,100,140,180,220,260};
        constexpr u8 alpha[]{45,65,90,120,170,255};
        constexpr float w=roundtable_layout::exitHalfWidth,y=roundtable_layout::floorY;
        // Back-to-front; no depth writes or collisions on the shadow planes.
        for(int i=5;i>=0;--i) {
            const float z=roundtable_layout::exitZ+depths[i];
            const float corners[6][2]{{-w,y},{-w,y+660},{w,y+660},{-w,y},{w,y+660},{w,y}};
            GXBegin(GX_TRIANGLES,GX_VTXFMT0,6);
            for(const auto& corner:corners) {GXPosition3f32(corner[0],corner[1],z);GXColor4u8(0,0,0,alpha[i]);}
            GXEnd();
        }
        j3dSys.reinitGX();
    }
};

class Room : public J3DPacket {
public:
    ActorId owner=kNoRoom;
    PortalShade shade;
    dBgW* originalDzb=nullptr;
    dBgWKCol* originalKcl=nullptr;
    dBgW* collision=nullptr;
    std::unique_ptr<u8[]> storage;
    JKRExpHeap* heap=nullptr;
    bool installed=false;
    std::array<u32,18> fire{};
    LIGHT_INFLUENCE light{},tableLight{};
    bool lit=false;

    ~Room() override { if(heap)heap->destroy(); }
    void extinguish() {
        for(auto& id:fire) {
            if(id)if(auto* emitter=dComIfGp_particle_getEmitter(id)) {
                emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();
            }
            id=0;
        }
        if(lit){dKy_plight_cut(&light);dKy_plight_cut(&tableLight);}
        lit=false;
    }
    void update_fire() {
        constexpr u16 effects[]{0x3ad,0x3af,0x3ae};
        // Overlapping tongues over the log bed. Scale sparks independently
        // from the taller flames so they remain a subtle part of the fire.
        for(unsigned i=0;i<15;++i) {
            const unsigned tongue=i/3,effect=i%3;
            const float z=-110.f+55.f*tongue;
            const cXyz position(roundtable_layout::hearthFireX+(tongue%2?12.f:0.f),
                roundtable_layout::hearthFireY,z);
            const float height=tongue==2?2.4f:(tongue%2?1.9f:1.5f);
            const cXyz scale(effect==2?.5f:1.1f,effect==2?.7f:height,effect==2?.5f:1.0f);
            fire[i]=dComIfGp_particle_set(fire[i],effects[effect],&position,nullptr,nullptr,&scale,
                0xff,nullptr,-1,nullptr,nullptr,nullptr);
        }
        const cXyz flame(0,roundtable_layout::flameY,0);
        const cXyz flameScale(.85f,2.5f,.85f);
        for(unsigned i=15;i<fire.size();++i) {
            fire[i]=dComIfGp_particle_set(fire[i],effects[i-15],&flame,nullptr,nullptr,&flameScale,
                0xff,nullptr,-1,nullptr,nullptr,nullptr);
            if(auto* emitter=dComIfGp_particle_getEmitter(fire[i])) {
                emitter->setGlobalPrmColor(255,225,120);
                emitter->setGlobalEnvColor(255,130,18);
            }
        }
        if(!lit) {
            tableLight.mPosition.set(0,roundtable_layout::flameLightY,0);
            tableLight.mColor={255,205,112,255};tableLight.mPow=1800;
            tableLight.mFluctuation=1;dKy_plight_set(&tableLight);
            light.mPosition.set(roundtable_layout::hearthLightX,roundtable_layout::hearthLightY,0);light.mColor={204,123,60,255};
            light.mPow=850;light.mFluctuation=1;dKy_plight_set(&light);lit=true;
        }
    }
    bool prepare() {
        constexpr u32 size=2*1024*1024;
        storage.reset(new(std::nothrow) u8[size+31]);
        if(!storage)return false;
        auto* memory=reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(storage.get())+31)&~uintptr_t{31});
        heap=JKRExpHeap::create(memory,size,JKRHeap::getRootHeap(),false);
        if(!heap)return false;
        auto* solid=JKRSolidHeap::create(0x180000,heap,false);
        if(!solid)return false;
        auto* previous=solid->becomeCurrentHeap();
        auto* bytes=solid->alloc(static_cast<u32>(s_collision.size()),32);
        bool ready=false;
        if(bytes) {
            std::memcpy(bytes,s_collision.data(),s_collision.size());
            auto* dzb=static_cast<cBgD_t*>(cBgS::ConvDzb(bytes));
            collision=JKR_NEW dBgW();
            ready=collision && !collision->Set(dzb,cBgW::GLOBAL_e,nullptr);
            if(ready)collision->SetPriority(dBgW_Base::PRIORITY_0);
        }
        previous->becomeCurrentHeap();
        return ready;
    }
    void draw() override {
        auto* actor=static_cast<fopAc_ac_c*>(fopAcM_SearchByID(owner));
        if(!installed || !actor || fopAcM_GetName(actor)!=fpcNm_BG_e)return;
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX1,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_S16,3);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_U16,16);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX1,GX_TEX_ST,GX_U16,16);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(2);GXSetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
        GXSetTexCoordGen(GX_TEXCOORD1,GX_TG_MTX2x4,GX_TG_TEX1,GX_IDENTITY);
        GXLoadTexObj(&s_shadowObject,GX_TEXMAP1);
        GXLoadTexObj(&s_textureObject,GX_TEXMAP0);
        GXSetNumTevStages(2);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
        GXSetTevOrder(GX_TEVSTAGE1,GX_TEXCOORD1,GX_TEXMAP1,GX_COLOR_NULL);
        GXSetTevOp(GX_TEVSTAGE1,GX_MODULATE);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE);
        GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);GXSetCullMode(GX_CULL_NONE);
        for(size_t start=0;start<s_vertices.size();start+=60000) {
            const size_t end=std::min(start+60000,s_vertices.size());
            GXBegin(GX_TRIANGLES,GX_VTXFMT0,static_cast<u16>(end-start));
            for(size_t i=start;i<end;++i) {
                const auto& v=s_vertices[i];GXPosition3s16(v.x,v.y,v.z);
                GXColor4u8(v.r,v.g,v.b,v.a);GXTexCoord2u16(v.u,v.v);GXTexCoord2u16(v.lu,v.lv);
            }
            GXEnd();
        }
        j3dSys.reinitGX();
    }
};
std::list<Room> s_rooms;
struct CreateFrame { void* args; bool hub; };
std::vector<CreateFrame> s_creates;

void detach_packet(J3DDrawBuffer* list,J3DPacket* packet) {
    // Packets are intrusive and can survive presentation-only frames. Remove
    // every retained reference before freeing the packet or unloading the mod.
    if(!list || !list->mpBuffer)return;
    for(u32 i=0;i<list->getEntryTableSize();++i) {
        J3DPacket* previous=nullptr;
        for(auto* p=list->mpBuffer[i];p;) {
            auto* next=p->getNextPacket();
            if(p==packet) {
                if(previous)previous->setNextPacket(next);else list->mpBuffer[i]=next;
            } else previous=p;
            p=next;
        }
    }
    packet->setNextPacket(nullptr);
}
void detach(Room& room) {
    detach_packet(dComIfGd_getOpaListBG(),&room);
    detach_packet(portal_list(),&room.shade);
}
void restore(Room& room,daBg_c* actor,bool registerOriginal) {
    detach(room);room.extinguish();room.installed=false;
    dComIfG_Bgsp().Release(room.collision);
    actor->mpBgW=room.originalDzb;actor->mpKCol=room.originalKcl;
    auto* original=room.originalDzb?static_cast<dBgW_Base*>(room.originalDzb):room.originalKcl;
    if(registerOriginal && original)dComIfG_Bgsp().Regist(original,actor);
    dStage_roomControl_c::setBgW(fopAcM_GetParam(actor),original);
}
daBg_c* background(void* args) {
    auto* process=mods::arg<void*>(args,1);
    if(!process || !is_actor_process(process))return nullptr;
    auto* actor=static_cast<fopAc_ac_c*>(process);
    return fopAcM_GetName(actor)==fpcNm_BG_e?static_cast<daBg_c*>(actor):nullptr;
}
}

HookAction before_roundtable_process(void* args,void* result,bool createHub) {
    auto* actor=background(args);if(!actor)return HOOK_CONTINUE;
    const auto* methods=reinterpret_cast<const leafdraw_method_class*>(actor->sub_method);
    if(!methods)return HOOK_CONTINUE;
    const auto method=mods::arg<process_method_func>(args,0);
    if(method==methods->base.create_method)s_creates.push_back({args,createHub});
    auto it=std::find_if(s_rooms.begin(),s_rooms.end(),[&](const Room& r){return r.owner==fopAcM_GetID(actor);});
    if(it==s_rooms.end())return HOOK_CONTINUE;
    if(method==methods->base.delete_method) {
        // Native Delete still owns the original room heap and its models.
        restore(*it,actor,true);s_rooms.erase(it);return HOOK_CONTINUE;
    }
    if(method==methods->base.execute_method)it->update_fire();
    if(method!=methods->draw_method)return HOOK_CONTINUE;
    // Terrain must precede native projected shadows, not overpaint them
    // in the later actor opaque pass (Link and Zelda both use real shadows).
    auto* list=dComIfGd_getOpaListBG();
    if(list && list->mpBuffer && list->getEntryTableSize()) {
        bool queued=false;
        for(auto* p=list->mpBuffer[0];p;p=p->getNextPacket())if(p==&*it){queued=true;break;}
        if(!queued)list->entryImm(&*it,0);
    }
    auto* shadeList=portal_list();
    if(shadeList && shadeList->mpBuffer && shadeList->getEntryTableSize()) {
        bool queued=false;
        for(auto* p=shadeList->mpBuffer[0];p;p=p->getNextPacket())if(p==&it->shade){queued=true;break;}
        if(!queued)shadeList->entryImm(&it->shade,0);
    }
    // Keep room lighting updated for actors, but don't submit the old models
    // or native room-part decorations over the authored hall.
    g_env_light.settingTevStruct(0x10,nullptr,dComIfGp_roomControl_getTevStr(fopAcM_GetParam(actor)));
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}

void after_roundtable_process(void* args,void* result) {
    // A generic post-hook also runs after Delete, where process may already be
    // invalid. Only paired, still-live Create callbacks may inspect the actor.
    if(s_creates.empty() || s_creates.back().args!=args)return;
    const bool createHub=s_creates.back().hub;s_creates.pop_back();
    if(!createHub)return;
    auto* actor=background(args);
    if(!actor || fopAcM_GetParam(actor)!=51)return;
    const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
    if(!methods || mods::arg<process_method_func>(args,0)!=methods->create_method ||
       *static_cast<int*>(result)!=cPhs_COMPLEATE_e)return;
    const auto owner=fopAcM_GetID(actor);
    if(std::any_of(s_rooms.begin(),s_rooms.end(),[&](const Room& r){return r.owner==owner;}))return;
    if(!load_assets()) {
        svc_log->error(mod_ctx,"Roundtable hub: invalid/missing assets; retaining native room");return;
    }
    s_rooms.emplace_back();auto& room=s_rooms.back();room.owner=owner;room.shade.owner=owner;
    if(!room.prepare()) {s_rooms.pop_back();svc_log->error(mod_ctx,"Roundtable hub: collision allocation failed");return;}
    room.originalDzb=actor->mpBgW;room.originalKcl=actor->mpKCol;
    auto* original=actor->mpBgW?static_cast<dBgW_Base*>(actor->mpBgW):actor->mpKCol;
    // Register first: allocation failure must never leave the room without a
    // floor. Both registries are changed within this completed create callback.
    if(dComIfG_Bgsp().Regist(room.collision,actor)) {s_rooms.pop_back();return;}
    if(original)dComIfG_Bgsp().Release(original);
    actor->mpBgW=room.collision;actor->mpKCol=nullptr;
    dStage_roomControl_c::setBgW(51,room.collision);
    room.installed=true;
    svc_log->info(mod_ctx,"Roundtable hub: installed private room geometry and collision");
}

void shutdown_roundtable() {
    for(auto& room:s_rooms) {
        auto* actor=static_cast<fopAc_ac_c*>(fopAcM_SearchByID(room.owner));
        if(actor && fopAcM_GetName(actor)==fpcNm_BG_e)restore(room,static_cast<daBg_c*>(actor),true);
        else {detach(room);room.extinguish();if(room.installed)dComIfG_Bgsp().Release(room.collision);}
    }
    s_rooms.clear();s_creates.clear();s_vertices.clear();s_collision.clear();s_ready=false;
}
}
