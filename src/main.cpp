#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace {
    enum class Page {
        Player, Practice, Level, Visuals, Recorder, Keybinds, Settings
    };

    void playUniverseClick() {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("click-sound")) return;
        auto audioPath = mod->getResourcesDir() / "click.mp3";
        FMODAudioEngine::sharedEngine()->playEffect(audioPath.string());
    }

    bool isUniverseNoclipActive() {
        auto mod = Mod::get();
        auto layer = PlayLayer::get();
        return mod
            && layer
            && mod->getSettingValue<bool>("practice-tools")
            && mod->getSettingValue<bool>("noclip");
    }

    const char* pageName(Page page) {
        switch (page) {
            case Page::Player: return "Player";
            case Page::Practice: return "Practice";
            case Page::Level: return "Level";
            case Page::Visuals: return "Visuals";
            case Page::Recorder: return "Recorder";
            case Page::Keybinds: return "Keybinds";
            case Page::Settings: return "Settings";
        }
        return "Universe";
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

    bool init() override { return CCLayer::init(); }

    bool ccTouchBegan(CCTouch*, CCEvent*) override {
        playUniverseClick();
        return false;
    }
};

class UniversePanel : public CCLayer {
protected:
    Page m_page = Page::Player;
    CCMenu* m_menu = nullptr;
    CCLabelBMFont* m_pageTitle = nullptr;
    CCLayer* m_content = nullptr;

    void addText(const char* text, CCPoint pos, float scale = 0.42f) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});
        label->setScale(scale);
        label->setPosition(pos);
        m_content->addChild(label);
    }

    void addAction(const char* text, int tag, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "goldFont.fnt");
        label->setScale(0.38f);
        auto item = CCMenuItemLabel::create(label, this, menu_selector(UniversePanel::onAction));
        item->setTag(tag);
        item->setPosition(pos);
        m_menu->addChild(item);
    }

    void addToggle(const char* title, const char* setting, int tag, float y) {
        auto mod = Mod::get();
        bool enabled = mod && mod->getSettingValue<bool>(setting);
        auto line = fmt::format("{}: {}", title, enabled ? "ON" : "OFF");
        addText(line.c_str(), {-245.f, y});
        addAction(enabled ? "TURN OFF" : "TURN ON", tag, {205.f, y});
    }

    void showPage(Page page) {
        m_page = page;
        if (m_content) {
            m_content->removeFromParentAndCleanup(true);
            m_content = nullptr;
        }
        m_content = CCLayer::create();
        m_content->setPosition({0.f, 0.f});
        this->addChild(m_content, 3);

        m_pageTitle->setString(pageName(page));

        switch (page) {
            case Page::Player:
                addText("Player utilities", {-245.f, 105.f});
                addText("Player profiles and appearance tools will live here.", {-245.f, 65.f}, 0.34f);
                addText("Icon preview is local-only; it does not change online profiles.", {-245.f, 25.f}, 0.32f);
                addToggle("Local icon preview", "icon-preview", 101, -35.f);
                break;
            case Page::Practice:
                addText("Practice & testing", {-245.f, 105.f});
                addText("Use these options for local practice sessions.", {-245.f, 65.f}, 0.34f);
                addToggle("Practice tools", "practice-tools", 102, 15.f);
                addToggle("Noclip (all modes)", "noclip", 103, -45.f);
                addText("Noclip works in both Normal and Practice Mode.", {-245.f, -110.f}, 0.30f);
                addText("Safe Mode ON marks the run as Practice Mode to avoid saving it as a normal record.", {-245.f, -140.f}, 0.27f);
                break;
            case Page::Level:
                addText("Level tools", {-245.f, 105.f});
                addText("Level information and start-position helpers are planned.", {-245.f, 65.f}, 0.34f);
                addText("No verification, coin, or leaderboard spoofing is provided.", {-245.f, 25.f}, 0.32f);
                break;
            case Page::Visuals:
                addText("Visual tools", {-245.f, 105.f});
                addText("Visual customization controls are planned for a later update.", {-245.f, 65.f}, 0.34f);
                addText("This page currently has no active visual modifications.", {-245.f, 25.f}, 0.32f);
                break;
            case Page::Recorder:
                addText("Recorder", {-245.f, 105.f});
                addText("Replay and recording integration is planned.", {-245.f, 65.f}, 0.34f);
                addText("No recording is running in this build.", {-245.f, 25.f}, 0.32f);
                break;
            case Page::Keybinds:
                addText("Keybinds", {-245.f, 105.f});
                addText("Custom keybind editing is planned.", {-245.f, 65.f}, 0.34f);
                addText("The main menu button opens Universe for now.", {-245.f, 25.f}, 0.32f);
                break;
            case Page::Settings:
                addText("Universe settings", {-245.f, 105.f});
                addToggle("Global click sound", "click-sound", 104, 45.f);
                addToggle("Safe Mode", "safe-mode", 105, -15.f);
                addText("When ON with Noclip, Universe marks the current run as Practice Mode.", {-245.f, -85.f}, 0.29f);
                addText("You can switch Safe Mode ON or OFF at any time.", {-245.f, -115.f}, 0.30f);
                break;
        }
    }

    void onAction(CCObject* sender) {
        auto node = static_cast<CCNode*>(sender);
        int tag = node->getTag();
        const char* key = nullptr;
        switch (tag) {
            case 101: key = "icon-preview"; break;
            case 102: key = "practice-tools"; break;
            case 103: key = "noclip"; break;
            case 104: key = "click-sound"; break;
            case 105: key = "safe-mode"; break;
            default: return;
        }
        auto mod = Mod::get();
        if (!mod) return;
        bool current = mod->getSettingValue<bool>(key);

        mod->setSettingValue<bool>(key, !current);
        showPage(m_page);
    }

    void closePanel(CCObject*) {
        this->removeFromParentAndCleanup(true);
    }

public:
    static UniversePanel* create() {
        auto ret = new UniversePanel();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto dim = CCLayerColor::create({8, 10, 22, 225});
        dim->setContentSize(win);
        dim->setPosition({0.f, 0.f});
        this->addChild(dim, 0);

        auto panel = CCLayerColor::create({24, 28, 48, 255});
        panel->setContentSize({win.width * 0.82f, win.height * 0.78f});
        panel->setPosition({win.width * 0.09f, win.height * 0.11f});
        this->addChild(panel, 1);

        auto title = CCLabelBMFont::create("UNIVERSE", "bigFont.fnt");
        title->setScale(0.72f);
        title->setPosition({win.width * 0.5f, win.height * 0.81f});
        this->addChild(title, 2);

        m_pageTitle = CCLabelBMFont::create("Player", "goldFont.fnt");
        m_pageTitle->setScale(0.55f);
        m_pageTitle->setPosition({win.width * 0.5f, win.height * 0.72f});
        this->addChild(m_pageTitle, 3);

        m_menu = CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        this->addChild(m_menu, 5);

        float left = win.width * 0.16f;
        float tabY = win.height * 0.64f;
        Page pages[] = {Page::Player, Page::Practice, Page::Level, Page::Visuals,
                        Page::Recorder, Page::Keybinds, Page::Settings};
        for (int i = 0; i < 7; ++i) {
            auto label = CCLabelBMFont::create(pageName(pages[i]), "bigFont.fnt");
            label->setScale(0.34f);
            auto item = CCMenuItemLabel::create(label, this, menu_selector(UniversePanel::onTab));
            item->setTag(200 + i);
            item->setPosition({left + i * (win.width * 0.113f), tabY});
            m_menu->addChild(item);
        }

        auto closeLabel = CCLabelBMFont::create("CLOSE", "goldFont.fnt");
        closeLabel->setScale(0.42f);
        auto close = CCMenuItemLabel::create(closeLabel, this, menu_selector(UniversePanel::closePanel));
        close->setPosition({win.width * 0.87f, win.height * 0.81f});
        m_menu->addChild(close);

        showPage(Page::Player);
        return true;
    }

    void onTab(CCObject* sender) {
        int index = static_cast<CCNode*>(sender)->getTag() - 200;
        if (index < 0 || index > 6) return;
        showPage(static_cast<Page>(index));
    }
};

// Noclip works in Normal and Practice Mode. With Safe Mode enabled, the
// PlayLayer hook below marks the run as Practice Mode before gameplay updates.
class $modify(UniversePlayerObject, PlayerObject) {
    bool collidedWithObjectInternal(float dt, GameObject* object, CCRect rect, bool skipCheck) {
        if (isUniverseNoclipActive()) return false;
        return PlayerObject::collidedWithObjectInternal(dt, object, rect, skipCheck);
    }

    void collidedWithSlopeInternal(float dt, GameObject* object, bool forced) {
        if (isUniverseNoclipActive()) return;
        PlayerObject::collidedWithSlopeInternal(dt, object, forced);
    }
};

class $modify(UniversePlayLayer, PlayLayer) {
    void update(float dt) {
        auto mod = Mod::get();
        if (mod
            && mod->getSettingValue<bool>("safe-mode")
            && mod->getSettingValue<bool>("noclip")
            && !this->m_isPracticeMode) {
            // Keep the modified attempt out of normal completion records.
            this->m_isPracticeMode = true;
        }
        PlayLayer::update(dt);
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
            sprite, this, menu_selector(UniverseMenuLayer::openUniverse)
        );
        button->setID("universe-open-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void openUniverse(CCObject*) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;
        auto panel = UniversePanel::create();
        if (panel) scene->addChild(panel, 10000);
    }
};

$on_mod(Loaded) {
    auto delegate = UniverseTouchDelegate::create();
    if (!delegate) {
        log::error("Universe could not create its global touch delegate.");
        return;
    }
    delegate->retain();
    CCDirector::sharedDirector()->getTouchDispatcher()
        ->addTargetedDelegate(delegate, -128, false);
    log::info("Universe loaded. Global touch sound is registered.");
}
