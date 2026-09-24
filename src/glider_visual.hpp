#pragma once
#include "mods/svc/actor.h"
class daAlink_c;
namespace dawnlight {
void init_glider_visual();
void queue_glider_visual(daAlink_c* link);
void clear_glider_visual();
void queue_glider_reward_visual(daAlink_c* link, ActorId item);
void clear_glider_reward_visual();
}
