#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult initialize_collection_service(ModError* error);
void update_collection_service();
bool collection_service_active();
bool collection_service_equipped();
void shutdown_collection_service();
}
