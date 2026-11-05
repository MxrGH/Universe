#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace {
    void playUniverseClick() {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("click-sound")) return;

        auto audioPath = mod->getResourcesDir() / "click.mp3";
        FMODAudioEngine::sharedEngine()->playEffect(audioPath.string());
    }
}

class UniverseTouchDelegate : public CCLayer {
public:
    static UniverseTouchDelegate* create() {
        auto ret = new UniverseTouchDelegate();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        return CCLayer::init();
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override {
        playUniverseClick();
        // Do not claim or swallow the touch; GD receives it normally.
        return false;
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
        FLAlertLayer::create(
            "Universe",
            "Universe v0.1.1\n\nGlobal click/tap sound plays on Android screen touches when enabled in Geode settings.\nSafe Mode remains enabled by default.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    auto delegate = UniverseTouchDelegate::create();
    if (!delegate) {
        log::error("Universe could not create its global touch delegate.");
        return;
    }

    // Keep a reference for the lifetime of the mod. The dispatcher also
    // retains registered delegates, and swallowTouches=false preserves GD input.
    delegate->retain();
    CCDirector::sharedDirector()->getTouchDispatcher()
        ->addTargetedDelegate(delegate, -128, false);

    log::info("Universe loaded. Global Android touch sound is registered.");
}
