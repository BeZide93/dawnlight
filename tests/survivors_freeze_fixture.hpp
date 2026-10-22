#pragma once
// Lifecycle fixtures run gameplay unchanged with diagnostics both off and on.
// The reporter itself is exercised separately with a deterministic clock.
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
#include "survivors_freeze_diagnostics.hpp"
namespace dawnlight::freeze_diag {
inline Scope::Scope(Metric metric):metric_(metric){}
inline Scope::~Scope()=default;
inline void cache(CacheEvent){}
inline void rejected_contact(bool){}
}
namespace freeze_diag=dawnlight::freeze_diag;
#endif
