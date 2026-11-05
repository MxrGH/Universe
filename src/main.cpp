#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

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
        FLAlertLayer::create(
            "Universe",
            "Universe v0.1.0\n\nOpen Geode > Universe > Settings to configure the available options.\nSafe Mode is enabled by default.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    log::info("Universe loaded. Safe Mode defaults to enabled.");
}
