#include "Collection.hpp"
#include <argon/argon.hpp>

namespace iconstat {
int Collection::total() const {
    int value = 0;
    for (auto const& category : icons) value += static_cast<int>(category.size());
    return value;
}

matjson::Value Collection::toUploadJSON() const {
    matjson::Value root = matjson::Value::object();
    root["schemaVersion"] = 1;
    root["gameVersion"] = "2.2081";
    root["accountId"] = accountID;
    root["userId"] = userID;
    root["username"] = username;
    matjson::Value values = matjson::Value::object();
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        matjson::Value list = matjson::Value::array();
        for (int id : icons[i]) list.push(id);
        values[CATEGORIES[i].key] = std::move(list);
    }
    root["icons"] = std::move(values);
    matjson::Value selected = matjson::Value::object();
    for (size_t i = 0; i < CATEGORIES.size(); ++i) selected[CATEGORIES[i].key] = equipped[i];
    root["equipped"] = std::move(selected);
    return root;
}

Result<Collection> Collection::fromPublicJSON(matjson::Value const& json) {
    if (json["schemaVersion"].asInt().unwrapOr(0) != 1 || json["gameVersion"].asString().unwrapOr("") != "2.2081") {
        return Err("Unsupported collection schema");
    }
    Collection out;
    out.accountID = static_cast<int>(json["accountId"].asInt().unwrapOr(0));
    out.username = json["username"].asString().unwrapOr("");
    out.synchronizedAt = json["synchronizedAt"].asString().unwrapOr("");
    if (out.accountID <= 0 || out.username.empty()) return Err("Invalid collection identity");
    auto const& source = json["icons"];
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        auto array = source[CATEGORIES[i].key].asArray();
        if (array.isErr()) return Err("Missing category {}", CATEGORIES[i].key);
        for (auto const& item : array.unwrap()) {
            int id = static_cast<int>(item.asInt().unwrapOr(0));
            if (id <= 0) return Err("Invalid icon ID");
            out.icons[i].push_back(id);
        }
        out.equipped[i] = static_cast<int>(json["equipped"][CATEGORIES[i].key].asInt().unwrapOr(0));
        if (out.equipped[i] < 1) return Err("Missing equipped {} icon", CATEGORIES[i].title);
    }
    return Ok(std::move(out));
}

Result<Collection> Collection::collectLocal() {
    auto account = argon::getGameAccountData();
    if (!account.valid()) return Err("Sign in to a Geometry Dash account first");
    Collection out;
    out.accountID = account.accountId;
    out.userID = account.userId;
    out.username = account.username;
    auto gm = GameManager::get();
    out.equipped = {
        gm->getPlayerFrame(), gm->getPlayerShip(), gm->getPlayerBall(),
        gm->getPlayerBird(), gm->getPlayerDart(), gm->getPlayerRobot(),
        gm->getPlayerSpider(), gm->getPlayerSwing(), gm->getPlayerJetpack(),
    };
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        int count = gm->countForType(CATEGORIES[i].iconType);
        if (count <= 0 || count > 10000) return Err("Geometry Dash returned an invalid {} icon count", CATEGORIES[i].title);
        for (int id = 1; id <= count; ++id) {
            if (gm->isIconUnlocked(id, CATEGORIES[i].iconType)) out.icons[i].push_back(id);
        }
        if (std::find(out.icons[i].begin(), out.icons[i].end(), out.equipped[i]) == out.icons[i].end()) {
            return Err("Selected {} icon is not unlocked", CATEGORIES[i].title);
        }
    }
    return Ok(std::move(out));
}

size_t categoryIndex(std::string_view key) {
    for (size_t i = 0; i < CATEGORIES.size(); ++i) if (key == CATEGORIES[i].key) return i;
    return CATEGORIES.size();
}

std::string normalizedServerURL() {
    auto url = Mod::get()->getSettingValue<std::string>("server-url");
    while (!url.empty() && url.back() == '/') url.pop_back();
    bool secure = url.starts_with("https://");
    bool loopback = url.starts_with("http://127.0.0.1:") || url.starts_with("http://localhost:");
    if (!secure && !loopback) return {};
    return url;
}
} // namespace iconstat
