cmake_minimum_required(VERSION 3.25)
project(dawnlight_mod CXX)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DUSK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to dusk source root")
    option(DUSK_MOD_USE_FULL_TREE "Use full build instead of the minimal mod SDK" OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    if (DUSK_MOD_USE_FULL_TREE)
        add_subdirectory("${DUSK_DIR}" dusk EXCLUDE_FROM_ALL)
    else ()
        add_subdirectory("${DUSK_DIR}/sdk" dusk-sdk EXCLUDE_FROM_ALL)
    endif ()
endif ()

if (NOT DEFINED DUSK_DIR)
    set(DUSK_DIR "${CMAKE_SOURCE_DIR}")
endif ()

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
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/mods"
)

target_include_directories(dawnlight_mod PRIVATE src "${DUSK_DIR}/src")
set(DAWNLIGHT_RMLUI_INCLUDE "" CACHE PATH "Optional RmlUi include directory for Dawnlight mod builds")
set(DAWNLIGHT_SDL3_INCLUDE "" CACHE PATH "Optional SDL3 include directory for Dawnlight mod builds")
foreach (_dawnlight_rmlui_include
        "${DAWNLIGHT_RMLUI_INCLUDE}"
        "${CMAKE_BINARY_DIR}/_deps/rmlui-src/Include")
    if (EXISTS "${_dawnlight_rmlui_include}/RmlUi/Core.h")
        target_include_directories(dawnlight_mod PRIVATE "${_dawnlight_rmlui_include}")
    endif ()
endforeach ()
foreach (_dawnlight_sdl_include
        "${DAWNLIGHT_SDL3_INCLUDE}"
        "${CMAKE_BINARY_DIR}/_deps/sdl3_prebuilt-src/include"
        "${CMAKE_BINARY_DIR}/_deps/sdl-src/include")
    if (EXISTS "${_dawnlight_sdl_include}/SDL3/SDL_touch.h")
        target_include_directories(dawnlight_mod PRIVATE "${_dawnlight_sdl_include}")
    endif ()
endforeach ()
target_link_libraries(dawnlight_mod PRIVATE dusklight_game_headers)
