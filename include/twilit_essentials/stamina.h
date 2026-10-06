#pragma once

#include <mods/service.hpp>

#define TWILIT_ESSENTIALS_STAMINA_SERVICE_ID "com.dusklight.twilit_essentials.stamina"
#define TWILIT_ESSENTIALS_STAMINA_SERVICE_MAJOR 1u
#define TWILIT_ESSENTIALS_STAMINA_SERVICE_MINOR 3u

struct TwilitEssentialsStaminaState {
    uint32_t struct_size;
    uint32_t enabled;
    uint32_t gameplay;
    uint32_t exhausted;
    float current;
    float maximum;
    float recover_at;
    float regen_delay;
};

#define TWILIT_ESSENTIALS_STAMINA_STATE_V1_0_SIZE 24u

#define TWILIT_ESSENTIALS_STAMINA_STATE_INIT \
    {sizeof(TwilitEssentialsStaminaState), 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f}

enum TwilitEssentialsStaminaSource : uint32_t {
    TWILIT_ESSENTIALS_STAMINA_SOURCE_ATTACK = 0,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_JUMP_ATTACK = 1,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SPIN_ATTACK = 2,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SPIN_CHARGE = 3,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_HIDDEN_SKILL = 4,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_ROLL = 5,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SIDESTEP = 6,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_BLOCK = 7,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_CLIMB = 8,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_CRAWL = 9,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_HANG = 10,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SWIM = 11,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_PUSH_PULL = 12,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT = 13,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SWIM_SPRINT = 14,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_DASH = 15,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT = 16,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_BULLET_TIME = 17,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_BOW_DRAW = 18,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SLINGSHOT = 19,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_BALL_AND_CHAIN = 20,
};

struct TwilitEssentialsStaminaSourceInfo {
    uint32_t struct_size;
    uint32_t enabled;
    float cost_multiplier;
};

#define TWILIT_ESSENTIALS_STAMINA_SOURCE_INFO_INIT \
    {sizeof(TwilitEssentialsStaminaSourceInfo), 0, 0.0f}

struct TwilitEssentialsStaminaService {
    ServiceHeader header;
    ModResult (*get_state)(ModContext* caller, TwilitEssentialsStaminaState* out_state);
    ModResult (*try_consume)(ModContext* caller, float amount);
    ModResult (*drain)(ModContext* caller, float amount);
    ModResult (*restore)(ModContext* caller, float amount);
    ModResult (*deny)(ModContext* caller);
    ModResult (*get_source)(ModContext* caller, uint32_t source,
        TwilitEssentialsStaminaSourceInfo* out_info);
};

MOD_DECLARE_SERVICE(TwilitEssentialsStaminaService, svc_te_stamina,
    TWILIT_ESSENTIALS_STAMINA_SERVICE_ID, TWILIT_ESSENTIALS_STAMINA_SERVICE_MAJOR,
    TWILIT_ESSENTIALS_STAMINA_SERVICE_MINOR);
