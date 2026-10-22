#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
namespace dawnlight::survivors {
// Values 0/1/2 are the original v1 Empty/Sword/Tunic identifiers.
enum class GearKind : uint8_t {
    Empty, Sword, Tunic, Greatsword, HerosBow, SharpshooterBow,
    StormBoomerang, HurricaneBoomerang, BombBag, SiegeBombs, IronBall, BreakerBall, Count
};
enum class GearSlot { Weapon, Tunic };
enum class GearFamily { None, Sword, Bow, Boomerang, Bombs, Ball, Tunic };
// Perk IDs are persisted: append new entries without shifting existing saves.
enum class GearPerk : uint8_t { None, Wave, Spin, MultiStrike, FireArrows, Piercing, Multishot, Reach, Haste, FireEdge, BombArrow, Hurricane, FireBomb, ChainSpin, Lightning, Count };
inline constexpr std::array gear_perk_names{"", "Sword Wave", "Spin Attack", "Multi Strike", "Fire Arrows", "Piercing", "Multishot", "Reach", "Haste", "Fire Edge", "Bomb Arrow", "Hurricane", "Fire Bomb", "Chain Spin", "Lightning Strike"};
inline constexpr bool gear_perk_legendary(GearPerk perk) {
    return perk>=GearPerk::FireEdge && perk<=GearPerk::ChainSpin;
}
// The last option in each family is its Legendary-only effect.
inline constexpr std::array sword_perks{GearPerk::Wave,GearPerk::Spin,GearPerk::MultiStrike,GearPerk::Lightning,GearPerk::FireEdge};
inline constexpr std::array bow_perks{GearPerk::FireArrows,GearPerk::Piercing,GearPerk::Multishot,GearPerk::BombArrow};
inline constexpr std::array boomerang_perks{GearPerk::Multishot,GearPerk::Reach,GearPerk::Haste,GearPerk::Hurricane};
inline constexpr std::array bomb_perks{GearPerk::Multishot,GearPerk::Reach,GearPerk::Haste,GearPerk::FireBomb};
inline constexpr std::array ball_perks{GearPerk::Multishot,GearPerk::Reach,GearPerk::Haste,GearPerk::ChainSpin};
inline constexpr std::span<const GearPerk> gear_perk_pool(GearFamily family) {
    switch(family) {
    case GearFamily::Sword:return sword_perks;
    case GearFamily::Bow:return bow_perks;
    case GearFamily::Boomerang:return boomerang_perks;
    case GearFamily::Bombs:return bomb_perks;
    case GearFamily::Ball:return ball_perks;
    default:return {};
    }
}
enum class GearStat { None, Damage, Crit, Speed, Reach, Knockback, Piercing, HopReach, Hops, FlightSpeed, BlastRadius, Hearts, Dodge };
struct GearAffix {
    GearStat stat=GearStat::None;
    const char* label="";
    unsigned scale=1;
    bool percent=false, count=false;
};
struct GearDefinition {
    const char* name;
    GearFamily family;
    std::array<GearAffix,3> affixes;
};
inline constexpr std::array<GearDefinition,unsigned(GearKind::Count)> gear_definitions{{
    {"Empty slot",GearFamily::None,{}},
    {"Hero's Sword",GearFamily::Sword,{{{GearStat::Damage,"sword damage"},{GearStat::Crit,"melee critical chance",1,true},{GearStat::Speed,"melee attack speed",1,true}}}},
    {"Tunic",GearFamily::Tunic,{{{GearStat::Hearts,"max hearts"},{GearStat::Dodge,"dodge chance",1,true},{}}}},
    {"Greatsword",GearFamily::Sword,{{{GearStat::Damage,"sword damage"},{GearStat::Reach,"melee reach",2,true},{GearStat::Knockback,"sword knockback",5,true}}}},
    {"Hero's Bow",GearFamily::Bow,{{{GearStat::Damage,"bow damage"},{GearStat::Crit,"bow critical chance",1,true},{GearStat::Speed,"bow attack speed",1,true}}}},
    {"Sharpshooter Bow",GearFamily::Bow,{{{GearStat::Damage,"bow damage"},{GearStat::Reach,"arrow range",5,true},{GearStat::Piercing,"additional pierced targets",1,false,true}}}},
    {"Storm Boomerang",GearFamily::Boomerang,{{{GearStat::Damage,"boomerang damage"},{GearStat::Speed,"boomerang attack speed",1,true},{GearStat::HopReach,"hop range",5,true}}}},
    {"Hurricane Boomerang",GearFamily::Boomerang,{{{GearStat::Damage,"boomerang damage"},{GearStat::Hops,"additional target hops",1,false,true},{GearStat::FlightSpeed,"boomerang flight speed",5,true}}}},
    {"Bomb Bag",GearFamily::Bombs,{{{GearStat::Damage,"bomb damage"},{GearStat::BlastRadius,"explosion radius",2,true},{GearStat::Speed,"bomb throw speed",1,true}}}},
    {"Siege Bombs",GearFamily::Bombs,{{{GearStat::Damage,"bomb damage"},{GearStat::BlastRadius,"explosion radius",2,true},{GearStat::Knockback,"bomb knockback",5,true}}}},
    {"Iron Ball",GearFamily::Ball,{{{GearStat::Damage,"ball and chain damage"},{GearStat::Speed,"ball and chain attack speed",1,true},{GearStat::Reach,"ball and chain range",5,true}}}},
    {"Breaker Ball",GearFamily::Ball,{{{GearStat::Damage,"ball and chain damage"},{GearStat::Knockback,"ball and chain knockback",5,true},{GearStat::FlightSpeed,"ball and chain flight speed",5,true}}}}
}};
struct GearItem {
    GearKind kind=GearKind::Empty;
    // Ordinary affixes store tiers 1..5; count affixes store their actual 1..2.
    uint8_t first=0, second=0, third=0;
    GearPerk perk=GearPerk::None;
    bool operator==(const GearItem&) const = default;
    const GearDefinition& definition() const {
        return gear_definitions[unsigned(kind)<gear_definitions.size()?unsigned(kind):0];
    }
    GearFamily family() const { return definition().family; }
    bool is_weapon() const { return family()!=GearFamily::None && family()!=GearFamily::Tunic; }
    std::array<uint8_t,3> stats() const { return {first,second,third}; }
    bool valid() const {
        if(unsigned(kind)>=gear_definitions.size()||unsigned(perk)>=unsigned(GearPerk::Count))return false;
        if(perk!=GearPerk::None) {
            if(!is_weapon()||rarity()<3)return false;
            const auto pool=gear_perk_pool(family());
            const auto end=pool.end()-(rarity()==4?0:1);
            if(std::find(pool.begin(),end,perk)==end)return false;
        }
        const auto values=stats();
        for(unsigned i=0;i<values.size();++i) {
            const auto& affix=definition().affixes[i];
            if(affix.stat==GearStat::None) {if(values[i])return false;}
            else if(values[i]<1 || values[i]>(affix.count?2:5))return false;
        }
        return true;
    }
    unsigned bonus(GearStat stat) const {
        const auto values=stats();
        for(unsigned i=0;i<values.size();++i)if(definition().affixes[i].stat==stat && stat!=GearStat::None)
            return values[i]*definition().affixes[i].scale;
        return 0;
    }
    unsigned perk_levels() const {return perk==GearPerk::None?0:gear_perk_legendary(perk)?1:rarity()==4?2:1;}
    std::string perk_description() const {
        if(perk==GearPerk::None||unsigned(perk)>=gear_perk_names.size())return {};
        return std::string("Bonus: ")+(!gear_perk_legendary(perk)?"+"+std::to_string(perk_levels())+" ":"")+gear_perk_names[unsigned(perk)];
    }
    unsigned rarity() const {
        unsigned count=0,sum=0;const auto values=stats();
        for(unsigned i=0;i<values.size();++i)if(definition().affixes[i].stat!=GearStat::None) {
            ++count;sum+=definition().affixes[i].count?(values[i]==2?5:1):values[i];
        }
        return !count?0:sum*2>=count*9?4:sum>=count*4?3:sum>=count*3?2:sum>=count*2?1:0;
    }
};
struct GearInventory {
    static constexpr unsigned page_size=20, page_count=2, capacity=page_size*page_count, none=capacity;
    std::array<GearItem,capacity> items{};
    unsigned weapon=none,tunic=none;
    bool operator==(const GearInventory&) const = default;
    // New slot -> old slot. Keep identities even when two items are identical.
    using SlotOrder=std::array<unsigned,capacity>;
    static SlotOrder slot_order() {
        SlotOrder order{};for(unsigned i=0;i<capacity;++i)order[i]=i;return order;
    }
    SlotOrder sorted_order(bool byType) const {
        auto order=slot_order();
        std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b){
            const auto& x=items[a];const auto& y=items[b];
            if(x.kind==GearKind::Empty||y.kind==GearKind::Empty)
                return x.kind!=GearKind::Empty&&y.kind==GearKind::Empty;
            if(!byType&&x.rarity()!=y.rarity())return x.rarity()>y.rarity();
            if(x.family()!=y.family())return x.family()<y.family();
            if(x.kind!=y.kind)return x.kind<y.kind;
            return x.rarity()>y.rarity();
        });
        return order;
    }
    bool reorder(const SlotOrder& order) {
        std::array<bool,capacity> seen{};auto next=*this;
        for(unsigned i=0;i<capacity;++i) {
            const auto old=order[i];if(old>=capacity||seen[old])return false;
            seen[old]=true;next.items[i]=items[old];
            if(weapon==old)next.weapon=i;
            if(tunic==old)next.tunic=i;
        }
        *this=next;return true;
    }
    GearItem equipped(GearSlot slot) const {
        const auto i=slot==GearSlot::Weapon?weapon:tunic;
        if(i>=capacity)return {};
        const auto item=items[i];
        return (slot==GearSlot::Weapon?item.is_weapon():item.kind==GearKind::Tunic)?item:GearItem{};
    }
    bool equip(unsigned i) {
        if(i>=capacity||!items[i].valid()||items[i].kind==GearKind::Empty)return false;
        (items[i].is_weapon()?weapon:tunic)=i;return true;
    }
    bool unequip(unsigned i) {
        if(i>=capacity)return false;
        if(weapon==i){weapon=none;return true;}
        if(tunic==i){tunic=none;return true;}
        return false;
    }
    bool erase(unsigned i) {
        if(i>=capacity||items[i].kind==GearKind::Empty)return false;
        if(weapon==i)weapon=none;
        if(tunic==i)tunic=none;
        items[i]={};return true;
    }
    unsigned add(GearItem item) {
        if(!item.valid()||item.kind==GearKind::Empty)return none;
        for(unsigned i=0;i<capacity;++i)if(items[i].kind==GearKind::Empty){items[i]=item;return i;}
        return none;
    }
    static unsigned stat(unsigned roll) {return roll<50?1:roll<75?2:roll<90?3:roll<98?4:5;}
    // Linear interpolation of per-affix weights, in tenths of a percent.
    // Rank 0: 50/25/15/8/2%; rank 10: 20/25/25/20/10%.
    static unsigned quality_stat(unsigned roll,unsigned rank) {
        rank=rank>10?10:rank;
        return roll<500-30*rank?1:roll<750-30*rank?2:
            roll<900-20*rank?3:roll<980-8*rank?4:5;
    }
    // Bounded enumeration keeps the selected rarity exact while the talent
    // still weights the stats inside that rarity. No rejection/retry loop.
    template<class Random> static GearItem roll_quality(GearKind kind,unsigned rarity,unsigned rank,Random& random) {
        rank=std::min(rank,10u);
        const std::array<unsigned,5> weights{500-30*rank,250,150+10*rank,80+12*rank,20+8*rank};
        std::array<GearItem,125> candidates{};std::array<unsigned,125> chances{};
        unsigned count=0,total=0;
        for(unsigned code=0;code<125;++code) {
            GearItem item{kind};std::array<uint8_t*,3> values{&item.first,&item.second,&item.third};
            unsigned remaining=code,weight=1;bool valid=true;
            for(unsigned i=0;i<3;++i) {
                const unsigned digit=remaining%5;remaining/=5;
                const auto& affix=item.definition().affixes[i];
                if(affix.stat==GearStat::None){if(digit)valid=false;continue;}
                if(affix.count&&digit>1){valid=false;continue;}
                *values[i]=uint8_t(digit+1);
                weight*=affix.count?(digit?100+20*rank:900-20*rank):weights[digit];
            }
            if(!valid||item.rarity()!=rarity)continue;
            candidates[count]=item;chances[count++]=weight;total+=weight;
        }
        unsigned choice=random(total);
        for(unsigned i=0;i<count;++i){if(choice<chances[i])return candidates[i];choice-=chances[i];}
        return candidates[0];
    }
    template<class Random> static GearItem roll(Random&& random,unsigned rarityRank=0,bool legendary=false,unsigned twilitProgress=0) {
        rarityRank=rarityRank>10?10:rarityRank;
        // 80% Weapon / 20% Tunic; five equally likely families, two variants each.
        static constexpr std::array kinds{GearKind::Sword,GearKind::Greatsword,GearKind::HerosBow,GearKind::SharpshooterBow,
            GearKind::StormBoomerang,GearKind::HurricaneBoomerang,GearKind::BombBag,GearKind::SiegeBombs,GearKind::IronBall,GearKind::BreakerBall};
        const auto kind=random(5)<4?kinds[random(kinds.size())]:GearKind::Tunic;
        GearItem item{kind};std::array<uint8_t*,3> values{&item.first,&item.second,&item.third};
        const bool promoted=!legendary&&twilitProgress&&random(10000)<std::min(twilitProgress,10000u);
        if(promoted)item=roll_quality(kind,random(4)==0?4u:3u,rarityRank,random);
        else for(unsigned i=0;i<values.size();++i) {
            const auto& affix=item.definition().affixes[i];
            if(affix.stat!=GearStat::None)*values[i]=uint8_t(legendary?(affix.count?2:5):affix.count?(random(100)<90-2*rarityRank?1:2):
                rarityRank?quality_stat(random(1000),rarityRank):stat(random(100)));
        }
        if(item.is_weapon()&&item.rarity()>=3) {
            const auto pool=gear_perk_pool(item.family());
            item.perk=pool[random(unsigned(pool.size())-(item.rarity()==4?0:1))];
        }
        return item;
    }
    // Version 4 adds one validated perk byte per item; old items keep no perk.
    // Version 3 expands to two pages. Versions 1/2 keep their 20 slots and
    // translate the old empty equipped-slot sentinel (20) to the new one (40).
    static constexpr unsigned legacy_capacity=20, legacy_byte_count=12+legacy_capacity*4;
    static constexpr unsigned v3_byte_count=12+capacity*4;
    static constexpr unsigned byte_count=12+capacity*5;
    using Bytes=std::array<uint8_t,byte_count>;
    Bytes encode() const {
        Bytes b{'H','S','G','R',4,uint8_t(weapon),uint8_t(tunic),0};
        for(unsigned i=0;i<capacity;++i){b[8+5*i]=uint8_t(items[i].kind);b[9+5*i]=items[i].first;b[10+5*i]=items[i].second;b[11+5*i]=items[i].third;b[12+5*i]=uint8_t(items[i].perk);}
        uint32_t hash=2166136261u;for(unsigned i=0;i<byte_count-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)b[byte_count-4+i]=uint8_t(hash>>(8*i));
        return b;
    }
    bool decode(std::span<const uint8_t> b) {
        if(b.size()!=byte_count&&b.size()!=v3_byte_count&&b.size()!=legacy_byte_count)return false;
        if(b[0]!='H'||b[1]!='S'||b[2]!='G'||b[3]!='R'||b[7])return false;
        const bool legacy=b[4]==1||b[4]==2;
        if(!legacy&&b[4]!=3&&b[4]!=4)return false;
        const unsigned stride=b[4]==4?5:4;
        const unsigned storedCapacity=legacy?legacy_capacity:capacity;
        if(b.size()!=12+storedCapacity*stride||b[5]>storedCapacity||b[6]>storedCapacity)return false;
        uint32_t hash=2166136261u;for(unsigned i=0;i<b.size()-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)if(b[b.size()-4+i]!=uint8_t(hash>>(8*i)))return false;
        GearInventory next;next.weapon=b[5]==storedCapacity?none:b[5];next.tunic=b[6]==storedCapacity?none:b[6];
        for(unsigned i=0;i<storedCapacity;++i){const auto off=8+stride*i;next.items[i]={GearKind(b[off]),b[off+1],b[off+2],b[off+3],b[4]==4?GearPerk(b[off+4]):GearPerk::None};if(!next.items[i].valid()||(b[4]==1&&b[off]>2))return false;}
        if((next.weapon!=none&&!next.items[next.weapon].is_weapon())||(next.tunic!=none&&next.items[next.tunic].kind!=GearKind::Tunic))return false;
        *this=next;return true;
    }
};
}
