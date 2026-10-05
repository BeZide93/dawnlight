#include "twilit_aim.hpp"
#include "twilit_stamina.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "dusk/config_var.hpp"

#include <string_view>

namespace dawnlight {
namespace {
using BoolVar = dusk::config::ConfigVar<bool>;
using Layer = dusk::config::ConfigVarLayer;
using GetConfigVarFn = dusk::config::ConfigVarBase* (*)(std::string_view);
GetConfigVarFn s_getConfigVar = nullptr;
using HasSubscribersFn = bool (*)(const dusk::config::ConfigVarBase*);
using NotifyChangedFn = void (*)(dusk::config::ConfigVarBase*, const void*);
HasSubscribersFn s_hasSubscribers = nullptr;
NotifyChangedFn s_notifyChanged = nullptr;

bool resolve_notifications() {
    if (s_hasSubscribers && s_notifyChanged) return true;
    if (!svc_hook || !svc_hook->resolve) return false;
    void* has = nullptr;
    void* notify = nullptr;
    if (svc_hook->resolve(mod_ctx, "dusk::config::ConfigVarBase::has_subscribers",
                          &has, nullptr) != MOD_OK || !has ||
        svc_hook->resolve(mod_ctx, "dusk::config::ConfigVarBase::notify_changed",
                          &notify, nullptr) != MOD_OK || !notify) return false;
    s_hasSubscribers = reinterpret_cast<HasSubscribersFn>(has);
    s_notifyChanged = reinterpret_cast<NotifyChangedFn>(notify);
    return true;
}

// Identity only: reacquire the live CVar before every access, including shutdown.
BoolVar* s_ownedFirstPerson = nullptr;

BoolVar* first_person_var() {
    if (!s_getConfigVar) {
        void* address = nullptr;
        if (!svc_hook || !svc_hook->resolve ||
            svc_hook->resolve(mod_ctx, "dusk::config::GetConfigVar", &address, nullptr) != MOD_OK ||
            !address) return nullptr;
        s_getConfigVar = reinterpret_cast<GetConfigVarFn>(address);
    }
    return static_cast<BoolVar*>(
        s_getConfigVar("mod.com_dusklight_twilit__essentials.bulletTimeFirstPerson"));
}

void release_override(BoolVar* current) {
    if (current && current == s_ownedFirstPerson &&
        current->getLayer() == Layer::Override && current->getValue()) {
        current->clearOverride();
    }
    s_ownedFirstPerson = nullptr;
}
}  // namespace

bool twilit_bullet_time_aim_enabled() {
    if (!twilit_bullet_time_enabled()) return false;
    const auto* firstPerson = first_person_var();
    return firstPerson && firstPerson->getValue();
}

void update_twilit_aim() {
    auto* firstPerson = first_person_var();
    if (firstPerson != s_ownedFirstPerson ||
        (firstPerson && firstPerson->getLayer() != Layer::Override)) {
        s_ownedFirstPerson = nullptr;
    }
    const bool custom = aim_mode() == AimMode::Cinema || aim_mode() == AimMode::ThirdPerson;
    if (!custom || !twilit_bullet_time_enabled()) {
        release_override(firstPerson);
        return;
    }
    // TE's First-person option supplies the air aim status and input hooks.
    // Enable that backend without saving over the user's preference; our
    // normal camera hooks select Cinema/Third Person instead of first person.
    // Never replace launch/speedrun/another mod's pre-existing override.
    if (firstPerson && firstPerson->getLayer() == Layer::Value && !firstPerson->getValue() &&
        resolve_notifications()) {
        firstPerson->setOverrideValue(true);
        s_ownedFirstPerson = firstPerson;
    }
}

void shutdown_twilit_aim() {
    release_override(first_person_var());
    s_getConfigVar = nullptr;
    s_hasSubscribers = nullptr;
    s_notifyChanged = nullptr;
}
}  // namespace dawnlight

// The inline CVar mutators call these private host methods, which are absent
// from the SDK import libraries. Forward through the symbol manifest so TE's
// subscription receives both override and restore notifications on every OS.
// Resolve both methods before changing a CVar; never silently skip callbacks.
bool dusk::config::ConfigVarBase::has_subscribers() const {
    return dawnlight::resolve_notifications() && dawnlight::s_hasSubscribers(this);
}

void dusk::config::ConfigVarBase::notify_changed(const void* previousValue) {
    if (dawnlight::resolve_notifications()) dawnlight::s_notifyChanged(this, previousValue);
}
