#include "survivors_integrity.hpp"
#include "survivors_cheat_guard.hpp"
#include "service_imports.hpp"
// The host normally provides this alias through its settings precompiled header.
namespace dusk { using config::ConfigVar; }
#include "dusk/mod_loader.hpp"
#include "mods/service.hpp"
#include <array>

// Keep the Cheats enum types exact without pulling in the native UI headers.
namespace dusk { enum class AlwaysGreatspinMode : u8; enum class MagicArmorMode : u8; }

// These private host methods are absent from the SDK link stubs on Apple/Windows.
// Resolve their actual host addresses just like GetConfigVar; keep the bridge
// definitions hidden in this module so no native symbol is interposed.
namespace {
using NativeHasSubscribers=bool (*)(const dusk::config::ConfigVarBase*);
using NativeNotifyChanged=void (*)(dusk::config::ConfigVarBase*,const void*);
NativeHasSubscribers s_nativeHasSubscribers=nullptr;
NativeNotifyChanged s_nativeNotifyChanged=nullptr;
}
namespace dusk::config {
bool ConfigVarBase::has_subscribers() const {
    return s_nativeHasSubscribers && s_nativeHasSubscribers(this);
}
void ConfigVarBase::notify_changed(const void* previous) {
    if(s_nativeNotifyChanged)s_nativeNotifyChanged(this,previous);
}
}

namespace dawnlight {
namespace {
using namespace dusk::config;
constexpr std::array kBoolCheats{
    "game.infiniteHearts", "game.infiniteArrows", "game.infiniteSeeds", "game.infiniteBombs",
    "game.infiniteOil", "game.infiniteOxygen", "game.infiniteRupees", "game.enableIndefiniteItemDrops",
    "game.moonJump", "game.superClawshot", "game.enableFastIronBoots", "game.canTransformAnywhere",
    "game.fastRoll", "game.fastSpinner", "game.invincibleEnemies", "game.easyQuickSpin"
};
std::array<survivors::CheatGuard<bool>,kBoolCheats.size()> s_bools;
survivors::CheatGuard<dusk::AlwaysGreatspinMode> s_spin;
survivors::CheatGuard<dusk::MagicArmorMode> s_armor;
std::array<Subscription,18> s_subscriptions{};
bool s_running=false, s_locked=false;
using LoaderFn=dusk::mods::ModLoader& (*)();
LoaderFn s_loader=nullptr;
using GetFn=ConfigVarBase* (*)(std::string_view);
GetFn s_get=nullptr;
using SubscribeFn=Subscription (*)(std::string_view,ChangeCallback);
SubscribeFn s_subscribe=nullptr;
using UnsubscribeFn=void (*)(Subscription);
UnsubscribeFn s_unsubscribe=nullptr;

template<class T> bool resolve(const char* name,T& fn) {
    void* address=nullptr;
    if(svc_hook->resolve(mod_ctx,name,&address,nullptr)!=MOD_OK || !address)return false;
    fn=reinterpret_cast<T>(address);return true;
}
void enforce() {
    if(!s_locked)return;
    for(auto& cheat:s_bools)cheat.enforce();
    s_spin.enforce();s_armor.enforce();
}
bool other_mod_active() {
    if(!s_loader)return false;
    const std::string_view self=svc_host->mod_id(mod_ctx);
    // Use the native loader's actual activation state, including asset-only mods.
    // Disabled, failed and provider-suspended packages do not count.
    for(const auto& mod:s_loader().active_mods())
        if(mod.metadata.id!=self)return true;
    return false;
}
}
ModResult initialize_survivors_integrity(ModError* error) {
    if(!resolve("dusk::config::ConfigVarBase::has_subscribers",s_nativeHasSubscribers) ||
       !resolve("dusk::config::ConfigVarBase::notify_changed",s_nativeNotifyChanged) ||
       !resolve("dusk::config::GetConfigVar",s_get) ||
       !resolve("dusk::config::subscribe",s_subscribe) ||
       !resolve("dusk::config::unsubscribe",s_unsubscribe) ||
       !resolve("dusk::mods::ModLoader::instance",s_loader))
        return mods::set_error(error,MOD_UNAVAILABLE,"Survivors requires host cheat controls and mod inventory");
    for(size_t i=0;i<kBoolCheats.size();++i)s_bools[i].var=static_cast<ConfigVar<bool>*>(s_get(kBoolCheats[i]));
    s_spin.var=static_cast<ConfigVar<dusk::AlwaysGreatspinMode>*>(s_get("game.alwaysGreatspin"));
    s_armor.var=static_cast<ConfigVar<dusk::MagicArmorMode>*>(s_get("game.armorRupeeDrain"));
    if(!s_spin.var || !s_armor.var ||
       std::any_of(s_bools.begin(),s_bools.end(),[](const auto& guard){return !guard.var;}))
        return mods::set_error(error,MOD_UNAVAILABLE,"Survivors could not find all host Cheats settings");
    return MOD_OK;
}
void begin_survivors_integrity(bool allow_cheats,bool& modded) {
    end_survivors_integrity();
    s_running=true;s_locked=!allow_cheats;
    if(s_locked) {
        const auto changed=[](ConfigVarBase&,const void*){enforce();};
        for(size_t i=0;i<kBoolCheats.size();++i)s_subscriptions[i]=s_subscribe(kBoolCheats[i],changed);
        s_subscriptions[16]=s_subscribe("game.alwaysGreatspin",changed);
        s_subscriptions[17]=s_subscribe("game.armorRupeeDrain",changed);
    }
    enforce();modded=other_mod_active();
}
void update_survivors_integrity(bool& modded) {
    if(!s_running)return;
    enforce();modded=modded || other_mod_active();
}
void end_survivors_integrity() {
    s_running=s_locked=false;
    // Only remove our runtime masks. The host's speedrunMode preference merely
    // exposes another game mode; it does not mean that mode is currently active.
    for(auto& cheat:s_bools)cheat.release();
    s_spin.release();s_armor.release();
    for(auto& token:s_subscriptions) {if(token && s_unsubscribe)s_unsubscribe(token);token=0;}
}
void shutdown_survivors_integrity() {
    end_survivors_integrity();
    for(auto& cheat:s_bools)cheat={};
    s_spin={};s_armor={};
    s_loader=nullptr;s_get=nullptr;s_subscribe=nullptr;s_unsubscribe=nullptr;
    s_nativeHasSubscribers=nullptr;s_nativeNotifyChanged=nullptr;
}
}
