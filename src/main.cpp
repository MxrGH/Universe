#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>

using namespace geode::prelude;

namespace {
    double g_originalFrameInterval = 0.0;

    struct MacroEvent {
        float time = 0.f;
        int button = 0;
        bool down = false;
    };

    std::vector<MacroEvent> g_macroEvents;
    bool g_macroRecording = false;
    bool g_macroPlayingBack = false;
    PlayLayer* g_macroRecordingLayer = nullptr;
    PlayLayer* g_macroPlaybackLayer = nullptr;
    float g_macroTime = 0.f;
    float g_macroPlaybackTime = 0.f;
    size_t g_macroPlaybackIndex = 0;

    std::string macroFilePath() {
        auto mod = Mod::get();
        if (!mod) return {};
        return (mod->getSaveDir() / "universe-practice-replay.csv").string();
    }

    void stopMacroRecording() {
        g_macroRecording = false;
        auto path = macroFilePath();
        if (path.empty()) return;
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            log::error("Universe could not save the practice replay.");
            return;
        }
        for (auto const& event : g_macroEvents)
            file << event.time << ',' << event.button << ',' << (event.down ? 1 : 0) << '\\n';
        log::info("Universe saved {} practice replay events.", g_macroEvents.size());
    }

    bool startMacroRecording() {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_isPracticeMode || g_macroPlayingBack)
            return false;
        g_macroEvents.clear();
        g_macroTime = 0.f;
        g_macroRecordingLayer = layer;
        g_macroRecording = true;
        return true;
    }

    bool startMacroPlayback() {
        auto layer = PlayLayer::get();
        if (!layer || g_macroRecording)
            return false;
        auto path = macroFilePath();
        if (path.empty()) return false;
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::vector<MacroEvent> loaded;
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream row(line);
            MacroEvent event;
            char comma1 = 0;
            char comma2 = 0;
            int down = 0;
            if (row >> event.time >> comma1 >> event.button >> comma2 >> down
                && comma1 == ',' && comma2 == ',') {
                event.down = down != 0;
                loaded.push_back(event);
            }
        }
        if (loaded.empty()) return false;

        g_macroEvents = std::move(loaded);
        g_macroPlaybackIndex = 0;
        g_macroPlaybackTime = 0.f;
        g_macroPlaybackLayer = layer;
        g_macroPlayingBack = true;
        // Macro playback is always a local practice action.
        layer->m_isPracticeMode = true;
        return true;
    }

    void updateMacroPlayback(PlayLayer* layer, float dt) {
        if (g_macroRecording) {
            if (layer != g_macroRecordingLayer)
                stopMacroRecording();
            else
                g_macroTime += dt;
        }
        if (!g_macroPlayingBack || !layer || layer != g_macroPlaybackLayer || !layer->m_player1) {
            if (g_macroPlayingBack && layer != g_macroPlaybackLayer)
                g_macroPlayingBack = false;
            return;
        }

        g_macroPlaybackTime += dt;
        while (g_macroPlaybackIndex < g_macroEvents.size()
            && g_macroEvents[g_macroPlaybackIndex].time <= g_macroPlaybackTime) {
            auto const& event = g_macroEvents[g_macroPlaybackIndex++];
            auto player = layer->m_player1;
            g_macroPlayingBack = true;
            auto button = static_cast<PlayerButton>(event.button);
            if (event.down)
                player->pushButton(button);
            else
                player->releaseButton(button);
            g_macroPlayingBack = true;
        }

        if (g_macroPlaybackIndex >= g_macroEvents.size())
            g_macroPlayingBack = false;
    }

    void applyFpsBypass() {
        auto mod = Mod::get();
        auto director = CCDirector::sharedDirector();
        if (!mod || !director) return;

        if (g_originalFrameInterval <= 0.0)
            g_originalFrameInterval = director->getAnimationInterval();

        if (mod->getSettingValue<bool>("fps-bypass-enabled")) {
            auto target = mod->getSettingValue<int64_t>("fps-target");
            target = std::clamp<int64_t>(target, 60, 360);
            director->setAnimationInterval(1.0 / static_cast<double>(target));
        } else if (g_originalFrameInterval > 0.0) {
            director->setAnimationInterval(g_originalFrameInterval);
        }
    }

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

    void switchStartPos(int direction) {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_startPosObjects || layer->m_startPosObjects->count() == 0)
            return;

        auto objects = layer->m_startPosObjects;
        int index = objects->indexOfObject(layer->m_startPosObject);
        CCObject* next = nullptr;

        if (direction < 0) {
            if (!layer->m_startPosObject)
                next = objects->lastObject();
            else if (layer->m_startPosObject != objects->firstObject())
                next = objects->objectAtIndex(index - 1);
        } else {
            if (!layer->m_startPosObject)
                next = objects->firstObject();
            else if (layer->m_startPosObject != objects->lastObject())
                next = objects->objectAtIndex(index + 1);
        }

        layer->setActiveStartPosObject(static_cast<StartPosObject*>(next));
        if (layer->isRunning()) {
            if (layer->m_isPracticeMode)
                layer->resetLevelFromStart();
            layer->resetLevel();
            layer->startMusic();
        }
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
            case Page::Practice: {
                auto mod = Mod::get();
                addText("Practice & testing", {-245.f, 105.f});
                addText("Use these options for local practice sessions.", {-245.f, 65.f}, 0.34f);
                addToggle("Practice tools", "practice-tools", 102, 35.f);
                addToggle("Noclip (all modes)", "noclip", 103, -15.f);
                addToggle("Speedhack", "speedhack-enabled", 106, -65.f);
                auto speed = mod->getSettingValue<float>("speedhack-speed");
                auto speedText = fmt::format("Speed: {:.2f}x", speed);
                addText(speedText.c_str(), {-245.f, -110.f}, 0.36f);
                addAction("-", 107, {125.f, -110.f});
                addAction("+", 108, {205.f, -110.f});
                addText("Speedhack changes local gameplay timing.", {-245.f, -145.f}, 0.28f);
                addText("Safe Mode ON marks modified runs as Practice Mode.", {-245.f, -175.f}, 0.28f);
                break;
            }
            case Page::Level: {
                addText("Level tools", {-245.f, 105.f});
                auto layer = PlayLayer::get();
                if (layer && layer->m_startPosObjects) {
                    auto count = layer->m_startPosObjects->count();
                    auto index = layer->m_startPosObject
                        ? layer->m_startPosObjects->indexOfObject(layer->m_startPosObject) + 1
                        : 0;
                    auto positionText = fmt::format("StartPos: {} / {}", index, count);
                    addText(positionText.c_str(), {-245.f, 55.f}, 0.38f);
                    addAction("PREVIOUS", 109, {-120.f, 0.f});
                    addAction("NEXT", 110, {100.f, 0.f});
                } else {
                    addText("Enter a level with StartPos objects to use the switcher.", {-245.f, 55.f}, 0.31f);
                }
                addText("StartPos switching resets the current attempt.", {-245.f, -65.f}, 0.30f);
                addText("No verification, coin, or leaderboard spoofing is provided.", {-245.f, -105.f}, 0.29f);
                break;
            }
            case Page::Visuals: {
                addText("Visual tools", {-245.f, 105.f});
                addToggle("Show object hitboxes", "show-hitboxes", 111, 65.f);
                addToggle("FPS Bypass", "fps-bypass-enabled", 112, 15.f);
                auto target = Mod::get()->getSettingValue<int64_t>("fps-target");
                auto fpsText = fmt::format("FPS target: {}", target);
                addText(fpsText.c_str(), {-245.f, -35.f}, 0.34f);
                addAction("-", 113, {125.f, -35.f});
                addAction("+", 114, {205.f, -35.f});
                addText("FPS Bypass changes the render interval, not physics.", {-245.f, -85.f}, 0.28f);
                break;
            }
            case Page::Recorder: {
                addText("Practice recorder", {-245.f, 105.f});
                const char* status = g_macroRecording ? "RECORDING" : (g_macroPlayingBack ? "PLAYING BACK" : "IDLE");
                addText(status, {-245.f, 60.f}, 0.40f);
                addAction(g_macroRecording ? "STOP & SAVE" : "START RECORDING", 115, {-115.f, 0.f});
                addAction("PLAY SAVED REPLAY", 116, {125.f, 0.f});
                addText("Practice Mode only. Replay is saved locally.", {-245.f, -65.f}, 0.30f);
                addText("File: universe-practice-replay.csv", {-245.f, -105.f}, 0.29f);
                break;
            }
            case Page::Keybinds:
                addText("Keybinds", {-245.f, 105.f});
                addText("Configure these bindings in Geode > Universe > Settings.", {-245.f, 65.f}, 0.32f);
                addText("Ctrl+U  Open / close Universe", {-245.f, 20.f}, 0.34f);
                addText("N  Toggle Noclip", {-245.f, -20.f}, 0.34f);
                addText("M  Toggle Speedhack", {-245.f, -60.f}, 0.34f);
                addText("Ctrl+Shift+S  Toggle Safe Mode", {-245.f, -100.f}, 0.34f);
                addText("H  Toggle object hitboxes", {-245.f, -140.f}, 0.34f);
                addText("F8  Toggle FPS Bypass", {-245.f, -180.f}, 0.34f);
                addText("F6  Start / stop recorder", {-245.f, -220.f}, 0.34f);
                addText("F7  Play saved replay", {-245.f, -260.f}, 0.34f);
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
        auto mod = Mod::get();
        if (!mod) return;

        if (tag == 107 || tag == 108) {
            float speed = mod->getSettingValue<float>("speedhack-speed");
            speed += tag == 108 ? 0.25f : -0.25f;
            speed = std::clamp(speed, 0.25f, 3.0f);
            mod->setSettingValue<float>("speedhack-speed", speed);
            showPage(m_page);
            return;
        }
        if (tag == 109 || tag == 110) {
            switchStartPos(tag == 109 ? -1 : 1);
            showPage(m_page);
            return;
        }
        if (tag == 113 || tag == 114) {
            auto target = mod->getSettingValue<int64_t>("fps-target");
            target += tag == 114 ? 30 : -30;
            target = std::clamp<int64_t>(target, 60, 360);
            mod->setSettingValue<int64_t>("fps-target", target);
            applyFpsBypass();
            showPage(m_page);
            return;
        }
        if (tag == 115) {
            if (g_macroRecording) {
                stopMacroRecording();
            } else if (!startMacroRecording()) {
                FLAlertLayer::create("Universe", "Start recording inside a Practice Mode level. Replay playback must be stopped.", "OK")->show();
            }
            showPage(m_page);
            return;
        }
        if (tag == 116) {
            if (!startMacroPlayback())
                FLAlertLayer::create("Universe", "Could not load the replay. Enter Practice Mode and make sure a saved replay exists.", "OK")->show();
            showPage(m_page);
            return;
        }

        const char* key = nullptr;
        switch (tag) {
            case 101: key = "icon-preview"; break;
            case 102: key = "practice-tools"; break;
            case 103: key = "noclip"; break;
            case 104: key = "click-sound"; break;
            case 105: key = "safe-mode"; break;
            case 106: key = "speedhack-enabled"; break;
            case 111: key = "show-hitboxes"; break;
            case 112: key = "fps-bypass-enabled"; break;
            default: return;
        }
        bool current = mod->getSettingValue<bool>(key);
        mod->setSettingValue<bool>(key, !current);
        if (tag == 112) applyFpsBypass();
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

        auto themeColor = Mod::get()->getSettingValue<ccColor4B>("panel-color");
        auto panel = CCLayerColor::create(themeColor);
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
    bool pushButton(PlayerButton button) {
        auto layer = PlayLayer::get();
        if (g_macroRecording && !g_macroPlayingBack && layer && layer->m_isPracticeMode
            && layer->m_player1 == this) {
            g_macroEvents.push_back({g_macroTime, static_cast<int>(button), true});
        }
        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        auto layer = PlayLayer::get();
        if (g_macroRecording && !g_macroPlayingBack && layer && layer->m_isPracticeMode
            && layer->m_player1 == this) {
            g_macroEvents.push_back({g_macroTime, static_cast<int>(button), false});
        }
        return PlayerObject::releaseButton(button);
    }

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
    struct Fields {
        CCDrawNode* hitboxDrawNode = nullptr;
    };

    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();
        if (!m_fields->hitboxDrawNode) {
            m_fields->hitboxDrawNode = CCDrawNode::create();
            if (m_fields->hitboxDrawNode) {
                m_fields->hitboxDrawNode->setID("universe-hitboxes"_spr);
                this->addChild(m_fields->hitboxDrawNode, 10000);
            }
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto node = m_fields->hitboxDrawNode;
        if (!node) return;
        node->clear();

        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("show-hitboxes"))
            return;

        for (auto object : CCArrayExt<GameObject*>(m_objects)) {
            if (!object || object->m_isDecoration || object->m_isDecoration2)
                continue;
            auto rect = object->getObjectRect();
            node->drawRect(
                rect,
                ccc4f(0.f, 0.f, 0.f, 0.f),
                1.0f,
                ccc4f(1.f, 0.18f, 0.25f, 0.85f)
            );
        }
    }

    void update(float dt) {
        auto mod = Mod::get();
        static PlayLayer* trackedLayer = nullptr;
        static bool attemptUsedCheat = false;

        // Keep a per-level-attempt flag. Turning a cheat off later does not
        // erase the fact that it was used during this run.
        if (trackedLayer != this) {
            trackedLayer = this;
            attemptUsedCheat = false;
        }

        updateMacroPlayback(this, dt);

        if (mod) {
            bool noclipActive = mod->getSettingValue<bool>("practice-tools")
                && mod->getSettingValue<bool>("noclip");
            bool speedhackActive = mod->getSettingValue<bool>("speedhack-enabled");
            if (noclipActive || speedhackActive || g_macroPlayingBack)
                attemptUsedCheat = true;

            // Safe Mode can be toggled at any time. If a cheat was used at any
            // point in this PlayLayer, enabling Safe Mode marks it as Practice.
            if (mod->getSettingValue<bool>("safe-mode")
                && attemptUsedCheat
                && !this->m_isPracticeMode) {
                this->m_isPracticeMode = true;
            }

            if (speedhackActive) {
                float speed = mod->getSettingValue<float>("speedhack-speed");
                dt *= std::clamp(speed, 0.25f, 3.0f);
            }
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
        if (auto existing = scene->getChildByID("universe-panel"_spr)) {
            existing->removeFromParentAndCleanup(true);
            return;
        }
        auto panel = UniversePanel::create();
        if (panel) {
            panel->setID("universe-panel"_spr);
            scene->addChild(panel, 10000);
        }
    }
};

$on_game(Loaded) {
    listenForKeybindSettingPresses("keybind-open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;
        if (auto existing = scene->getChildByID("universe-panel"_spr)) {
            existing->removeFromParentAndCleanup(true);
            return;
        }
        auto panel = UniversePanel::create();
        if (panel) {
            panel->setID("universe-panel"_spr);
            scene->addChild(panel, 10000);
        }
    });

    listenForKeybindSettingPresses("keybind-noclip", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (!mod) return;
        bool enabled = !mod->getSettingValue<bool>("noclip");
        mod->setSettingValue<bool>("noclip", enabled);
        if (enabled) mod->setSettingValue<bool>("practice-tools", true);
    });

    listenForKeybindSettingPresses("keybind-speedhack", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (mod) mod->setSettingValue<bool>("speedhack-enabled", !mod->getSettingValue<bool>("speedhack-enabled"));
    });

    listenForKeybindSettingPresses("keybind-safe-mode", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (mod) mod->setSettingValue<bool>("safe-mode", !mod->getSettingValue<bool>("safe-mode"));
    });

    listenForKeybindSettingPresses("keybind-hitboxes", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (mod) mod->setSettingValue<bool>("show-hitboxes", !mod->getSettingValue<bool>("show-hitboxes"));
    });

    listenForKeybindSettingPresses("keybind-fps-bypass", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (!mod) return;
        mod->setSettingValue<bool>("fps-bypass-enabled", !mod->getSettingValue<bool>("fps-bypass-enabled"));
        applyFpsBypass();
    });

    listenForKeybindSettingPresses("keybind-record", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        if (g_macroRecording) stopMacroRecording();
        else if (!startMacroRecording())
            FLAlertLayer::create("Universe", "Start recording inside a Practice Mode level.", "OK")->show();
    });

    listenForKeybindSettingPresses("keybind-replay", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        if (!startMacroPlayback())
            FLAlertLayer::create("Universe", "Could not load the replay. Enter Practice Mode and make sure a saved replay exists.", "OK")->show();
    });
}

$on_mod(Loaded) {
    applyFpsBypass();
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
