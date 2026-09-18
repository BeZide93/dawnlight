#include "bow_fire_resource.hpp"
#include "particle_resource.hpp"
#include "service_imports.hpp"

#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAEmitterManager.h"
#include "JSystem/JParticle/JPAResource.h"
#include "JSystem/JParticle/JPAResourceManager.h"
#include "d/d_particle.h"
#include "dolphin/dvd.h"
#include "mods/service.hpp"

#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace dawnlight {
namespace {

constexpr u16 kBulblinFlame = 0x8113;
JPAResourceManager* s_common = nullptr;
JKRExpHeap* s_heap = nullptr;
std::unique_ptr<u8[]> s_heapStorage;
JPAResource* s_resource = nullptr;
bool s_attempted = false;
u16 s_effect = 0;
JPAResource** s_originalResources = nullptr;
JPATexture** s_originalTextures = nullptr;
u16 s_resourceCount = 0, s_resourceCapacity = 0;
u16 s_textureCount = 0, s_textureCapacity = 0;
std::vector<JPAResource*> s_resources;
std::vector<JPATexture*> s_textures;

JPAResourceManager* common_manager() {
    auto* manager = dPa_control_c::mEmitterMng;
    return manager != nullptr ? manager->getResourceManager(u8{0}) : nullptr;
}

std::vector<u8> read_flame_resource() {
    DVDDir directory{};
    if (!DVDOpenDir("/res/Particle", &directory)) return {};
    std::vector<u8> result;
    DVDDirEntry entry{};
    while (DVDReadDir(&directory, &entry)) {
        if (entry.isDir || entry.name == nullptr) continue;
        const std::string name(entry.name);
        if (name.size() < 4 || name.substr(name.size() - 4) != ".jpc") continue;
        DVDFileInfo file{};
        if (!DVDFastOpen(entry.entryNum, &file)) continue;
        const auto length = file.length;
        // Bound allocations for malformed files; DVD reads require 32-byte alignment.
        if (length >= 16 && length <= 16 * 1024 * 1024) {
            const auto rounded = (length + 31) & ~u32{31};
            std::vector<u8> storage(rounded + 31);
            auto* bytes = reinterpret_cast<u8*>(
                (reinterpret_cast<uintptr_t>(storage.data()) + 31) & ~uintptr_t{31});
            if (DVDReadPrio(&file, bytes, rounded, 0, 2) >= static_cast<s32>(length)) {
                result = particle_resource::extract({bytes, length}, kBulblinFlame);
            }
        }
        DVDClose(&file);
        if (!result.empty()) {
            svc_log->info(mod_ctx, ("Dawnlight bow: loaded Bulblin flame from " + name).c_str());
            break;
        }
    }
    DVDCloseDir(&directory);
    return result;
}

bool install_resource(JPAResourceManager* common) {
    auto bytes = read_flame_resource();
    if (bytes.empty() || common->resRegNum == 0xffff) return false;
    const u16 textureCount = particle_resource::read16(bytes.data() + 10);
    if (unsigned(common->texRegNum) + textureCount > 0xffff) return false;

    // A private common ID keeps native Bulblin arrows and every room's pack intact.
    u16 effect = 0x7fff;
    while (effect != 0 && common->checkUserIndexDuplication(effect)) --effect;
    if (effect == 0) return false;
    particle_resource::write16(bytes.data() + 16, effect);

    // The game's fixed system heap is already partitioned during play. The
    // size-only create overload would allocate from it and abort on exhaustion,
    // even with errorFlag=false. Back this JPA heap with mod-owned host memory;
    // the parent only registers it for JKR disposer/texture lifetime tracking.
    const u32 heapSize = static_cast<u32>(bytes.size()) + 256 * 1024;
    s_heapStorage.reset(new (std::nothrow) u8[heapSize + 31]);
    if (!s_heapStorage) return false;
    auto* heapMemory = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(s_heapStorage.get()) + 31) & ~uintptr_t{31});
    s_heap = JKRExpHeap::create(heapMemory, heapSize, JKRHeap::getSystemHeap(), false);
    if (s_heap == nullptr) {
        s_heapStorage.reset();
        return false;
    }
    auto* data = static_cast<u8*>(s_heap->alloc(static_cast<u32>(bytes.size()), 32));
    if (data == nullptr) {
        s_heap->destroy();
        s_heap = nullptr;
        s_heapStorage.reset();
        return false;
    }
    std::memcpy(data, bytes.data(), bytes.size());
    JPAResourceManager donor(data, s_heap);
    s_resource = donor.getResource(effect);
    auto* indices = const_cast<BE(u16)*>(s_resource->mpTDB1);
    for (u8 i = 0; i < s_resource->texNum; ++i) {
        indices[i] = static_cast<u16>(indices[i]) + common->texRegNum;
    }

    s_originalResources = common->pResAry;
    s_originalTextures = common->pTexAry;
    s_resourceCount = common->resRegNum;
    s_resourceCapacity = common->resMaxNum;
    s_textureCount = common->texRegNum;
    s_textureCapacity = common->texMaxNum;
    s_resources.assign(common->pResAry, common->pResAry + common->resRegNum);
    s_textures.assign(common->pTexAry, common->pTexAry + common->texRegNum);
    s_resources.push_back(s_resource);
    s_textures.insert(s_textures.end(), donor.pTexAry, donor.pTexAry + donor.texRegNum);
    common->pResAry = s_resources.data();
    common->pTexAry = s_textures.data();
    common->resMaxNum = common->resRegNum = static_cast<u16>(s_resources.size());
    common->texMaxNum = common->texRegNum = static_cast<u16>(s_textures.size());
    s_effect = effect;
    return true;
}

}  // namespace

std::uint16_t bow_fire_effect() {
    auto* common = common_manager();
    if (common == nullptr) return 0;
    if (s_common != common) {
        shutdown_bow_fire_resource();
        s_common = common;
    }
    if (!s_attempted) {
        s_attempted = true;
        if (!install_resource(common)) {
            svc_log->error(mod_ctx, "Dawnlight bow: could not load the original Bulblin flame from the disc");
        }
    }
    return s_effect;
}

void shutdown_bow_fire_resource() {
    if (s_resource != nullptr && common_manager() == s_common) {
        // Invalidated emitters can still hold pRes until the next particle update.
        // Remove them before releasing the resource/texture memory.
        auto* manager = dPa_control_c::mEmitterMng;
        for (u8 group = 0; group < manager->gidMax; ++group) {
            auto& list = manager->pEmtrUseList[group];
            for (auto* link = list.getFirst(); link != list.getEnd();) {
                auto* next = link->getNext();
                auto* emitter = link->getObject();
                if (emitter->pRes == s_resource) manager->forceDeleteEmitter(emitter);
                link = next;
            }
        }
        s_common->pResAry = s_originalResources;
        s_common->pTexAry = s_originalTextures;
        s_common->resRegNum = s_resourceCount;
        s_common->resMaxNum = s_resourceCapacity;
        s_common->texRegNum = s_textureCount;
        s_common->texMaxNum = s_textureCapacity;
    }
    if (s_heap != nullptr) s_heap->destroy();
    s_heap = nullptr;
    // destroy() runs texture/disposer cleanup in this buffer; free it afterwards.
    s_heapStorage.reset();
    s_resource = nullptr;
    s_common = nullptr;
    s_effect = 0;
    s_attempted = false;
    s_resources.clear();
    s_textures.clear();
}

}  // namespace dawnlight
