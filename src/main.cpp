#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <filesystem>

using namespace geode::prelude;

namespace {
    void playUniverseClick() {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("click-sound")) return;

        auto audioPath = mod->getResourcesDir() / "click.mp3";
        if (!std::filesystem::exists(audioPath)) {
            log::warn("Universe click sound is missing: {}", audioPath.string());
            return;
        }

        FMODAudioEngine::sharedEngine()->playEffect(audioPath.string());
    }
}

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
            "Universe v0.1.0\n\nGlobal click/tap sound is active when enabled in Geode settings.\nSafe Mode remains enabled by default.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    // A non-swallowing global listener hears Android touch input without
    // consuming it, so normal GD buttons and gameplay continue to work.
    auto listener = EventListenerTouchOneByOne::create();
    if (!listener) {
        log::error("Universe could not create its global touch listener.");
        return;
    }

    listener->setSwallowTouches(false);
    listener->onTouchBegan = [](CCTouch*, CCEvent*) {
        playUniverseClick();
        return false;
    };

    CCDirector::sharedDirector()->getEventDispatcher()
        ->addEventListenerWithFixedPriority(listener, -128);

    log::info("Universe loaded. Global touch click sound listener registered.");
}
