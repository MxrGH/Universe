#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class UniverseInfoPopup final : public FLAlertLayer {
protected:
    void onClose(CCObject*) {
        this->keyBackClicked();
    }

public:
    static UniverseInfoPopup* create() {
        auto ret = new UniverseInfoPopup();
        if (ret && ret->init(360.f, 220.f, "GJ_square01.png")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(float width, float height, char const* bg) {
        if (!FLAlertLayer::init(width, height, bg)) return false;

        this->m_noElasticity = true;
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto title = CCLabelBMFont::create("UNIVERSE", "bigFont.fnt");
        title->setScale(0.75f);
        title->setPosition({width / 2.f, height - 34.f});
        m_mainLayer->addChild(title);

        auto body = CCLabelBMFont::create(
            "Universe v0.1.0\n\nOpen Geode > Universe > Settings to configure features.\nSafe Mode is enabled by default.",
            "goldFont.fnt"
        );
        body->setScale(0.42f);
        body->setAlignment(kCCTextAlignmentCenter);
        body->setPosition({width / 2.f, height / 2.f - 8.f});
        m_mainLayer->addChild(body);

        auto close = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_okBtn_001.png"),
            this,
            menu_selector(UniverseInfoPopup::onClose)
        );
        auto menu = CCMenu::create();
        menu->addChild(close);
        menu->setPosition({width / 2.f, 28.f});
        m_mainLayer->addChild(menu);

        return true;
    }
};

class $modify(UniverseMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) return true;

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(UniverseMenuLayer::openUniverse)
        );
        button->setID("universe-open-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void openUniverse(CCObject*) {
        if (auto popup = UniverseInfoPopup::create()) {
            popup->show();
        }
    }
};

$on_mod(Loaded) {
    log::info("Universe loaded. Safe Mode defaults to enabled.");
}
