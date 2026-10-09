#pragma once

#include "Collection.hpp"
#include <Geode/ui/Popup.hpp>

namespace iconstat {
class CollectionPopup final : public geode::Popup {
public:
    static CollectionPopup* create(Collection collection);
    bool init(Collection collection);

private:
    Collection m_collection;
    void onCategory(cocos2d::CCObject* sender);
    void onViewAll(cocos2d::CCObject* sender);
};

class GalleryPopup final : public geode::Popup {
public:
    static GalleryPopup* create(Collection collection, std::optional<size_t> category);
    bool init(Collection collection, std::optional<size_t> category);

private:
    Collection m_collection;
    std::optional<size_t> m_category;
    void onIcon(cocos2d::CCObject* sender);
};
} // namespace iconstat
