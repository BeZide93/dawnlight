// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#include "boss_rush_common.hpp"
static const BossPartAttachment g_darknutArmorParts[] = {
    {"tn_armor_arm_l.bmd",      8,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_arm_r.bmd",      14, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_chest_b.bmd",    3,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_chest_f.bmd",    3,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_head_b.bmd",     5,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_head_f.bmd",     5,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_shoulder_l.bmd", 11, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_shoulder_r.bmd", 17, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_waist_b.bmd",    26, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_waist_f.bmd",    25, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_waist_l.bmd",    27, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_armor_waist_r.bmd",    28, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_shield.bmd",           9,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_sword_a.bmd",          15, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_sword_b_saya.bmd",     27, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"tn_sword_b.bmd",          -1, 14, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, "B_tn", "tnb_sword_b_pull_a.bck", 0.0f},
};
static constexpr u8 kDarknutArmorPartCount =
    static_cast<u8>(sizeof(g_darknutArmorParts) / sizeof(g_darknutArmorParts[0]));

static const BossPartAttachment g_dangoroParts[] = {
    {"mg_met.bmd", 23, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
};
static constexpr u8 kDangoroPartCount =
    static_cast<u8>(sizeof(g_dangoroParts) / sizeof(g_dangoroParts[0]));

static const BossPartAttachment g_fyrusParts[] = {
    {"fm_core.bmd", 3, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, nullptr, nullptr, 0.0f, nullptr, "core_beat.btk"},
};
static constexpr u8 kFyrusPartCount =
    static_cast<u8>(sizeof(g_fyrusParts) / sizeof(g_fyrusParts[0]));

static const BossPartAttachment g_ganondorfParts[] = {
    {"egnd_sword.bmd", 33, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
};
static constexpr u8 kGanondorfPartCount =
    static_cast<u8>(sizeof(g_ganondorfParts) / sizeof(g_ganondorfParts[0]));

static const BossPartAttachment g_aeralfosParts[] = {
    {"gg_met.bmd",    5,  -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"gg_shield.bmd", 11, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
    {"gg_sword.bmd",  16, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
};
static constexpr u8 kAeralfosPartCount =
    static_cast<u8>(sizeof(g_aeralfosParts) / sizeof(g_aeralfosParts[0]));

static const BossPartAttachment g_morpheelParts[] = {
    {"oh_core.bmd", 0, -1, {0.0f, 0.0f, 520.0f}, {0, 0, 0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd",  8, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd",  9, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 10, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 11, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 12, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 13, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 14, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
    {"oh.bmd", 15, -1, {0,0,0}, {0,0,0}, nullptr, nullptr, 0.0f, "oh_loop.brk", "oh_loop.btk"},
};
static constexpr u8 kMorpheelPartCount =
    static_cast<u8>(sizeof(g_morpheelParts) / sizeof(g_morpheelParts[0]));

static const BossPartAttachment g_deathSwordParts[] = {
    {"va_weapon.bmd", 23, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, nullptr, nullptr, 0.0f, "va_weapon.brk", nullptr},
};
static constexpr u8 kDeathSwordPartCount =
    static_cast<u8>(sizeof(g_deathSwordParts) / sizeof(g_deathSwordParts[0]));

static const BossPartAttachment g_blizzetaParts[] = {
    {"ykw_b.bmd", 0, -1, {0.0f, 580.0f, 0.0f}, {0, 0, 0}, "B_yo", "ykw_b_float.bck", 0.0f, "ykw_b_angry.brk", "ykw_b_float.btk"},
};
static constexpr u8 kBlizzetaPartCount =
    static_cast<u8>(sizeof(g_blizzetaParts) / sizeof(g_blizzetaParts[0]));

static const BossPartAttachment g_puppetZeldaParts[] = {
    {"hzelda_sword.bmd", 28, -1, {0.0f, 0.0f, 0.0f}, {0, 0, 0}},
};
static constexpr u8 kPuppetZeldaPartCount =
    static_cast<u8>(sizeof(g_puppetZeldaParts) / sizeof(g_puppetZeldaParts[0]));

static const cXyz kDiababaFightSpawnPos{4.17f, 5.41f, 2662.34f};

static const cXyz kFyrusFightSpawnPos{-1.0f, 0.0f, 1473.0f};

static const cXyz kDangoroFightSpawnPos{21.83f, 879.22f, 817.38f};

static const cXyz kMorpheelFightSpawnPos{-1193.0f, -24000.0f, -770.0f};

static const cXyz kDeathSwordFightSpawnPos{270.0f, 0.0f, 210.0f};

static const cXyz kStallordFightSpawnPos{-60.0f, 1775.0f, 4449.0f};

static const cXyz kBlizzetaFightSpawnPos{-200.0f, 2.0f, 580.0f};

static const cXyz kDarknutFightSpawnPos{150.0f, -350.0f, 600.0f};

static const cXyz kZantFightSpawnPos{0.0f, 0.0f, 0.0f};

static const cXyz kArmogohmaFightSpawnPos{0.0f, 0.0f, 2391.84f};

static const cXyz kBeastGanonFightSpawnPos{0.0f, 0.0f, -2890.0f};

const BossGalleryEntry g_bossGalleryTable[] = {
    {"Ook",          "Forest Temple",       "E_mk",   "mk.bmd",     nullptr,  "mk_wait.bck",       "D_MN05B", 0, 51, 0, 1.2f,  0.0f,  260.0f},
    {"Diababa",      "Forest Temple",       "B_bq",   "bq.bmd",     nullptr,  "bq_wait01.bck",     "D_MN05A", 0, 50, 0, 0.275f, 0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr,
     &kDiababaFightSpawnPos, static_cast<s16>(0x8000)},
    {"Dangoro",      "Goron Mines",         "E_gob",  "mg.bmd",     nullptr,  "mg_wait.bck",       "D_MN04B", 3, 51, 0, 0.9f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_dangoroParts, kDangoroPartCount, nullptr, nullptr, nullptr, nullptr,
     &kDangoroFightSpawnPos, static_cast<s16>(0x8000)},
    {"Fyrus",        "Goron Mines",         "E_fm",   "fm.bmd",     nullptr,  "fm_wait01.bck",     "D_MN04A", 0, 50, 0, 0.5f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_fyrusParts, kFyrusPartCount, nullptr, "fm.brk", nullptr, "fm.btk",
     &kFyrusFightSpawnPos, static_cast<s16>(0x8000)},
    {"Deku Toad",    "Lakebed Temple",      "E_dt",   "dt.bmd",     nullptr,  "dt_wait01.bck",     "D_MN01B", 0, 51, 0, 0.55f,  0.0f,  260.0f, 150.0f},
    {"Morpheel",     "Lakebed Temple",      "B_oh",   "oi_head.bmd",nullptr,  "",                  "D_MN01A", 0, 50, 0, 0.275f, -165.0f, 240.0f, 60.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_morpheelParts, kMorpheelPartCount,
     nullptr, nullptr, nullptr, nullptr, &kMorpheelFightSpawnPos, static_cast<s16>(0x2A02),
     csXyz(static_cast<s16>(-0x4000), 0, 0)},
    {"Death Sword",  "Arbiter's Grounds",   "E_va",   "va.bmd",     nullptr,  "va_subs_wait.bck",  "D_MN10B", 0, 51, 0, 0.7f,  0.0f, 260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_deathSwordParts, kDeathSwordPartCount,
     nullptr, nullptr, nullptr, nullptr, &kDeathSwordFightSpawnPos, static_cast<s16>(-0x6000)},
    {"Stallord",     "Arbiter's Grounds",   "B_ds",   "ds.bmd",     nullptr,  "ds_wait01_a.bck",   "D_MN10A", 0, 50, 0, 0.14f,  0.0f,  260.0f, 100.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr,
     &kStallordFightSpawnPos, static_cast<s16>(-0x8000)},
    {"Darkhammer",   "Snowpeak Ruins",      "E_th",   "th.bmd",     nullptr,  "th_wait.bck",       "D_MN11B", 0, 51, 0, 0.9f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, "E_th_ball"},
    {"Blizzeta",     "Snowpeak Ruins",      "B_yo",   "yo_core.bmd",nullptr,  "",                  "D_MN11A", 0, 50, 0, 0.3f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_blizzetaParts, kBlizzetaPartCount,
     nullptr, nullptr, nullptr, nullptr, &kBlizzetaFightSpawnPos, static_cast<s16>(0x6AAB)},
    {"Darknut",      "Temple of Time",      "B_tnp",  "tn.bmd",     "B_tn",   "tnb_wait.bck",      "D_MN06B", 0, 51, 0, 1.0f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_darknutArmorParts, kDarknutArmorPartCount,
     nullptr, nullptr, nullptr, nullptr, &kDarknutFightSpawnPos, static_cast<s16>(-0x7000)},
    {"Armogohma",    "Temple of Time",      "B_gm",   "goma.bmd",   nullptr,  "goma_wait.bck",     "D_MN06A", 0, 50, 0, 0.3f,  0.0f,  260.0f, 50.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr,
     &kArmogohmaFightSpawnPos, static_cast<s16>(0x8000)},
    {"Aeralfos",     "City in the Sky",     "B_gg",   "gg.bmd",     nullptr,  "ggb_wait_a.bck",    "D_MN07B", 2, 51, 0, 0.7f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_aeralfosParts, kAeralfosPartCount},
    {"Argorok",      "City in the Sky",     "B_dr",   "dr.bmd",     nullptr,  "dr_pole_stayb.bck",   "D_MN07A", 2, 50, 0, 0.275f, 75.0f, 260.0f},
    {"Zant",         "Palace of Twilight",  "B_zan",  "zan.bmd",    nullptr,  "zan_wait.bck",      "D_MN08D", 0, 53, 0, 1.0f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr,
     &kZantFightSpawnPos, static_cast<s16>(0)},
    {"Puppet Zelda", "Hyrule Castle",       "Hzelda", "hzelda.bmd", nullptr,  "hzelda_fwait.bck",  "D_MN09A", 0, 50, 0, 1.0f,  50.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_puppetZeldaParts, kPuppetZeldaPartCount},
    {"Beast Ganon",      "Hyrule Castle",       "B_mgn",  "mgn.bmd",    nullptr,  "mgn_wait.bck",      "D_MN09A", 2, 50, 1, 0.45f,  0.0f,  260.0f, 100.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr,
     &kBeastGanonFightSpawnPos, static_cast<s16>(0)},
    {"Ganondorf",    "Hyrule Castle",       "B_gnd",  "egnd.bmd",   nullptr,  "egnd_wait02.bck",   "D_MN09B", 1, 0,  0, 0.8f,  0.0f,  260.0f, 0.0f,
     nullptr, nullptr, nullptr, nullptr, nullptr, g_ganondorfParts, kGanondorfPartCount, nullptr, "egnd_core_beat.brk"},
};
const size_t g_bossGalleryCount = sizeof(g_bossGalleryTable) / sizeof(g_bossGalleryTable[0]);

