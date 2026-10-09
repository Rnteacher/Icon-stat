#include "ApiClient.hpp"
#include <Geode/ui/Notification.hpp>

namespace iconstat {
using namespace geode::prelude;
using namespace geode::utils;

static Result<argon::AccountData> accountFor(Collection const& collection) {
    auto account = argon::getGameAccountData();
    if (!account.valid()) return Err("Sign in to a Geometry Dash account first");
    if (account.accountId != collection.accountID || account.userId != collection.userID || account.username != collection.username) {
        return Err("Geometry Dash account changed; try again");
    }
    return Ok(std::move(account));
}

static std::string responseError(std::string_view action, web::WebResponse& response) {
    if (auto json = response.json(); json.isOk()) {
        if (auto message = json.unwrap()["message"].asString(); message.isOk() && !message.unwrap().empty()) {
            return fmt::format("{}: {}", action, message.unwrap());
        }
    }
    if (response.code() == 0) return fmt::format("{}: network request failed", action);
    return fmt::format("{} (HTTP {})", action, response.code());
}

ApiClient& api() {
    static ApiClient instance;
    return instance;
}

std::string ApiClient::cacheKey(int accountID) { return fmt::format("cache/{}", accountID); }

void ApiClient::fetch(int accountID, FetchCallback callback) {
    auto key = cacheKey(accountID);
    auto cached = Mod::get()->getSavedValue<std::string>(key);
    auto cachedAt = Mod::get()->getSavedValue<int64_t>(key + "/time", 0);
    auto maxAge = Mod::get()->getSettingValue<int64_t>("cache-minutes") * 60;
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (!cached.empty() && maxAge > 0 && now - cachedAt <= maxAge) {
        if (auto json = matjson::parse(cached); json.isOk()) {
            if (auto parsed = Collection::fromPublicJSON(json.unwrap()); parsed.isOk()) {
                auto collection = std::move(parsed).unwrap();
                if (collection.accountID == accountID) {
                    callback(Ok(std::optional(std::move(collection))));
                    return;
                }
            }
        }
    }

    auto server = normalizedServerURL();
    if (server.empty()) { callback(Err("Icon Stat server URL must use HTTPS")); return; }
    auto url = fmt::format("{}/api/v1/players/{}/icons", server, accountID);
    web::WebRequest request;
    request.timeout(std::chrono::seconds(8)).userAgent("IconStat/1.0 (Geode)");
    m_fetchTask.spawn(request.get(url), [callback = std::move(callback), key, accountID](web::WebResponse response) mutable {
        if (response.code() == 404) { callback(Ok(std::optional<Collection>{})); return; }
        if (!response.ok()) { callback(Err("Server returned HTTP {}", response.code())); return; }
        auto json = response.json();
        if (json.isErr()) { callback(Err("Server returned invalid JSON")); return; }
        auto parsed = Collection::fromPublicJSON(json.unwrap());
        if (parsed.isErr()) { callback(Err(parsed.unwrapErr())); return; }
        auto copy = parsed.unwrap();
        if (copy.accountID != accountID) { callback(Err("Server returned a collection for the wrong account")); return; }
        Mod::get()->setSavedValue(key, json.unwrap().dump());
        Mod::get()->setSavedValue(key + "/time", static_cast<int64_t>(std::time(nullptr)));
        callback(Ok(std::optional(std::move(copy))));
    });
}

void ApiClient::synchronize(bool interactive, ActionCallback callback) {
    if (!Mod::get()->getSettingValue<bool>("publish")) {
        if (callback) callback(Err("Publishing is disabled in Icon Stat settings"));
        return;
    }
    if (normalizedServerURL().empty()) { if (callback) callback(Err("Icon Stat server URL must use HTTPS")); return; }
    auto local = Collection::collectLocal();
    if (local.isErr()) { if (callback) callback(Err(local.unwrapErr())); return; }
    if (interactive) Notification::create("Authenticating Icon Stat...", NotificationIcon::Loading)->show();
    auto collection = std::move(local).unwrap();
    auto account = accountFor(collection);
    if (account.isErr()) { if (callback) callback(Err(account.unwrapErr())); return; }
    if (m_actionInFlight) { if (callback) callback(Err("Another Icon Stat account action is already in progress")); return; }
    m_actionInFlight = true;
    m_actionAuthTask.spawn(argon::startAuth({ .account = std::move(account).unwrap(), .forceStrong = true }), [this, collection = std::move(collection), callback = std::move(callback)](Result<std::string> token) mutable {
        if (token.isErr()) {
            m_actionInFlight = false;
            if (callback) callback(Err("Authentication failed: {}", token.unwrapErr()));
            return;
        }
        web::WebRequest request;
        request.header("Authorization", fmt::format("Bearer {}", token.unwrap()));
        request.timeout(std::chrono::seconds(12)).userAgent("IconStat/1.0 (Geode)").bodyJSON(collection.toUploadJSON());
        m_actionWebTask.spawn(request.put(normalizedServerURL() + "/api/v1/me/icons"), [this, callback = std::move(callback)](web::WebResponse response) mutable {
            m_actionInFlight = false;
            if (!response.ok()) { if (callback) callback(Err(responseError("Synchronization failed", response))); return; }
            if (callback) callback(Ok("Icon collection synchronized"));
        });
    });
}

void ApiClient::deletePublished(ActionCallback callback) {
    if (normalizedServerURL().empty()) { if (callback) callback(Err("Icon Stat server URL must use HTTPS")); return; }
    auto local = Collection::collectLocal();
    if (local.isErr()) { if (callback) callback(Err(local.unwrapErr())); return; }
    auto collection = std::move(local).unwrap();
    auto account = accountFor(collection);
    if (account.isErr()) { if (callback) callback(Err(account.unwrapErr())); return; }
    if (m_actionInFlight) { if (callback) callback(Err("Another Icon Stat account action is already in progress")); return; }
    m_actionInFlight = true;
    m_actionAuthTask.spawn(argon::startAuth({ .account = std::move(account).unwrap(), .forceStrong = true }), [this, collection = std::move(collection), callback = std::move(callback)](Result<std::string> token) mutable {
        if (token.isErr()) {
            m_actionInFlight = false;
            if (callback) callback(Err("Authentication failed: {}", token.unwrapErr()));
            return;
        }
        matjson::Value identity = matjson::Value::object();
        identity["accountId"] = collection.accountID;
        identity["userId"] = collection.userID;
        identity["username"] = collection.username;
        web::WebRequest request;
        request.header("Authorization", fmt::format("Bearer {}", token.unwrap()));
        request.timeout(std::chrono::seconds(12)).userAgent("IconStat/1.0 (Geode)").bodyJSON(identity);
        m_actionWebTask.spawn(request.send("DELETE", normalizedServerURL() + "/api/v1/me/icons"), [this, callback = std::move(callback)](web::WebResponse response) mutable {
            m_actionInFlight = false;
            if (!response.ok()) { if (callback) callback(Err(responseError("Deletion failed", response))); return; }
            clearCache();
            if (callback) callback(Ok("Published collection deleted"));
        });
    });
}

void ApiClient::clearCache() {
    auto saved = Mod::get()->getSaveContainer();
    std::vector<std::string> keys;
    for (auto const& [key, value] : saved) if (key.starts_with("cache/")) keys.push_back(key);
    for (auto const& key : keys) Mod::get()->setSavedValue(key, matjson::Value());
}
} // namespace iconstat
