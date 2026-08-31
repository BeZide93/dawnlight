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

# Dawnlight's touch aim compatibility hooks mirror Dusklight's private touch-control ABI.
# Desktop Linux has no touch UI, so keep the private host headers out of that build.
set(DAWNLIGHT_PRIVATE_TOUCH_UI_HEADERS ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DAWNLIGHT_PRIVATE_TOUCH_UI_HEADERS OFF)
endif()

if(DAWNLIGHT_PRIVATE_TOUCH_UI_HEADERS)
    # Fetch only the matching UI headers; the mod resolves all host functions at runtime.
    include(FetchContent)
    FetchContent_Declare(dawnlight_rmlui_headers
        URL https://github.com/mikke89/RmlUi/archive/f9b8c9e2935d5df2c7dff2c190d3968e99b0c3dc.tar.gz
        URL_HASH SHA256=42b830abc2509a8c07cf02ba3313505de3a4642725654f489402022d8fefdd79
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        SOURCE_SUBDIR dawnlight-headers-only
    )
    FetchContent_Declare(dawnlight_sdl3_headers
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.10.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        SOURCE_SUBDIR dawnlight-headers-only
    )
    FetchContent_MakeAvailable(dawnlight_rmlui_headers dawnlight_sdl3_headers)
endif()

# Read version from mod.json automatically
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON_CONTENT)
string(JSON DAWNLIGHT_VER GET "${MOD_JSON_CONTENT}" "version")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")
set_source_files_properties(src/mod.cpp src/update_service.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

add_mod(dawnlight_mod
    FEATURES game
    SOURCES
        src/aim_hooks.cpp
        src/bullet_time.cpp
        src/config.cpp
        src/deferred_features.cpp
        src/enemy_scaling.cpp
        src/item_integrity.cpp
        src/item_slot_hooks.cpp
        src/jump_hooks.cpp
        src/manual_shield_hooks.cpp
        src/mod.cpp
        src/model_overlays.cpp
        src/new_save_modes.cpp
        src/save_compat.cpp
        src/ui.cpp
        src/update_service.cpp
    MOD_JSON mod.json
    RES_DIR res
)

target_compile_definitions(dawnlight_mod PRIVATE DAWNLIGHT_VERSION="${DAWNLIGHT_VER}")

if(DAWNLIGHT_PRIVATE_TOUCH_UI_HEADERS)
    target_include_directories(dawnlight_mod PRIVATE
        "${DUSKLIGHT_DIR}/src"
        "${dawnlight_rmlui_headers_SOURCE_DIR}/Include"
        "${dawnlight_sdl3_headers_SOURCE_DIR}/include"
    )
endif()
