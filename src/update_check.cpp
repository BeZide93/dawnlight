#include "update_check.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/ui.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace borealis {

struct AppInfo {
    std::string_view orgName;
    std::string_view appName;
    std::string_view githubOwner;
    std::string_view githubRepo;
    std::string_view discordApplicationId;
};

namespace http {

enum class Error {
    None,
    NoBackend,
    InvalidUrl,
    UnsupportedScheme,
    Timeout,
    TooLarge,
    Network,
};

struct Header {
    std::string name;
    std::string value;
};

struct Request {
    std::string url;
    std::vector<Header> headers;
    std::chrono::milliseconds timeout{10000};
    size_t maxBodyBytes = 1024 * 1024;
};

struct Response {
    int statusCode = 0;
    std::vector<Header> headers;
    std::string body;
};

struct Result {
    Error error = Error::None;
    std::string message;
    Response response;
};

}  // namespace http

namespace update {

enum class Status {
    Disabled,
    UpToDate,
    UpdateAvailable,
    Failed,
};

struct Asset {
    std::string name;
    std::string browserDownloadUrl;
    std::string digest;
};

struct Release {
    std::string tagName;
    std::string name;
    std::string htmlUrl;
    std::string body;
    std::vector<Asset> assets;
};

struct Result {
    Status status = Status::Failed;
    std::string message;
    Release latest;
};

struct Options {
    std::string_view currentVersion;
    bool includePrereleases = false;
    std::function<http::Result(const http::Request&)> fetch;
    std::chrono::milliseconds timeout{10000};
};

}  // namespace update
}  // namespace borealis

namespace dawnlight {
namespace {

constexpr borealis::AppInfo kDawnlightAppInfo{
    .orgName = "BeZide93",
    .appName = "Dawnlight",
    .githubOwner = "BeZide93",
    .githubRepo = "dawnlight",
    .discordApplicationId = "",
};

using CheckLatestReleaseFn = borealis::update::Result (*)(
    const borealis::AppInfo&, const borealis::update::Options&);

CheckLatestReleaseFn s_checkLatestRelease = nullptr;

struct UpdateCheckTask {
    explicit UpdateCheckTask(std::string version) : currentVersion(std::move(version)) {
        worker = std::thread([this] {
            try {
                borealis::update::Options options;
                options.currentVersion = currentVersion;
                result = s_checkLatestRelease(kDawnlightAppInfo, options);
            } catch (const std::exception& e) {
                result = {
                    .status = borealis::update::Status::Failed,
                    .message = std::string("Update check failed: ") + e.what(),
                };
            } catch (...) {
                result = {
                    .status = borealis::update::Status::Failed,
                    .message = "Update check failed with an unknown exception",
                };
            }
            done.store(true, std::memory_order_release);
        });
    }

    ~UpdateCheckTask() {
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool finished() const {
        return done.load(std::memory_order_acquire);
    }

    std::string currentVersion;
    borealis::update::Result result;
    std::atomic_bool done = false;
    std::thread worker;
};

std::unique_ptr<UpdateCheckTask> s_updateCheckTask;
std::optional<borealis::update::Result> s_updateCheckResult;
std::string s_checkedVersion;

bool s_unavailableReported = false;
bool s_resultReported = false;

void push_toast(const char* title, const char* body, const char* type = nullptr) {
    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.type = type;
    toast.title_rml = title;
    toast.body_rml = body;
    svc_ui->push_toast(mod_ctx, &toast);
}

bool resolve_update_checker() {
    if (s_checkLatestRelease != nullptr) {
        return true;
    }

    void* symbol = nullptr;
    const ModResult result = svc_hook->resolve(mod_ctx,
        "borealis::update::check_latest_github_release", &symbol, nullptr);
    if (result != MOD_OK || symbol == nullptr) {
        svc_log->warn(mod_ctx, "Dawnlight update check unavailable: failed to resolve Borealis "
                               "GitHub release checker");
        return false;
    }

    s_checkLatestRelease = reinterpret_cast<CheckLatestReleaseFn>(symbol);
    return true;
}

std::string installed_version() {
    if (svc_host != nullptr && svc_host->mod_version != nullptr) {
        if (const char* version = svc_host->mod_version(mod_ctx); version != nullptr && *version) {
            return version;
        }
    }
    return "1.0.1";
}

std::string release_label(const borealis::update::Release& release) {
    if (!release.name.empty()) {
        return release.name;
    }
    return release.tagName;
}

void report_update_result(const borealis::update::Result& result) {
    s_resultReported = true;
    switch (result.status) {
    case borealis::update::Status::UpdateAvailable: {
        static std::string body;
        body = "Installed: ";
        body += s_checkedVersion;
        body += "\nLatest: ";
        body += release_label(result.latest);
        if (!result.latest.htmlUrl.empty()) {
            body += "\n";
            body += result.latest.htmlUrl;
        }
        push_toast("Dawnlight Update Available", body.c_str());
        break;
    }
    case borealis::update::Status::Disabled:
        push_toast("Dawnlight Update Check Disabled", result.message.c_str(), "warning");
        break;
    case borealis::update::Status::Failed:
        push_toast("Dawnlight Update Check Failed", result.message.c_str(), "warning");
        break;
    case borealis::update::Status::UpToDate:
        {
            const std::string message = std::string("Dawnlight update check: up to date (") +
                                        s_checkedVersion + ")";
            svc_log->info(mod_ctx, message.c_str());
        }
        break;
    }
}

}  // namespace

void update_check_tick() {
    if (!check_for_updates_enabled()) {
        return;
    }

    if (!resolve_update_checker()) {
        if (!s_unavailableReported) {
            s_unavailableReported = true;
            push_toast("Dawnlight Update Check Unavailable",
                "This build does not expose the host update checker.", "warning");
        }
        return;
    }

    if (s_updateCheckTask == nullptr && !s_updateCheckResult.has_value()) {
        s_checkedVersion = installed_version();
        s_updateCheckTask = std::make_unique<UpdateCheckTask>(s_checkedVersion);
    }

    if (s_updateCheckTask != nullptr && s_updateCheckTask->finished()) {
        s_updateCheckResult = std::move(s_updateCheckTask->result);
        s_updateCheckTask.reset();
    }

    if (s_updateCheckResult.has_value() && !s_resultReported) {
        report_update_result(*s_updateCheckResult);
    }
}

void shutdown_update_check() {
    s_updateCheckTask.reset();
}

}  // namespace dawnlight
