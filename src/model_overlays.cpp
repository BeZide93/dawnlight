#include "model_overlays.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "mods/svc/overlay.h"

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace dawnlight {
namespace {

struct ModelOverlayDesc {
    CustomModel model;
    const char* fileName;
    const char* discPath;
};

constexpr std::array<ModelOverlayDesc, static_cast<size_t>(CustomModel::Count)> kModelOverlays = {{
    {CustomModel::OrdonLink, "BMDL.arc", "/res/Object/BMDL.arc"},
    {CustomModel::HeroClothes, "Kmdl.arc", "/res/Object/Kmdl.arc"},
    {CustomModel::ZoraArmor, "Zmdl.arc", "/res/Object/Zmdl.arc"},
    {CustomModel::MagicArmor, "Mmdl.arc", "/res/Object/Mmdl.arc"},
    {CustomModel::WolfLink, "Wmdl.arc", "/res/Object/Wmdl.arc"},
    {CustomModel::SumoLink, "alSumou.arc", "/res/Object/alSumou.arc"},
    {CustomModel::Horse, "Horse.arc", "/res/Object/Horse.arc"},
    {CustomModel::Items, "Alink.arc", "/res/Object/Alink.arc"},
}};

std::array<OverlayHandle, kModelOverlays.size()> s_modelOverlayHandles = {};

void log_model_message(void (*logFn)(ModContext*, const char*), const std::string& message) {
    if (logFn != nullptr) {
        logFn(mod_ctx, message.c_str());
    }
}

bool load_model_file(const std::filesystem::path& path, std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    const std::streamoff fileSize = file.tellg();
    if (fileSize <= 0 || static_cast<uint64_t>(fileSize) >
            static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
        return false;
    }

    data.resize(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    return static_cast<bool>(file.read(reinterpret_cast<char*>(data.data()), fileSize));
}

}  // namespace

void initialize_model_overlays() {
    bool hasCustomModel = false;
    for (const ModelOverlayDesc& model : kModelOverlays) {
        hasCustomModel |= custom_model_enabled(model.model);
    }
    if (!hasCustomModel) {
        return;
    }

    const char* dataDir = nullptr;
    if (svc_host == nullptr || svc_overlay == nullptr ||
        svc_host->data_dir(mod_ctx, &dataDir) != MOD_OK || dataDir == nullptr || *dataDir == '\0')
    {
        svc_log->warn(mod_ctx, "Dawnlight Models: failed to resolve the mods directory");
        return;
    }

    try {
        const std::filesystem::path configRoot =
            std::filesystem::path(dataDir).parent_path().parent_path();
        const std::filesystem::path modsPath = configRoot / "mods";

        for (size_t i = 0; i < kModelOverlays.size(); ++i) {
            const ModelOverlayDesc& model = kModelOverlays[i];
            if (!custom_model_enabled(model.model)) {
                continue;
            }

            const std::filesystem::path sourcePath = modsPath / model.fileName;
            std::vector<uint8_t> data;
            if (!load_model_file(sourcePath, data)) {
                log_model_message(svc_log->info,
                    std::string("Dawnlight Models: mods/") + model.fileName +
                        " not found or unreadable; using vanilla");
                continue;
            }

            OverlayHandle handle = 0;
            const ModResult result = svc_overlay->add_buffer(
                mod_ctx, model.discPath, data.data(), data.size(), &handle);
            if (result != MOD_OK || handle == 0) {
                log_model_message(svc_log->warn,
                    std::string("Dawnlight Models: failed to register mods/") + model.fileName +
                        "; using vanilla");
                continue;
            }

            s_modelOverlayHandles[i] = handle;
            log_model_message(svc_log->info,
                std::string("Dawnlight Models: using mods/") + model.fileName + " for " +
                    model.discPath);
        }
    } catch (const std::exception&) {
        svc_log->warn(mod_ctx, "Dawnlight Models: failed to load custom model overlays");
    }
}

void shutdown_model_overlays() {
    if (svc_overlay == nullptr) {
        return;
    }

    for (OverlayHandle& handle : s_modelOverlayHandles) {
        if (handle == 0) {
            continue;
        }
        if (svc_overlay->remove(mod_ctx, handle) != MOD_OK) {
            svc_log->warn(mod_ctx, "Dawnlight Models: failed to unregister model overlay");
        }
        handle = 0;
    }
}

}  // namespace dawnlight
