#pragma once
#include "mods/api.h"

namespace dawnlight {
ModResult initialize_glider_reward(ModError* error);
bool glider_reward_message_active();
void queue_glider_reward();
void update_glider_reward();
void cancel_glider_reward();
void shutdown_glider_reward();
}
