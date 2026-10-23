#pragma once
#include <array>
#include <algorithm>
namespace dawnlight::survivors {
// First five IDs retain the existing combat variants.
enum class Species : unsigned {
    Bokoblin, Keese, Rat, Club, Bow, Tektite, Lizalfos, Helmasaur, Helmasaurus,
    Guay, Kargarok, Bomskit, Stalchild, Gibdo, Stalfos, Puppet,
    Bubble, FireBubble, BlackChu, Dynalfos, Aeralfos, Darknut, Dodongo, Stalhound, ShadowClub, ShadowBow, ShadowBeast,
    ShadowKeese, ShadowKargarok, StallordHead, ZantHand, KingBulblin, DeathSword, TwilitBloat, PhantomZant, Darkhammer, PuppetZelda, Ganondorf, Bullbo, Count
};
constexpr unsigned species_count=unsigned(Species::Count);
struct RosterEntry { Species species; unsigned unlock_seconds=0; };
struct MapRoster {
    std::array<Species,3> basic;
    std::array<RosterEntry,4> specialists;
    unsigned specialist_count;
    std::array<Species,4> elites;
};
inline constexpr std::array<MapRoster,5> map_rosters{{
    {{{Species::Bokoblin,Species::Rat,Species::Keese}},
     {{{Species::Lizalfos,60},{Species::Helmasaur,120},{Species::Club,180}}},3,
     {{Species::Lizalfos,Species::Helmasaur,Species::Dynalfos,Species::Darknut}}},
    {{{Species::Bokoblin,Species::Club,Species::Tektite}},
     {{{Species::Bow,60},{Species::Guay,120},{Species::Kargarok,180}}},3,
     {{Species::Bokoblin,Species::Bullbo,Species::Aeralfos,Species::KingBulblin}}},
    {{{Species::Stalchild,Species::Club,Species::Bow}},
     {{{Species::Stalhound,60},{Species::Dodongo,120},{Species::Gibdo,240}}},3,
     {{Species::Stalhound,Species::Stalfos,Species::DeathSword,Species::StallordHead}}},
    {{{Species::ShadowClub,Species::ShadowBow,Species::ShadowKeese}},
     {{{Species::ShadowBeast,120},{Species::ShadowKargarok,180}}},2,
     {{Species::ShadowBeast,Species::TwilitBloat,Species::ShadowKargarok,Species::PhantomZant}}},
    {{{Species::Bokoblin,Species::Club,Species::Bow}},
     {{{Species::Lizalfos,60},{Species::Helmasaurus,120},{Species::Dynalfos,180}}},3,
     {{Species::Darknut,Species::Darkhammer,Species::PuppetZelda,Species::Ganondorf}}}
}};
constexpr const MapRoster& map_roster(unsigned map) { return map_rosters[map<map_rosters.size()?map:0]; }
constexpr unsigned ordinary_limit(Species type,float horde) {
    horde=std::clamp(horde,1.f,5.f);
    switch(type) {
    case Species::ShadowBow:case Species::Bow:return std::min(12u,unsigned(4*horde));
    case Species::ShadowKargarok:case Species::Kargarok:return std::min(6u,unsigned(2*horde));
    case Species::Guay:return std::min(12u,unsigned(4*horde));
    case Species::Bomskit:return std::min(6u,unsigned(2*horde));
    case Species::ShadowBeast:case Species::Gibdo:case Species::Dynalfos:return std::min(4u,unsigned(2*horde));
    default:return 345;
    }
}
// Filter before choosing; saturated specialists fall back to the basic pool.
// Counts include pending actors so async creation cannot overbook a cap.
inline Species choose_species(unsigned map,unsigned seconds,bool horde_event,float horde,
                              const std::array<unsigned,species_count>& counts,unsigned group_roll,unsigned pick_roll) {
    const auto& roster=map_roster(map);
    std::array<Species,4> candidates{};unsigned count=0;
    if(group_roll%100<(horde_event?10u:25u)) {
        for(unsigned i=0;i<roster.specialist_count;++i) {
            const auto entry=roster.specialists[i];
            if(seconds>=entry.unlock_seconds && counts[unsigned(entry.species)]<ordinary_limit(entry.species,horde))
                candidates[count++]=entry.species;
        }
    }
    if(!count) for(auto type:roster.basic)
        if(counts[unsigned(type)]<ordinary_limit(type,horde)) candidates[count++]=type;
    return count?candidates[pick_roll%count]:Species::Count;
}
constexpr Species guard_species(unsigned map) {return map==2?Species::Stalfos:map==3?Species::ShadowClub:Species::Bokoblin;}
constexpr float elite_scale(Species type) {
    switch(type) {
    case Species::KingBulblin:case Species::DeathSword:case Species::TwilitBloat:
    case Species::PhantomZant:case Species::Darkhammer:case Species::PuppetZelda:case Species::Ganondorf:
    case Species::Helmasaurus:case Species::Dynalfos:
    case Species::StallordHead:case Species::ZantHand:case Species::Aeralfos:case Species::Darknut:case Species::Bullbo:return 1.f;
    default:return 2.f;
    }
}
} // namespace dawnlight::survivors
