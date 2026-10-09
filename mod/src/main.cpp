#include "ApiClient.hpp"
#include "UnlockMonitor.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

static bool s_autoSyncAttemptedThisLaunch = false;

static uint64_t unlockedIconFingerprint(iconstat::Collection const& collection) {
    uint64_t hash = 1469598103934665603ull;
    auto mix = [&hash](uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ull;
    };
    mix(static_cast<uint32_t>(collection.accountID));
    for (size_t category = 0; category < collection.icons.size(); ++category) {
        mix(category + 1);
        mix(collection.icons[category].size());
        for (int id : collection.icons[category]) mix(static_cast<uint32_t>(id));
    }
    return hash;
}

static void notifyResult(Result<std::string> result) {
    if (result.isOk()) Notification::create(result.unwrap(), NotificationIcon::Success)->show();
    else Notification::create(result.unwrapErr(), NotificationIcon::Error)->show();
}

$on_mod(Loaded) {
    ButtonSettingPressedEventV3(Mod::get(), "actions").listen([](std::string_view key) {
        if (key == "sync-now") iconstat::api().synchronize(true, notifyResult);
        else if (key == "delete-published") {
            createQuickPopup("Delete Icon Stat Data", "Delete your published icon collection? This cannot be undone.", "Cancel", "Delete", [](FLAlertLayer*, bool confirmed) {
                if (confirmed) iconstat::api().deletePublished(notifyResult);
            });
        } else if (key == "clear-cache") {
            iconstat::ApiClient::clearCache();
            Notification::create("Icon Stat cache cleared", NotificationIcon::Success)->show();
        }
    }).leak();
}

class $modify(IconStatMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (Mod::get()->getSettingValue<bool>("enabled") && Mod::get()->getSettingValue<bool>("publish") &&
            Mod::get()->getSettingValue<bool>("auto-sync") && !s_autoSyncAttemptedThisLaunch) {
            s_autoSyncAttemptedThisLaunch = true;
            iconstat::api().synchronize(false, [](Result<std::string> result) {
                if (result.isOk()) log::info("{}", result.unwrap());
                else log::warn("Automatic Icon Stat synchronization failed: {}", result.unwrapErr());
            });
        }
        return true;
    }
};

class $modify(IconStatGameManager, GameManager) {
    struct Fields {
        float pollElapsed = 0.f;
        iconstat::UnlockMonitorState monitor;
    };

    void update(float dt) {
        GameManager::update(dt);
        m_fields->monitor.tick(dt);
        m_fields->pollElapsed += dt;
        if (m_fields->pollElapsed < 2.f) return;
        m_fields->pollElapsed = 0.f;

        auto mod = Mod::get();
        bool enabled = mod->getSettingValue<bool>("enabled") &&
            mod->getSettingValue<bool>("publish") &&
            mod->getSettingValue<bool>("sync-new-icons");
        if (!enabled) {
            m_fields->monitor.reset();
            return;
        }

        auto local = iconstat::Collection::collectLocal();
        if (local.isErr()) return;
        auto fingerprint = unlockedIconFingerprint(local.unwrap());
        if (!m_fields->monitor.observe(fingerprint, iconstat::api().isActionInFlight())) return;

        iconstat::api().synchronize(false, [this, fingerprint](Result<std::string> result) {
            bool success = result.isOk();
            m_fields->monitor.complete(success, fingerprint);
            if (success) log::info("Icon Stat synchronized after the unlocked icon collection changed");
            else log::warn("Icon Stat new-icon synchronization failed; retrying later: {}", result.unwrapErr());
        });
    }
};
