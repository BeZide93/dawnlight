cmake_minimum_required(VERSION 3.25)
project(dawnlight_mod CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "a775418c667f748724c24283b9bad62ebe2dc48b")
endif ()
# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

add_mod(dawnlight_mod
    FEATURES game
    SOURCES
        src/aim_hooks.cpp
        src/config.cpp
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
