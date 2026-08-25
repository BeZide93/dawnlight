cmake_minimum_required(VERSION 3.26)
project(dawnlight_mod CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "d281c34a65568ee7be2319985f3a290c0ef65abc")
endif ()

# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

# Read version from mod.json automatically
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON_CONTENT)
string(JSON DAWNLIGHT_VER GET "${MOD_JSON_CONTENT}" "version")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")
set_source_files_properties(src/mod.cpp src/update_service.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

add_mod(dawnlight_mod
    FEATURES game
    SOURCES
        src/aim_hooks.cpp
        src/config.cpp
        src/deferred_features.cpp
        src/enemy_scaling.cpp
        src/item_integrity.cpp
        src/item_slot_hooks.cpp
        src/jump_hooks.cpp
        src/manual_shield_hooks.cpp
        src/mod.cpp
        src/new_save_modes.cpp
        src/save_compat.cpp
        src/ui.cpp
        src/update_check.cpp
    MOD_JSON mod.json
    RES_DIR res
)
