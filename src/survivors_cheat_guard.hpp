#pragma once
#include "dusk/config_var.hpp"

namespace dawnlight::survivors {
// Suppress effective values without changing stored preferences. The host saves
// Value/Speedrun layers, but OMITS Override entries entirely. Use its temporary
// policy layer so a config save mid-run cannot erase an enabled preference.
// This does not enable Dusklight's speedrun mode.
// All Cheats-tab defaults are zero/false, including both enum selectors.
template<class T> struct CheatGuard {
    dusk::config::ConfigVar<T>* var = nullptr;
    bool owned = false;
    bool applying = false;
    dusk::config::ConfigVarLayer previous_layer = dusk::config::ConfigVarLayer::Default;
    T previous{};
    void enforce() {
        using Layer = dusk::config::ConfigVarLayer;
        if (!var || applying) return;
        // A UI edit replaces our override. Keep that newly saved preference;
        // suppress its effect again synchronously through the change subscription.
        if (owned && (var->getLayer()!=Layer::Speedrun || var->getValue()!=T{})) owned=false;
        if (owned || var->getValue()==T{}) return;
        previous=var->getValue();
        previous_layer=var->getLayer();
        applying=true;
        // Launch overrides normally outrank the policy layer. Temporarily lift
        // one without touching its underlying saved value, and restore it on exit.
        // Clearing can notify subscribers synchronously: ignore our own writes.
        if(previous_layer==Layer::Override)var->clearOverride();
        else if(previous_layer==Layer::Speedrun)var->clearSpeedrunOverride();
        owned=true;
        var->setSpeedrunValue(T{});
        applying=false;
    }
    void release() {
        using Layer = dusk::config::ConfigVarLayer;
        const bool ours=owned && var && var->getLayer()==Layer::Speedrun && var->getValue()==T{};
        owned=false;
        if (!ours) return;
        applying=true;
        var->clearSpeedrunOverride();
        if(previous_layer==Layer::Override)var->setOverrideValue(previous);
        else if(previous_layer==Layer::Speedrun)var->setSpeedrunValue(previous);
        applying=false;
    }
};
}
