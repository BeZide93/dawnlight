#pragma once
#include <cstdint>
#ifndef DAWNLIGHT_FREEZE_DIAGNOSTICS
#define DAWNLIGHT_FREEZE_DIAGNOSTICS 0
#endif

namespace dawnlight::freeze_diag {
enum class Metric : unsigned { Execute, Resource, Contacts, ContactDispatch, ContactDelete, Update, Player, Count };
enum class CacheEvent { Acquire, Reuse, Full, Release };
struct Context {
    bool active=false,hub=false,arena=false,transition=false,paused=false,event=false,menu=false;
    unsigned map=0,phase=0,tick=0;
};
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
void initialize();
void shutdown();
void pulse();
void context(Context value);
void cache(CacheEvent event);
void rejected_contact(bool incoming);
class Scope {
    Metric metric_;
    uint64_t start_=0,generation_=0;
    bool engaged_=false;
public:
    explicit Scope(Metric metric);
    ~Scope();
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
#endif
}
