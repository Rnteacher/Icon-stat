#include "CollectionPopup.hpp"
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/binding/GJItemIcon.hpp>
#include <Geode/binding/ItemInfoPopup.hpp>

namespace iconstat {
using namespace geode::prelude;

static CCNode* categoryGraphic(size_t index, float scale = .62f) {
    auto graphic = GJItemIcon::createBrowserItem(CATEGORIES[index].unlockType, 1);
    graphic->setScale(scale);
    return graphic;
}

CollectionPopup* CollectionPopup::create(Collection collection) {
    auto ret = new CollectionPopup();
    if (ret->init(std::move(collection))) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

bool CollectionPopup::init(Collection collection) {
    if (!Popup::init(420.f, 270.f)) return false;
    m_collection = std::move(collection);
    setTitle(fmt::format("{}'s Icons", m_collection.username));

    auto total = CCLabelBMFont::create(fmt::format("{} unlocked", m_collection.total()).c_str(), "bigFont.fnt");
    total->setScale(.45f);
    total->setPosition({210.f, 221.f});
    m_mainLayer->addChild(total);

    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    m_mainLayer->addChild(menu);
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        float x = 58.f + static_cast<float>(i % 5) * 76.f;
        float y = 166.f - static_cast<float>(i / 5) * 76.f;
        auto holder = CCNode::create();
        holder->setContentSize({62.f, 62.f});
        auto icon = categoryGraphic(i);
        icon->setPosition({31.f, 39.f});
        holder->addChild(icon);
        auto count = CCLabelBMFont::create(fmt::format("{}", m_collection.icons[i].size()).c_str(), "bigFont.fnt");
        count->setScale(.28f);
        count->setPosition({31.f, 6.f});
        holder->addChild(count);
        auto name = CCLabelBMFont::create(CATEGORIES[i].title, "goldFont.fnt");
        name->setScale(.28f);
        name->setPosition({31.f, 18.f});
        holder->addChild(name);
        auto button = CCMenuItemSpriteExtra::create(holder, this, menu_selector(CollectionPopup::onCategory));
        button->setTag(static_cast<int>(i));
        button->setPosition({x, y});
        button->setID(fmt::format("{}-category", CATEGORIES[i].key));
        menu->addChild(button);
    }
    auto view = ButtonSprite::create("View All", 80, true, "bigFont.fnt", "GJ_button_01.png", 28.f, .6f);
    auto button = CCMenuItemSpriteExtra::create(view, this, menu_selector(CollectionPopup::onViewAll));
    button->setPosition({210.f, 28.f});
    button->setID("view-all-button");
    menu->addChild(button);
    return true;
}

void CollectionPopup::onCategory(CCObject* sender) {
    auto index = static_cast<size_t>(static_cast<CCNode*>(sender)->getTag());
    if (index < CATEGORIES.size()) GalleryPopup::create(m_collection, index)->show();
}

void CollectionPopup::onViewAll(CCObject*) { GalleryPopup::create(m_collection, std::nullopt)->show(); }

GalleryPopup* GalleryPopup::create(Collection collection, std::optional<size_t> category) {
    auto ret = new GalleryPopup();
    if (ret->init(std::move(collection), category)) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

bool GalleryPopup::init(Collection collection, std::optional<size_t> category) {
    if (!Popup::init(440.f, 280.f)) return false;
    m_collection = std::move(collection);
    m_category = category;
    setTitle(category ? fmt::format("{} - {}", m_collection.username, CATEGORIES[*category].title)
                      : fmt::format("{} - All Icons", m_collection.username));
    auto scroll = geode::ScrollLayer::create(CCRect(25.f, 28.f, 390.f, 210.f));
    scroll->setID("icon-grid-scroll");
    m_mainLayer->addChild(scroll);

    constexpr float cell = 58.f;
    constexpr int columns = 6;
    float requiredHeight = 12.f;
    size_t entryCount = 0;
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        if (category && *category != i) continue;
        if (!category) requiredHeight += 26.f;
        requiredHeight += std::max(1, static_cast<int>((m_collection.icons[i].size() + columns - 1) / columns)) * cell;
        entryCount += m_collection.icons[i].size();
    }
    float height = std::max(210.f, requiredHeight);
    scroll->m_contentLayer->setContentSize({390.f, height});
    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->setContentSize({390.f, height});
    scroll->m_contentLayer->addChild(menu);
    float cursorY = height - 12.f;
    for (size_t i = 0; i < CATEGORIES.size(); ++i) {
        if (category && *category != i) continue;
        if (!category) {
            auto header = CCLabelBMFont::create(CATEGORIES[i].title, "goldFont.fnt");
            header->setScale(.42f);
            header->setAnchorPoint({0.f, .5f});
            header->setPosition({10.f, cursorY - 12.f});
            scroll->m_contentLayer->addChild(header);
            cursorY -= 26.f;
        }
        int rows = std::max(1, static_cast<int>((m_collection.icons[i].size() + columns - 1) / columns));
        for (size_t n = 0; n < m_collection.icons[i].size(); ++n) {
            int id = m_collection.icons[i][n];
            float x = 36.f + static_cast<float>(n % columns) * cell;
            float y = cursorY - 30.f - static_cast<float>(n / columns) * cell;
            if (id == m_collection.equipped[i]) {
                auto selected = CCSprite::createWithSpriteFrameName("GJ_select_001.png");
                selected->setPosition({x, y});
                selected->setScale(.72f);
                selected->setColor({90, 255, 90});
                selected->setID("equipped-highlight");
                menu->addChild(selected, -1);
            }
            auto item = GJItemIcon::createBrowserItem(CATEGORIES[i].unlockType, id);
            item->setScale(.58f);
            auto button = CCMenuItemSpriteExtra::create(item, this, menu_selector(GalleryPopup::onIcon));
            button->setTag(id);
            button->setUserObject(CCInteger::create(static_cast<int>(i)));
            button->setPosition({x, y});
            menu->addChild(button);
        }
        cursorY -= rows * cell;
    }
    scroll->scrollToTop();
    auto scrollbar = geode::Scrollbar::create(scroll);
    scrollbar->setID("icon-grid-scrollbar");
    scrollbar->setPosition({422.f, 133.f});
    m_mainLayer->addChild(scrollbar, 2);
    if (entryCount == 0) {
        auto empty = CCLabelBMFont::create("No unlocked icons", "goldFont.fnt");
        empty->setScale(.55f);
        empty->setPosition({195.f, 105.f});
        scroll->m_contentLayer->addChild(empty);
    }
    return true;
}

void GalleryPopup::onIcon(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto category = static_cast<size_t>(static_cast<CCInteger*>(button->getUserObject())->getValue());
    if (category < CATEGORIES.size()) ItemInfoPopup::create(button->getTag(), CATEGORIES[category].unlockType)->show();
}
} // namespace iconstat
