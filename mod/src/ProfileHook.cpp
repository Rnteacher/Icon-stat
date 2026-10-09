#include "ApiClient.hpp"
#include "CollectionPopup.hpp"
#include <Geode/modify/ProfilePage.hpp>

using namespace geode::prelude;

class $modify(IconStatProfilePage, ProfilePage) {
    struct Fields {
        std::optional<iconstat::Collection> collection;
    };

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);
        if (!Mod::get()->getSettingValue<bool>("enabled") || !Mod::get()->getSettingValue<bool>("show-profile-stat")) return;
        auto accountID = score->m_accountID;
        iconstat::api().fetch(accountID, [self = WeakRef(this), accountID](Result<std::optional<iconstat::Collection>> result) {
            auto page = self.lock();
            if (!page || page->m_accountID != accountID) return;
            if (result.isErr()) {
                log::warn("Unable to load Icon Stat for account {}: {}", accountID, result.unwrapErr());
                return;
            }
            if (!result.unwrap().has_value()) {
                log::debug("Account {} has not published an Icon Stat collection", accountID);
                return;
            }
            static_cast<IconStatProfilePage*>(page.data())->showIconStat(std::move(*result.unwrap()));
        });
    }

    void showIconStat(iconstat::Collection collection) {
        auto statsMenu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByID("stats-menu"));
        if (!statsMenu) {
            log::warn("Unable to display Icon Stat: ProfilePage stats-menu was not found");
            return;
        }
        if (statsMenu->getChildByID("axolodlaat.icon-stat/profile-stat")) return;
        m_fields->collection = std::move(collection);

        constexpr float statHeight = 42.f;
        auto label = CCLabelBMFont::create(fmt::format("{}", m_fields->collection->total()).c_str(), "bigFont.fnt");
        float vanillaLabelScale = .5f;
        auto referenceHolder = statsMenu->getChildByID("demons-label-container");
        if (!referenceHolder) referenceHolder = statsMenu->getChildByID("stars-label-container");
        CCLabelBMFont* referenceLabel = nullptr;
        if (referenceHolder) {
            referenceLabel = referenceHolder->getChildByType<CCLabelBMFont>(0);
            if (referenceLabel) vanillaLabelScale = referenceLabel->getScale();
        }
        label->setScale(vanillaLabelScale);
        if (referenceLabel) label->setAnchorPoint(referenceLabel->getAnchorPoint());
        auto labelSize = label->getScaledContentSize();
        auto labelHolder = CCNode::create();
        float labelHeight = referenceHolder ? referenceHolder->getContentHeight() : labelSize.height;
        labelHolder->setContentSize({labelSize.width, labelHeight});
        if (referenceLabel) {
            label->setPosition({labelSize.width * label->getAnchorPoint().x, referenceLabel->getPositionY()});
        } else {
            label->setAnchorPoint({1.f, .5f});
            label->setPosition({labelSize.width, labelHeight / 2.f});
        }
        labelHolder->addChild(label);
        labelHolder->setID("axolodlaat.icon-stat/profile-stat-label");
        auto labelLayoutOptions = AxisLayoutOptions::create()->setScaleLimits(.0f, 1.f);
        if (referenceHolder) {
            if (auto referenceOptions = typeinfo_cast<AxisLayoutOptions*>(referenceHolder->getLayoutOptions())) {
                labelLayoutOptions->setNextGap(referenceOptions->getNextGap());
            }
        }
        labelHolder->setLayoutOptions(labelLayoutOptions);

        auto icon = GJItemIcon::createBrowserItem(UnlockType::Cube, 1);
        icon->setScale(.46f);
        auto iconSize = icon->getScaledContentSize();
        auto iconHolder = CCNode::create();
        iconHolder->setContentSize({iconSize.width, statHeight});
        icon->setPosition({iconSize.width / 2.f, statHeight / 2.f});
        iconHolder->addChild(icon);
        auto iconButton = CCMenuItemSpriteExtra::create(iconHolder, this, menu_selector(IconStatProfilePage::onIconStat));
        iconButton->setID("axolodlaat.icon-stat/profile-stat");
        iconButton->setLayoutOptions(AxisLayoutOptions::create()->setScaleLimits(.65f, 1.f)->setNextGap(7.f));

        statsMenu->addChild(labelHolder);
        statsMenu->addChild(iconButton);
        statsMenu->updateLayout();
        float labelTargetScale = labelHolder->getScale();
        float iconTargetScale = iconButton->getScale();
        labelHolder->setScale(0.f);
        iconButton->setScale(0.f);
        labelHolder->runAction(CCEaseBackOut::create(CCScaleTo::create(.25f, labelTargetScale)));
        iconButton->runAction(CCSequence::create(
            CCDelayTime::create(.04f),
            CCEaseBackOut::create(CCScaleTo::create(.25f, iconTargetScale)),
            nullptr));
    }

    void onIconStat(CCObject*) {
        if (m_fields->collection) iconstat::CollectionPopup::create(*m_fields->collection)->show();
    }
};
