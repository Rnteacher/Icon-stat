#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <optional>

namespace iconstat {
using namespace geode::prelude;

struct Category {
    char const* key;
    char const* title;
    IconType iconType;
    UnlockType unlockType;
};

inline constexpr std::array<Category, 9> CATEGORIES {{
    {"cube", "Cube", IconType::Cube, UnlockType::Cube},
    {"ship", "Ship", IconType::Ship, UnlockType::Ship},
    {"ball", "Ball", IconType::Ball, UnlockType::Ball},
    {"ufo", "UFO", IconType::Ufo, UnlockType::Bird},
    {"wave", "Wave", IconType::Wave, UnlockType::Dart},
    {"robot", "Robot", IconType::Robot, UnlockType::Robot},
    {"spider", "Spider", IconType::Spider, UnlockType::Spider},
    {"swing", "Swing", IconType::Swing, UnlockType::Swing},
    {"jetpack", "Jetpack", IconType::Jetpack, UnlockType::Jetpack},
}};

struct Collection {
    int accountID = 0;
    int userID = 0;
    std::string username;
    std::string synchronizedAt;
    std::array<std::vector<int>, CATEGORIES.size()> icons;
    std::array<int, CATEGORIES.size()> equipped {};

    int total() const;
    matjson::Value toUploadJSON() const;
    static geode::Result<Collection> fromPublicJSON(matjson::Value const& json);
    static geode::Result<Collection> collectLocal();
};

size_t categoryIndex(std::string_view key);
std::string normalizedServerURL();
} // namespace iconstat
