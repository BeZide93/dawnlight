#include "boss.hpp"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("cLib_calcTimer<int>", int(int*), IntTimerHook);
DEFINE_HOOK_SYMBOL("cLib_calcTimer<unsigned char>", u8(u8*), ByteTimerHook);
bool s_conditionalTimers = false;

template <typename T, std::size_t N>
HookAction conditional_timer(EnemySlowStep* step, T* timer, void* result,
                             const std::array<T*, N>& owned) {
    if (step->timerTick) return HOOK_CONTINUE;
    for (auto* value : owned) {
        if (value == timer) {
            *static_cast<T*>(result) = *timer;
            return HOOK_SKIP_ORIGINAL;
        }
    }
    return HOOK_CONTINUE;
}
HookAction before_int_timer(ModContext*, void* args, void* result, void*) {
    auto* step = current_enemy_slow_step();
    return step ? conditional_timer(step, mods::arg<int*>(args, 0), result,
        step->conditionalIntTimers) : HOOK_CONTINUE;
}
HookAction before_byte_timer(ModContext*, void* args, void* result, void*) {
    auto* step = current_enemy_slow_step();
    return step ? conditional_timer(step, mods::arg<u8*>(args, 0), result,
        step->conditionalByteTimers) : HOOK_CONTINUE;
}
bool has_complete_symbol(const char* name) {
    void* address = nullptr;
    HookSymbolFlags flags{};
    // Pinned symgen 1.3.6's manifest bit 5 records inlined call sites. A hook
    // would cover only some callers in such a host build: retain native ticking.
    constexpr unsigned inlineSites = 1u << 5;
    return svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, name, &address, &flags) == MOD_OK && address &&
        !(static_cast<unsigned>(flags) & inlineSites);
}
}
bool boss_conditional_timers_available() { return s_conditionalTimers; }
void install_boss_conditional_timers() {
    s_conditionalTimers = has_complete_symbol("cLib_calcTimer<int>") &&
        has_complete_symbol("cLib_calcTimer<unsigned char>") &&
        mods::hook::add_pre<IntTimerHook>(svc_hook, before_int_timer) == MOD_OK &&
        mods::hook::add_pre<ByteTimerHook>(svc_hook, before_byte_timer) == MOD_OK;
}
}
