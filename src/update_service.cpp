#include "update_service.hpp"

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace dawnlight {

bool g_configCheckForUpdatesEnabled = true;

static const LogService* s_logSvc = nullptr;
static ModContext* s_modCtx = nullptr;
static const UiService* s_uiSvc = nullptr;
static const ConfigService* s_configSvc = nullptr;
static ConfigVarHandle s_varCheckForUpdates = 0;

static const char* MOD_CURRENT_VERSION = DAWNLIGHT_VERSION;
static const char* GITHUB_API_URL = "https://api.github.com/repos/BeZide93/dawnlight/releases/latest";

enum DownloadState {
    DL_IDLE = 0,
    DL_IN_PROGRESS,
    DL_SUCCESS,
    DL_FAILED,
    DL_HANDLED
};

static std::mutex s_mutex;
static bool s_updateAvailable = false;
static bool s_dialogShown = false;
static std::string s_latestTagName;
static std::string s_downloadUrl;

static std::chrono::steady_clock::time_point s_updateDetectedTime;
static std::atomic<DownloadState> s_downloadState{DL_IDLE};
static UiDialogHandle s_activeDialogHandle = 0;

static std::string http_get(const std::string& url) {
    std::string response;
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("DawnlightUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return response;
    }

    DWORD timeout = 5000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        "User-Agent: DawnlightModUpdater\r\nAccept: application/vnd.github.v3+json",
        -1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
        0
    );

    if (hConnect) {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response.append(buffer, bytesRead);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hInternet);
#elif defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    (void)url;
#else
    std::string cmd = "curl -s -L -H \"User-Agent: DawnlightModUpdater\" -H \"Accept: application/vnd.github.v3+json\" \"" + url + "\"";
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp) {
        char buffer[4096];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer) - 1, fp)) > 0) {
            buffer[n] = '\0';
            response.append(buffer, n);
        }
        pclose(fp);
    }
#endif
    return response;
}

static bool download_file(const std::string& url, const std::string& destPath) {
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("DawnlightUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }

    DWORD timeout = 15000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        "User-Agent: DawnlightModUpdater",
        -1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
        0
    );

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    FILE* f = std::fopen(destPath.c_str(), "wb");
    if (!f) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead = 0;
    bool success = true;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (std::fwrite(buffer, 1, bytesRead, f) != bytesRead) {
            success = false;
            break;
        }
    }

    std::fclose(f);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return success;
#elif defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    (void)url;
    (void)destPath;
    return false;
#else
    std::string cmd = "curl -s -L -H \"User-Agent: DawnlightModUpdater\" -o \"" + destPath + "\" \"" + url + "\"";
    int ret = std::system(cmd.c_str());
    return ret == 0;
#endif
}

static bool parse_github_release_json(const std::string& json, std::string& outTagName, std::string& outDownloadUrl) {
    outTagName.clear();
    outDownloadUrl.clear();

    size_t tagPos = json.find("\"tag_name\"");
    if (tagPos != std::string::npos) {
        size_t colonPos = json.find(':', tagPos);
        if (colonPos != std::string::npos) {
            size_t openQuote = json.find('"', colonPos);
            if (openQuote != std::string::npos) {
                size_t closeQuote = json.find('"', openQuote + 1);
                if (closeQuote != std::string::npos) {
                    outTagName = json.substr(openQuote + 1, closeQuote - openQuote - 1);
                }
            }
        }
    }

    size_t urlPos = 0;
    while ((urlPos = json.find("\"browser_download_url\"", urlPos)) != std::string::npos) {
        size_t colonPos = json.find(':', urlPos);
        if (colonPos != std::string::npos) {
            size_t openQuote = json.find('"', colonPos);
            if (openQuote != std::string::npos) {
                size_t closeQuote = json.find('"', openQuote + 1);
                if (closeQuote != std::string::npos) {
                    std::string url = json.substr(openQuote + 1, closeQuote - openQuote - 1);
                    if (url.find(".dusk") != std::string::npos || outDownloadUrl.empty()) {
                        outDownloadUrl = url;
                    }
                }
            }
        }
        urlPos += 22;
    }

    return !outTagName.empty() && !outDownloadUrl.empty();
}

static bool is_version_newer(const std::string& latestTag, const std::string& currentVersion) {
    std::string latest = latestTag;
    if (!latest.empty() && (latest[0] == 'v' || latest[0] == 'V')) {
        latest = latest.substr(1);
    }
    std::string current = currentVersion;
    if (!current.empty() && (current[0] == 'v' || current[0] == 'V')) {
        current = current.substr(1);
    }

    int lMaj = 0, lMin = 0, lPat = 0;
    int cMaj = 0, cMin = 0, cPat = 0;

    std::sscanf(latest.c_str(), "%d.%d.%d", &lMaj, &lMin, &lPat);
    std::sscanf(current.c_str(), "%d.%d.%d", &cMaj, &cMin, &cPat);

    if (lMaj > cMaj) return true;
    if (lMaj < cMaj) return false;
    if (lMin > cMin) return true;
    if (lMin < cMin) return false;
    return lPat > cPat;
}

static std::vector<std::string> get_target_mod_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32
    char appData[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appData, MAX_PATH) > 0) {
        std::string modPath = std::string(appData) + "\\TwilitRealm\\Dusklight\\mods\\dawnlight_mod.dusk";
        paths.push_back(modPath);
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
#if defined(__APPLE__)
        std::string modPath = std::string(home) + "/Library/Application Support/TwilitRealm/Dusklight/mods/dawnlight_mod.dusk";
        paths.push_back(modPath);
#else
        std::string modPath = std::string(home) + "/.local/share/TwilitRealm/Dusklight/mods/dawnlight_mod.dusk";
        paths.push_back(modPath);
#endif
    }
#endif

    return paths;
}

static void on_update_confirmed(ModContext*, UiDialogHandle, void*) {
    std::string downloadUrl;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        downloadUrl = s_downloadUrl;
    }

    if (downloadUrl.empty()) {
        s_downloadState = DL_FAILED;
        return;
    }

    s_downloadState = DL_IN_PROGRESS;

    std::thread([downloadUrl]() {
        auto paths = get_target_mod_paths();
        bool success = false;
        for (const auto& path : paths) {
            if (download_file(downloadUrl, path)) {
                success = true;
            }
        }
        if (success) {
            s_downloadState = DL_SUCCESS;
        } else {
            s_downloadState = DL_FAILED;
        }
    }).detach();
}

static void start_version_check_thread() {
    std::thread([]() {
        std::string json = http_get(GITHUB_API_URL);
        if (json.empty()) {
            return;
        }

        std::string tagName, downloadUrl;
        if (parse_github_release_json(json, tagName, downloadUrl)) {
            if (is_version_newer(tagName, MOD_CURRENT_VERSION)) {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_latestTagName = tagName;
                s_downloadUrl = downloadUrl;
                s_updateDetectedTime = std::chrono::steady_clock::now();
                s_updateAvailable = true;

                if (s_logSvc && s_modCtx) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[Updater] Update available: %s (Current: %s)", tagName.c_str(), MOD_CURRENT_VERSION);
                    s_logSvc->info(s_modCtx, buf);
                }
            }
        }
    }).detach();
}

ModResult init_update_service(const LogService* log_svc, ModContext* mod_ctx, const UiService* ui_svc, const ConfigService* config_svc, ConfigVarHandle var_handle) {
    s_logSvc = log_svc;
    s_modCtx = mod_ctx;
    s_uiSvc = ui_svc;
    s_configSvc = config_svc;
    s_varCheckForUpdates = var_handle;

    s_updateAvailable = false;
    s_dialogShown = false;
    s_downloadState = DL_IDLE;

    if (g_configCheckForUpdatesEnabled) {
        start_version_check_thread();
    }

    return MOD_OK;
}

void update_update_service(const LogService* log_svc, ModContext* mod_ctx, const UiService* ui_svc) {
    if (!g_configCheckForUpdatesEnabled || !ui_svc || !mod_ctx) {
        return;
    }

    if (s_updateAvailable && !s_dialogShown) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_updateDetectedTime).count() < 1000) {
            return;
        }

        std::string latestTag;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            latestTag = s_latestTagName;
        }

        static std::string s_promptRml;
        s_promptRml = "A new version of <b style=\"color: #d278ff;\">Dawnlight</b> is available on GitHub!<br/><br/>"
                      "Current Version: <b>v" + std::string(MOD_CURRENT_VERSION) + "</b><br/>"
                      "Latest Release: <b>" + latestTag + "</b><br/><br/>"
                      "Would you like to automatically download and update the mod file now?";

        static UiDialogAction s_actions[2] = {UI_DIALOG_ACTION_INIT, UI_DIALOG_ACTION_INIT};
        s_actions[0].struct_size = sizeof(UiDialogAction);
        s_actions[0].label = "Yes";
        s_actions[0].on_pressed = on_update_confirmed;
        s_actions[0].user_data = nullptr;
        s_actions[0].keep_open = false;

        s_actions[1].struct_size = sizeof(UiDialogAction);
        s_actions[1].label = "No";
        s_actions[1].on_pressed = nullptr;
        s_actions[1].user_data = nullptr;
        s_actions[1].keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Mod Update Available";
        desc.body_rml = s_promptRml.c_str();
        desc.variant = UI_DIALOG_NORMAL;
        desc.icon = "question-mark";
        desc.actions = s_actions;
        desc.action_count = 2;

        ModResult res = ui_svc->dialog_push(mod_ctx, &desc, &s_activeDialogHandle);
        if (res == MOD_OK) {
            s_dialogShown = true;
            if (log_svc) {
                log_svc->info(mod_ctx, "[Updater] Update dialog shown successfully.");
            }

            // Also push a subtle toast notification
            static std::string s_toastRml;
            s_toastRml = "Update available: <b>" + latestTag + "</b>";
            UiToastDesc toast = UI_TOAST_DESC_INIT;
            toast.type = "warning";
            toast.title_rml = "Dawnlight";
            toast.body_rml = s_toastRml.c_str();
            toast.duration_ms = 6000;
            ui_svc->push_toast(mod_ctx, &toast);
        }
    }

    // Handle download state
    DownloadState state = s_downloadState.load();
    if (state == DL_SUCCESS) {
        std::string latestTag;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            latestTag = s_latestTagName;
        }

        static std::string s_successRml;
        s_successRml = "<b style=\"color: #d278ff;\">Dawnlight</b> has been updated to <b>" + latestTag + "</b>!<br/><br/>"
                       "The mod file was successfully replaced.<br/>"
                       "Please restart Dusklight to apply the update.";

        static UiDialogAction s_okAction = UI_DIALOG_ACTION_INIT;
        s_okAction.struct_size = sizeof(UiDialogAction);
        s_okAction.label = "OK";
        s_okAction.on_pressed = nullptr;
        s_okAction.user_data = nullptr;
        s_okAction.keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Update Complete";
        desc.body_rml = s_successRml.c_str();
        desc.variant = UI_DIALOG_NORMAL;
        desc.icon = "celebration";
        desc.actions = &s_okAction;
        desc.action_count = 1;

        UiDialogHandle hDialog = 0;
        if (ui_svc->dialog_push(mod_ctx, &desc, &hDialog) == MOD_OK) {
            s_downloadState = DL_HANDLED;
        }
    } else if (state == DL_FAILED) {
        static std::string s_failRml;
        s_failRml = "Failed to download the update package from GitHub.<br/>"
                    "Please check your internet connection or update manually.";

        static UiDialogAction s_okAction = UI_DIALOG_ACTION_INIT;
        s_okAction.struct_size = sizeof(UiDialogAction);
        s_okAction.label = "OK";
        s_okAction.on_pressed = nullptr;
        s_okAction.user_data = nullptr;
        s_okAction.keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Update Failed";
        desc.body_rml = s_failRml.c_str();
        desc.variant = UI_DIALOG_DANGER;
        desc.icon = "error";
        desc.actions = &s_okAction;
        desc.action_count = 1;

        UiDialogHandle hDialog = 0;
        if (ui_svc->dialog_push(mod_ctx, &desc, &hDialog) == MOD_OK) {
            s_downloadState = DL_HANDLED;
        }
    }
}

void shutdown_update_service() {
}

}  // namespace dawnlight
