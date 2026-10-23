#include "../src/survivors_roster.hpp"
#include <cassert>
#include <set>
#include <cstdint>
using namespace dawnlight::survivors;
int main() {
    static_assert(elite_scale(Species::Stalfos)==2.f);
    const std::array<std::array<Species,4>,5> expected{{
        {Species::Lizalfos,Species::Helmasaur,Species::Dynalfos,Species::Darknut},
        {Species::Bow,Species::Bokoblin,Species::Kargarok,Species::KingBulblin},
        {Species::Stalhound,Species::Stalfos,Species::DeathSword,Species::StallordHead},
        {Species::ShadowBeast,Species::TwilitBloat,Species::ShadowKargarok,Species::PhantomZant},
        {Species::Darknut,Species::Darkhammer,Species::PuppetZelda,Species::Ganondorf}
    }};
    for(unsigned map=0;map<5;++map) {
        const auto& roster=map_roster(map);assert(roster.elites==expected[map]);
        std::array<unsigned,species_count> counts{};
        std::set<Species> seen;
        for(unsigned group=0;group<100;++group)for(unsigned pick=0;pick<12;++pick) {
            auto kind=choose_species(map,0,false,1,counts,group,pick);
            assert(std::find(roster.basic.begin(),roster.basic.end(),kind)!=roster.basic.end());
            seen.insert(kind);
        }
        assert(seen.size()==3);
        for(unsigned seconds=0;seconds<400;++seconds) {
            for(unsigned pick=0;pick<12;++pick) {
                auto kind=choose_species(map,seconds,false,1,counts,0,pick);
                for(unsigned i=0;i<roster.specialist_count;++i)
                    if(kind==roster.specialists[i].species)assert(seconds>=roster.specialists[i].unlock_seconds);
            }
        }
        for(bool horde:{false,true}) {
            unsigned special=0;
            for(unsigned roll=0;roll<10000;++roll) {
                auto kind=choose_species(map,600,horde,1,counts,roll,roll*71);
                special+=std::find(roster.basic.begin(),roster.basic.end(),kind)==roster.basic.end();
            }
            assert(special==(horde?1000u:2500u));
        }
        for(unsigned i=0;i<roster.specialist_count;++i)counts[unsigned(roster.specialists[i].species)]=345;
        for(unsigned pick=0;pick<100;++pick) {
            const auto kind=choose_species(map,600,false,5,counts,0,pick);
            assert(std::find(roster.basic.begin(),roster.basic.end(),kind)!=roster.basic.end());
        }
        // Simulate the maximum horde population, including pending admissions.
        counts={};uint32_t rng=23;
        for(unsigned n=0;n<345;++n) {
            rng=rng*1664525+1013904223;
            const auto kind=choose_species(map,600,false,5,counts,rng,rng>>12);
            assert(kind!=Species::Count);++counts[unsigned(kind)];
            assert(counts[unsigned(kind)]<=ordinary_limit(kind,5));
        }
    }
    assert(map_roster(2).specialist_count==3); // No Stallord-dependent Staltroops.
    assert(ordinary_limit(Species::Bow,1)==4&&ordinary_limit(Species::Bow,5)==12);
    assert(ordinary_limit(Species::Kargarok,1)==2&&ordinary_limit(Species::Kargarok,5)==6);
    assert(ordinary_limit(Species::Gibdo,5)==4&&ordinary_limit(Species::Dynalfos,5)==4);
    assert(elite_scale(Species::Darknut)==1&&elite_scale(Species::Helmasaurus)==1);
    assert(map_roster(3).basic== (std::array{Species::ShadowClub,Species::ShadowBow,Species::ShadowKeese}));
    assert(map_roster(3).specialist_count==2);
    assert(ordinary_limit(Species::ShadowBow,5)==12&&ordinary_limit(Species::ShadowKargarok,5)==6);
    assert(ordinary_limit(Species::ShadowBeast,5)==4);
    assert(guard_species(3)==Species::ShadowClub&&guard_species(2)==Species::Stalfos&&guard_species(1)==Species::Bokoblin);
    assert(elite_scale(Species::StallordHead)==1&&elite_scale(Species::ZantHand)==1);
    assert(elite_scale(Species::Stalhound)==2&&elite_scale(Species::ShadowBeast)==2);
    assert(elite_scale(Species::Tektite)==2&&elite_scale(Species::FireBubble)==2);
}
