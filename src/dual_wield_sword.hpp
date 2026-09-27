#pragma once

namespace dawnlight {
enum class SecondSword { Wooden = 0, Ordon = 1, Master = 2 };

struct SecondSwordAssets {
    const char* name;
    const char* archive;
    const char* sword;
    const char* sheath;
    const char* icon;
    bool environmentMapped;
    float bladeLength;
};

// Native resources; no dependency on Collection-Lib slot IDs or another mod.
inline constexpr SecondSwordAssets second_sword_assets(SecondSword sword) {
    switch (sword) {
    case SecondSword::Wooden:
        return {"Wooden Sword", "/res/Object/Kmdl.arc", "al_SWB.bmd", nullptr,
                "im_kinobou_48.bti", false, 100.0f};
    case SecondSword::Master:
        return {"Master Sword", "/res/Object/Alink.arc", "al_swm.bmd", "al_podm.bmd",
                "ni_mastersword_48.bti", true, 120.0f};
    default:
        return {"Ordon Sword", "/res/Object/Alink.arc", "al_swa.bmd", "al_poda.bmd",
                "tt_kokirinoken_s3_tc.bti", false, 100.0f};
    }
}
} // namespace dawnlight
