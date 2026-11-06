#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
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
    std::vector<PlayerButton> g_macroHeldButtons;

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
        std::stable_sort(g_macroEvents.begin(), g_macroEvents.end(),
            [](MacroEvent const& a, MacroEvent const& b) { return a.time < b.time; });
        for (auto const& event : g_macroEvents)
            file << event.time << ',' << event.button << ',' << (event.down ? 1 : 0) << '\n';
        g_macroRecordingLayer = nullptr;
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
            if (loaded.size() >= 100000) break;
            if (row >> event.time >> comma1 >> event.button >> comma2 >> down
                && comma1 == ',' && comma2 == ','
                && std::isfinite(event.time) && event.time >= 0.f
                && event.button >= 0 && event.button <= 32
                && (down == 0 || down == 1)) {
                event.down = down != 0;
                loaded.push_back(event);
            }
        }
        if (loaded.empty()) return false;

        std::stable_sort(loaded.begin(), loaded.end(),
            [](MacroEvent const& a, MacroEvent const& b) { return a.time < b.time; });
        g_macroEvents = std::move(loaded);
        g_macroPlaybackIndex = 0;
        g_macroPlaybackTime = 0.f;
        g_macroHeldButtons.clear();
        g_macroPlaybackLayer = layer;
        g_macroPlayingBack = true;
        // Macro playback is always a local practice action.
        layer->m_isPracticeMode = true;
        return true;
    }

    void stopMacroPlayback(PlayLayer* layer) {
        if (g_macroPlayingBack && layer && layer == g_macroPlaybackLayer && layer->m_player1) {
            for (auto button : g_macroHeldButtons)
                layer->m_player1->releaseButton(button);
        }
        g_macroHeldButtons.clear();
        g_macroPlayingBack = false;
        g_macroPlaybackIndex = 0;
        g_macroPlaybackLayer = nullptr;
    }

    void updateMacroPlayback(PlayLayer* layer, float dt) {
        if (g_macroRecording) {
            if (layer != g_macroRecordingLayer)
                stopMacroRecording();
            else
                g_macroTime += dt;
        }
        if (!g_macroPlayingBack || !layer || layer != g_macroPlaybackLayer || !layer->m_player1) {
            if (g_macroPlayingBack && layer != g_macroPlaybackLayer) {
                // The old PlayLayer is no longer active. Do not dereference it;
                // just discard the held-button bookkeeping for that scene.
                g_macroHeldButtons.clear();
                g_macroPlayingBack = false;
                g_macroPlaybackIndex = 0;
                g_macroPlaybackLayer = nullptr;
            }
            return;
        }

        g_macroPlaybackTime += dt;
        while (g_macroPlaybackIndex < g_macroEvents.size()
            && g_macroEvents[g_macroPlaybackIndex].time <= g_macroPlaybackTime) {
            auto const& event = g_macroEvents[g_macroPlaybackIndex++];
            auto player = layer->m_player1;
            g_macroPlayingBack = true;
            auto button = static_cast<PlayerButton>(event.button);
            if (event.down) {
                player->pushButton(button);
                if (std::find(g_macroHeldButtons.begin(), g_macroHeldButtons.end(), button)
                    == g_macroHeldButtons.end()) {
                    g_macroHeldButtons.push_back(button);
                }
            } else {
                player->releaseButton(button);
                auto held = std::find(g_macroHeldButtons.begin(), g_macroHeldButtons.end(), button);
                if (held != g_macroHeldButtons.end())
                    g_macroHeldButtons.erase(held);
            }
        }

        if (g_macroPlaybackIndex >= g_macroEvents.size())
            stopMacroPlayback(layer);
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
        Player, Practice, Level, Visuals, Recorder, Keybinds, Settings, Drafts
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
            && layer->m_isPracticeMode
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
            case Page::Drafts: return "Drafts";
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
    CCTextInputNode* m_draftPrompt = nullptr;
    CCMenu* m_menu = nullptr;
    CCLayerColor* m_panelBackground = nullptr;
    CCLayerColor* m_dimBackground = nullptr;
    CCLabelBMFont* m_pageTitle = nullptr;
    CCLayer* m_content = nullptr;
    bool m_isDragging = false;
    CCPoint m_dragTouchStart = {0.f, 0.f};
    CCPoint m_dragPanelStart = {0.f, 0.f};

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
        item->setScale(0.96f);
        m_menu->addChild(item);
    }

    void switchStartPos(int direction) {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_startPosObjects || layer->m_startPosObjects->count() == 0)
            return;

        auto objects = layer->m_startPosObjects;
        int index = objects->indexOfObject(layer->m_startPosObject);
        bool hasCurrent = layer->m_startPosObject && index >= 0;
        CCObject* next = nullptr;

        if (direction < 0) {
            if (!hasCurrent)
                next = objects->lastObject();
            else if (index > 0)
                next = objects->objectAtIndex(index - 1);
        } else {
            if (!hasCurrent)
                next = objects->firstObject();
            else if (static_cast<unsigned int>(index + 1) < objects->count())
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

    void cycleTheme() {
        auto mod = Mod::get();
        if (!mod) return;
        auto current = mod->getSettingValue<int64_t>("theme-preset");
        auto next = (current + 1) % 3;
        mod->setSettingValue<int64_t>("theme-preset", next);
        ccColor4B color = next == 0 ? ccColor4B{24, 28, 48, 255}
            : next == 1 ? ccColor4B{38, 18, 58, 255}
            : ccColor4B{12, 42, 48, 255};
        mod->setSettingValue<ccColor4B>("panel-color", color);
        if (m_panelBackground) {
            m_panelBackground->setColor({color.r, color.g, color.b});
            m_panelBackground->setOpacity(color.a);
        }
    }

    void smartStartPos() {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_startPosObjects || !layer->m_player1)
            return;

        auto objects = layer->m_startPosObjects;
        StartPosObject* best = nullptr;
        float playerX = layer->m_player1->getPositionX();
        float bestDistance = std::numeric_limits<float>::max();

        for (unsigned int i = 0; i < objects->count(); ++i) {
            auto startPos = static_cast<StartPosObject*>(objects->objectAtIndex(i));
            if (!startPos) continue;
            float x = startPos->getPositionX();
            if (x < playerX) continue;
            float distance = x - playerX;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = startPos;
            }
        }

        if (!best && objects->count() > 0)
            best = static_cast<StartPosObject*>(objects->lastObject());
        if (!best) return;

        layer->setActiveStartPosObject(best);
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
        m_content->setPosition({12.f, 0.f});
        m_content->setScale(0.98f);
        this->addChild(m_content, 3);
        m_content->runAction(CCSpawn::create(
            CCEaseBackOut::create(CCScaleTo::create(0.18f, 1.f)),
            CCMoveTo::create(0.16f, {0.f, 0.f}),
            nullptr
        ));

        m_pageTitle->setString(pageName(page));
        m_pageTitle->stopAllActions();
        m_pageTitle->setScale(0.48f);
        m_pageTitle->runAction(CCEaseBackOut::create(CCScaleTo::create(0.18f, 0.55f)));

        switch (page) {
            case Page::Player:
                addText("Your current run", {-245.f, 105.f});
                if (auto layer = PlayLayer::get(); layer && layer->m_player1) {
                    auto mode = layer->m_isPracticeMode ? "Practice Mode" : "Normal Mode";
                    auto modeText = fmt::format("Current mode: {}", mode);
                    auto position = layer->m_player1->getPosition();
                    auto positionText = fmt::format("Position: X {:.0f}  Y {:.0f}", position.x, position.y);
                    auto mod = Mod::get();
                    float speed = mod && mod->getSettingValue<bool>("speedhack-enabled")
                        ? mod->getSettingValue<float>("speedhack-speed") : 1.f;
                    auto speedText = fmt::format("Local speed: {:.2f}x", speed);
                    addText(modeText.c_str(), {-245.f, 55.f}, 0.38f);
                    addText(positionText.c_str(), {-245.f, 10.f}, 0.38f);
                    addText(speedText.c_str(), {-245.f, -35.f}, 0.38f);
                } else {
                    addText("Enter a level to see the current player state.", {-245.f, 55.f}, 0.34f);
                }
                addText("Choose your cube", {-245.f, -90.f}, 0.34f);
                if (auto gm = GameManager::get()) {
                    auto previewID = static_cast<int>(Mod::get()->getSettingValue<int64_t>("preview-icon-id"));
                    auto icon = SimplePlayer::create(0);
                    if (icon) {
                        icon->updatePlayerFrame(previewID, IconType::Cube);
                        icon->setColor(gm->colorForIdx(gm->getPlayerColor()));
                        icon->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
                        icon->setPosition({0.f, -155.f});
                        icon->setScale(1.35f);
                        m_content->addChild(icon);
                    }
                    auto iconText = fmt::format("Preview ID: {}", previewID);
                    addText(iconText.c_str(), {-245.f, -220.f}, 0.32f);
                    addAction("-", 124, {125.f, -220.f});
                    addAction("+", 125, {205.f, -220.f});
                }
                addAction("EQUIP SELECTED", 127, {0.f, -265.f});
                addText("Pick an ID and equip it. Profile sync depends on Geometry Dash saving it.", {-245.f, -300.f}, 0.27f);
                break;
            case Page::Practice: {
                auto mod = Mod::get();
                addText("Practice tools", {-245.f, 105.f});
                addText("Try these settings in Practice Mode before using them in a run.", {-245.f, 65.f}, 0.34f);
                addToggle("Practice tools", "practice-tools", 102, 35.f);
                addToggle("Practice-only noclip", "noclip", 103, -15.f);
                addToggle("Speedhack", "speedhack-enabled", 106, -65.f);
                auto speed = mod->getSettingValue<float>("speedhack-speed");
                auto speedText = fmt::format("Speed: {:.2f}x", speed);
                addText(speedText.c_str(), {-245.f, -110.f}, 0.36f);
                addAction("-", 107, {125.f, -110.f});
                addAction("+", 108, {205.f, -110.f});
                addAction("RESET TO 1.00x", 134, {0.f, -205.f});
                addText("Speedhack changes local gameplay timing.", {-245.f, -145.f}, 0.28f);
                addText("Safe Mode ON marks modified runs as Practice Mode.", {-245.f, -175.f}, 0.28f);
                break;
            }
            case Page::Level: {
                addText("Start positions", {-245.f, 105.f});
                auto layer = PlayLayer::get();
                if (layer && layer->m_startPosObjects) {
                    auto count = layer->m_startPosObjects->count();
                    auto index = layer->m_startPosObject
                        ? layer->m_startPosObjects->indexOfObject(layer->m_startPosObject) + 1
                        : 0;
                    auto positionText = fmt::format("StartPos: {} / {}", index, count);
                    addText(positionText.c_str(), {-245.f, 55.f}, 0.38f);
                    addAction("PREVIOUS", 109, {-150.f, 0.f});
                    addAction("SMART", 117, {0.f, 0.f});
                    addAction("NEXT", 110, {150.f, 0.f});
                } else {
                    addText("This level has no StartPos objects to switch between.", {-245.f, 55.f}, 0.31f);
                }
                addText("Smart picks the next StartPos in front of you.", {-245.f, -40.f}, 0.29f);
                addText("Changing a StartPos restarts the current attempt.", {-245.f, -75.f}, 0.30f);
                if (layer) addAction("RESTART ATTEMPT", 133, {0.f, -135.f});
                addText("These tools do not fake coins, records, or leaderboard results.", {-245.f, -175.f}, 0.29f);
                addText("Draft generator: scripts/level_maker.py", {-245.f, -165.f}, 0.29f);
                addText("Use --offline for templates or configure an Offline API key.", {-245.f, -200.f}, 0.27f);
                addText("Output is a JSON blueprint, not an import-ready level.", {-245.f, -235.f}, 0.27f);
                break;
            }
            case Page::Visuals: {
                addText("Visual helpers", {-245.f, 105.f});
                addToggle("Show object hitboxes", "show-hitboxes", 111, 65.f);
                addToggle("FPS Bypass", "fps-bypass-enabled", 112, 15.f);
                auto target = Mod::get()->getSettingValue<int64_t>("fps-target");
                auto fpsText = fmt::format("FPS target: {}", target);
                addText(fpsText.c_str(), {-245.f, -35.f}, 0.34f);
                addAction("-", 113, {125.f, -35.f});
                addAction("+", 114, {205.f, -35.f});
                addText("Quick FPS targets", {-245.f, -75.f}, 0.30f);
                addAction("120", 135, {-150.f, -108.f});
                addAction("240", 136, {0.f, -108.f});
                addAction("360", 137, {150.f, -108.f});
                addText("Render target only; it does not change game physics.", {-245.f, -145.f}, 0.27f);
                addToggle("Show player path trail", "show-trajectory", 118, -125.f);
                addToggle("Show movement projection", "show-projection", 123, -170.f);
                addAction("CYCLE PANEL THEME", 119, {90.f, -215.f});
                addText("The path preview follows your recent movement.", {-245.f, -245.f}, 0.27f);
                addText("It cannot predict portals, gravity changes, or collisions.", {-245.f, -275.f}, 0.27f);
                break;
            }
            case Page::Recorder: {
                addText("Record a practice run", {-245.f, 105.f});
                const char* status = g_macroRecording ? "RECORDING" : (g_macroPlayingBack ? "PLAYING BACK" : "IDLE");
                addText(status, {-245.f, 60.f}, 0.40f);
                addAction(g_macroRecording ? "STOP & SAVE" : "START RECORDING", 115, {-115.f, 0.f});
                addAction("PLAY SAVED REPLAY", 116, {125.f, 0.f});
                addAction("STOP REPLAY", 120, {-115.f, -55.f});
                addAction("CLEAR SAVED REPLAY", 121, {125.f, -55.f});
                addText("Recording and playback are for Practice Mode. Files stay on this device.", {-245.f, -115.f}, 0.30f);
                addText("File: universe-practice-replay.csv", {-245.f, -150.f}, 0.29f);
                addText("Video helper: scripts/export_video.py", {-245.f, -205.f}, 0.29f);
                addText("Requires FFmpeg and a screen recording.", {-245.f, -240.f}, 0.29f);
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
                addText("J  Toggle path trail     K  Toggle movement projection", {-245.f, -300.f}, 0.31f);
                break;
            case Page::Drafts: {
                addText("Offline level design planner", {-245.f, 105.f});
                addText("Level concept", {-245.f, 72.f}, 0.31f);
                m_draftPrompt = CCTextInputNode::create(430.f, 42.f, "Paste or type your level concept...", "bigFont.fnt");
                if (m_draftPrompt) {
                    m_draftPrompt->setPosition({55.f, 42.f});
                    m_content->addChild(m_draftPrompt, 4);
                }
                addAction("SAVE PROMPT", 141, {-175.f, -35.f});
                addAction("LOAD SAVED", 143, {0.f, -35.f});
                addAction("CLEAR", 142, {175.f, -35.f});
                addAction("GENERATE OFFLINE DRAFT", 145, {0.f, -100.f});
                addText("Creates a local procedural JSON design blueprint.", {-245.f, -155.f}, 0.30f);
                addText("No account, API key, internet, or Offline model is used.", {-245.f, -190.f}, 0.30f);
                addText("The blueprint is a planning aid, not an importable level.", {-245.f, -225.f}, 0.28f);
                addText("Saved files stay in the Universe mod folder.", {-245.f, -260.f}, 0.28f);
                break;
            }
            case Page::Settings:
                addText("Make Universe yours", {-245.f, 105.f});
                addToggle("Global click sound", "click-sound", 104, 75.f);
                addToggle("Safe Mode", "safe-mode", 105, 25.f);
                addToggle("Floating touch button", "floating-button", 122, -25.f);
                addText("Safe Mode marks the current attempt as Practice after Noclip or Speedhack is used.", {-245.f, -80.f}, 0.28f);
                addText("This is a local safeguard, not a server-side guarantee.", {-245.f, -115.f}, 0.28f);
                addText("Quick setups", {-245.f, -165.f}, 0.32f);
                addAction("PRACTICE", 130, {-155.f, -205.f});
                addAction("VISUALS", 131, {0.f, -205.f});
                addAction("CLEAN RUN", 132, {155.f, -205.f});
                addText("Choose a setup to change several options at once.", {-245.f, -245.f}, 0.27f);
                addAction("RESET PANEL POSITION", 146, {0.f, -285.f});
                break;
        }
    }

    void generateOfflineDraft() {
        if (!m_draftPrompt) {
            FLAlertLayer::create("Universe Offline Draft", "The prompt field could not be created. Reopen the Drafts tab and try again.", "OK")->show();
            return;
        }
        std::string prompt = m_draftPrompt->getString();
        if (prompt.empty()) {
            FLAlertLayer::create("Universe Offline Draft", "Enter a level idea first.", "OK")->show();
            return;
        }
        if (prompt.size() > 4000) {
            FLAlertLayer::create("Universe Offline Draft", "Please keep the prompt under 4000 characters.", "OK")->show();
            return;
        }

        auto escapeJSON = [](std::string const& value) {
            std::string out;
            out.reserve(value.size() + 16);
            for (unsigned char ch : value) {
                switch (ch) {
                    case '\\': out += "\\\\"; break;
                    case '"': out += "\\\""; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (ch >= 0x20) out += static_cast<char>(ch);
                        break;
                }
            }
            return out;
        };

        static const char* gameplay[] = {"cube", "ship", "ball", "ufo", "wave", "robot"};
        static const char* objects[] = {"block", "spike", "orb", "saw", "pad", "deco"};
        auto mod = Mod::get();
        if (!mod) return;
        auto path = mod->getSaveDir() / "universe-offline-draft.json";
        std::ofstream file(path.string(), std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            FLAlertLayer::create("Universe Offline Draft", "Universe could not create the draft file.", "OK")->show();
            return;
        }

        file << "{\n"
             << "  \"format\": \"universe-level-draft-v1\",\n"
             << "  \"generator\": \"offline-procedural\",\n"
             << "  \"title\": \"" << escapeJSON(prompt.substr(0, 64)) << "\",\n"
             << "  \"prompt\": \"" << escapeJSON(prompt) << "\",\n"
             << "  \"difficulty\": \"hard\",\n"
             << "  \"style\": \"mixed\",\n"
             << "  \"sections\": [\n";
        for (int section = 0; section < 6; ++section) {
            file << "    {\n"
                 << "      \"name\": \"Section " << (section + 1) << "\",\n"
                 << "      \"gameplay\": \"" << gameplay[section] << "\",\n"
                 << "      \"events\": [\n";
            for (int event = 0; event < 6; ++event) {
                int index = (section * 3 + event) % 6;
                int x = section * 900 + event * 130;
                int y = ((section + event) % 5) * 30;
                file << "        {\"x\": " << x
                     << ", \"y\": " << y
                     << ", \"type\": \"" << objects[index]
                     << "\", \"note\": \"Generated offline from your level concept\"}"
                     << (event == 5 ? "\n" : ",\n");
            }
            file << "      ]\n    }" << (section == 5 ? "\n" : ",\n");
        }
        file << "  ],\n"
             << "  \"notes\": [\"Procedural offline draft; no network or neural model was used.\", "
             << "\"This is a design blueprint, not an import-ready Geometry Dash level.\"]\n"
             << "}\n";
        file.close();

        auto message = fmt::format("Offline draft saved to:\\n{}\\n\\nThis is a procedural blueprint, not a playable level.", path.string());
        FLAlertLayer::create("Universe Offline Draft", message.c_str(), "OK")->show();
    }

    void onAction(CCObject* sender) {
        auto node = static_cast<CCNode*>(sender);
        node->stopAllActions();
        node->runAction(CCSequence::create(
            CCScaleTo::create(0.06f, 1.04f),
            CCScaleTo::create(0.10f, 0.96f),
            nullptr
        ));
        int tag = node->getTag();
        auto mod = Mod::get();
        if (!mod) return;

        if (tag == 134) {
            mod->setSettingValue<float>("speedhack-speed", 1.f);
            showPage(m_page);
            return;
        }
        if (tag >= 135 && tag <= 137) {
            const int64_t targets[] = {120, 240, 360};
            mod->setSettingValue<int64_t>("fps-target", targets[tag - 135]);
            applyFpsBypass();
            showPage(m_page);
            return;
        }
        if (tag == 141) {
            if (!m_draftPrompt || std::string(m_draftPrompt->getString()).empty()) {
                FLAlertLayer::create("Universe Drafts", "Write or paste a prompt before saving it.", "OK")->show();
                return;
            }
            auto path = mod->getSaveDir() / "universe-draft-prompt.txt";
            std::ofstream file(path.string(), std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                FLAlertLayer::create("Universe Offline", "Universe could not save the prompt file.", "OK")->show();
                return;
            }
            file << m_draftPrompt->getString();
            file.close();
            FLAlertLayer::create("Universe Offline", "Prompt saved in the Universe mod folder.", "OK")->show();
            return;
        }
        if (tag == 142) {
            if (m_draftPrompt) m_draftPrompt->setString("");
            return;
        }
        if (tag == 143) {
            if (!m_draftPrompt) return;
            auto path = mod->getSaveDir() / "universe-ai-prompt.txt";
            std::ifstream file(path.string());
            if (!file.is_open()) {
                FLAlertLayer::create("Universe Offline", "No saved prompt was found yet.", "OK")->show();
                return;
            }
            std::string prompt((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (prompt.size() > 4000) prompt.resize(4000);
            m_draftPrompt->setString(prompt.c_str());
            FLAlertLayer::create("Universe Offline", "Saved prompt loaded.", "OK")->show();
            return;
        }
        if (tag == 145) {
            generateOfflineDraft();
            return;
        }
        if (tag == 146) {
            setPosition({0.f, 0.f});
            if (m_dimBackground) m_dimBackground->setPosition({0.f, 0.f});
            mod->setSettingValue<float>("panel-offset-x", 0.f);
            mod->setSettingValue<float>("panel-offset-y", 0.f);
            return;
        }
        if (tag == 133) {
            if (auto layer = PlayLayer::get()) {
                layer->resetLevel();
                layer->startMusic();
            }
            showPage(m_page);
            return;
        }
        if (tag >= 130 && tag <= 132) {
            stopMacroPlayback(PlayLayer::get());
            if (tag == 130) {
                mod->setSettingValue<bool>("practice-tools", true);
                mod->setSettingValue<bool>("safe-mode", true);
                mod->setSettingValue<bool>("noclip", false);
                mod->setSettingValue<bool>("speedhack-enabled", false);
                mod->setSettingValue<bool>("show-hitboxes", false);
                mod->setSettingValue<bool>("show-trajectory", false);
                mod->setSettingValue<bool>("show-projection", false);
                mod->setSettingValue<bool>("fps-bypass-enabled", false);
                applyFpsBypass();
                FLAlertLayer::create("Universe", "Practice setup is ready. Noclip and speedhack are off; Safe Mode is on.", "OK")->show();
            } else if (tag == 131) {
                mod->setSettingValue<bool>("show-hitboxes", true);
                mod->setSettingValue<bool>("show-trajectory", true);
                mod->setSettingValue<bool>("show-projection", true);
                FLAlertLayer::create("Universe", "Visual helpers are on. Turn off anything you don't need from the Visuals page.", "OK")->show();
            } else {
                mod->setSettingValue<bool>("practice-tools", false);
                mod->setSettingValue<bool>("safe-mode", true);
                mod->setSettingValue<bool>("noclip", false);
                mod->setSettingValue<bool>("speedhack-enabled", false);
                mod->setSettingValue<bool>("show-hitboxes", false);
                mod->setSettingValue<bool>("show-trajectory", false);
                mod->setSettingValue<bool>("show-projection", false);
                mod->setSettingValue<bool>("fps-bypass-enabled", false);
                applyFpsBypass();
                g_macroRecording = false;
                FLAlertLayer::create("Universe", "Clean Run setup is ready. Practice modifications and visual overlays are off.", "OK")->show();
            }
            showPage(m_page);
            return;
        }
        if (tag == 127) {
            auto gm = GameManager::get();
            auto iconID = static_cast<int>(mod->getSettingValue<int64_t>("preview-icon-id"));
            if (!gm) return;
            // Set the equipped cube ID without changing the account's unlock flags.
            // Geometry Dash profile visibility depends on the game's normal account-save flow.
            gm->setPlayerFrame(iconID);
            FLAlertLayer::create(
                "Universe",
                "Cube selected. Use Geometry Dash's normal save option to try syncing your profile. This does not unlock the icon on your account.",
                "OK"
            )->show();
            showPage(m_page);
            return;
        }
        if (tag == 124 || tag == 125) {
            auto iconID = mod->getSettingValue<int64_t>("preview-icon-id");
            iconID += tag == 125 ? 1 : -1;
            iconID = std::clamp<int64_t>(iconID, 1, 500);
            mod->setSettingValue<int64_t>("preview-icon-id", iconID);
            showPage(m_page);
            return;
        }
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
        if (tag == 117) {
            smartStartPos();
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
        if (tag == 118) {
            mod->setSettingValue<bool>("show-trajectory", !mod->getSettingValue<bool>("show-trajectory"));
            showPage(m_page);
            return;
        }
        if (tag == 119) {
            cycleTheme();
            FLAlertLayer::create("Universe", "Panel theme preset changed. Reopen Universe to apply it.", "OK")->show();
            return;
        }
        if (tag == 120) {
            stopMacroPlayback(PlayLayer::get());
            showPage(m_page);
            return;
        }
        if (tag == 121) {
            stopMacroPlayback(PlayLayer::get());
            g_macroRecording = false;
            g_macroEvents.clear();
            auto path = macroFilePath();
            if (!path.empty()) {
                std::ofstream file(path, std::ios::out | std::ios::trunc);
                file.close();
            }
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
            case 122: key = "floating-button"; break;
            case 123: key = "show-projection"; break;
            default: return;
        }
        bool current = mod->getSettingValue<bool>(key);
        mod->setSettingValue<bool>(key, !current);
        if (tag == 103 && !current)
            mod->setSettingValue<bool>("practice-tools", true);
        if (tag == 112) applyFpsBypass();
        showPage(m_page);
    }

    void closePanel(CCObject*) {
        this->removeFromParentAndCleanup(true);
    }

public:
    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()
            ->addTargetedDelegate(this, -126, true);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto local = convertToNodeSpace(touch->getLocation());
        if (local.x < win.width * 0.15f || local.x > win.width * 0.82f
            || local.y < win.height * 0.76f || local.y > win.height * 0.88f)
            return false;
        m_isDragging = true;
        m_dragTouchStart = touch->getLocation();
        m_dragPanelStart = getPosition();
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_isDragging) return;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto delta = ccpSub(touch->getLocation(), m_dragTouchStart);
        auto next = ccpAdd(m_dragPanelStart, delta);
        setPosition({
            std::clamp(next.x, -win.width * 0.08f, win.width * 0.08f),
            std::clamp(next.y, -win.height * 0.10f, win.height * 0.10f)
        });
        if (m_dimBackground)
            m_dimBackground->setPosition({-getPositionX(), -getPositionY()});
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_isDragging = false;
        auto mod = Mod::get();
        auto win = CCDirector::sharedDirector()->getWinSize();
        if (mod && win.width > 0.f && win.height > 0.f) {
            mod->setSettingValue<float>("panel-offset-x", getPositionX() / win.width);
            mod->setSettingValue<float>("panel-offset-y", getPositionY() / win.height);
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_isDragging = false;
    }

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
        setContentSize(win);
        setPosition({
            mod->getSettingValue<float>("panel-offset-x") * win.width,
            mod->getSettingValue<float>("panel-offset-y") * win.height
        });
        setTouchEnabled(true);
        auto dim = CCLayerColor::create({8, 10, 22, 225});
        m_dimBackground = dim;
        dim->setContentSize(win);
        dim->setPosition({-getPositionX(), -getPositionY()});
        dim->setOpacity(0);
        dim->runAction(CCFadeTo::create(0.18f, 225));
        this->addChild(dim, 0);

        auto themeColor = Mod::get()->getSettingValue<ccColor4B>("panel-color");
        auto panel = CCLayerColor::create(themeColor);
        m_panelBackground = panel;
        panel->setContentSize({win.width * 0.82f, win.height * 0.78f});
        panel->setPosition({win.width * 0.09f, win.height * 0.11f});
        panel->setScale(0.94f);
        panel->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.f)));
        this->addChild(panel, 1);

        auto panelTitle = Mod::get()->getSettingValue<std::string>("panel-title");
        if (panelTitle.empty()) panelTitle = "UNIVERSE";
        auto title = CCLabelBMFont::create(panelTitle.c_str(), "bigFont.fnt");
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
                        Page::Recorder, Page::Keybinds, Page::Settings, Page::Drafts};
        int pageCount = 7;
        if (Mod::get()->getSettingValue<bool>("drafts-tab-enabled")) pageCount = 8;
        for (int i = 0; i < pageCount; ++i) {
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
        if (index < 0 || index > 7) return;
        if (index == 7 && !Mod::get()->getSettingValue<bool>("drafts-tab-enabled")) return;
        showPage(static_cast<Page>(index));
    }
};

// Noclip works in Normal and Practice Mode. With Safe Mode enabled, the
// PlayLayer hook below marks the run as Practice Mode before gameplay updates.
class $modify(UniversePlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        auto layer = PlayLayer::get();
        if (g_macroRecording && !g_macroPlayingBack && layer && layer->m_isPracticeMode
            && layer->m_player1 == this && g_macroEvents.size() < 100000) {
            g_macroEvents.push_back({g_macroTime, static_cast<int>(button), true});
        }
        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        auto layer = PlayLayer::get();
        if (g_macroRecording && !g_macroPlayingBack && layer && layer->m_isPracticeMode
            && layer->m_player1 == this && g_macroEvents.size() < 100000) {
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

class UniverseFloatingButton : public CCLayer {
    CCPoint m_touchStart = {0.f, 0.f};
    bool m_moved = false;

    void togglePanel() {
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

public:
    static UniverseFloatingButton* create() {
        auto ret = new UniverseFloatingButton();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        setContentSize({76.f, 52.f});
        setTouchEnabled(true);
        auto background = CCLayerColor::create({28, 32, 58, 220}, 76.f, 52.f);
        if (background) this->addChild(background);
        auto buttonText = Mod::get()->getSettingValue<std::string>("floating-button-label");
        if (buttonText.empty()) buttonText = "UNIV";
        auto label = CCLabelBMFont::create(buttonText.c_str(), "goldFont.fnt");
        if (label) {
            label->setScale(0.62f);
            label->setPosition({38.f, 26.f});
            this->addChild(label, 1);
        }
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()
            ->addTargetedDelegate(this, -127, true);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        auto local = convertToNodeSpace(touch->getLocation());
        auto bounds = CCRect(0.f, 0.f, getContentSize().width, getContentSize().height);
        if (!bounds.containsPoint(local)) return false;
        m_touchStart = touch->getLocation();
        m_moved = false;
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        auto current = touch->getLocation();
        if (!m_moved && ccpDistance(current, m_touchStart) > 8.f)
            m_moved = true;
        if (!m_moved) return;

        auto parent = getParent();
        auto local = parent ? parent->convertToNodeSpace(current) : current;
        auto win = CCDirector::sharedDirector()->getWinSize();
        setPosition({
            std::clamp(local.x, 38.f, win.width - 38.f),
            std::clamp(local.y, 26.f, win.height - 26.f)
        });
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_moved) {
            togglePanel();
            return;
        }
        m_moved = false;
        auto mod = Mod::get();
        auto win = CCDirector::sharedDirector()->getWinSize();
        if (mod && win.width > 0.f && win.height > 0.f) {
            mod->setSettingValue<float>("floating-button-x", getPositionX() / win.width);
            mod->setSettingValue<float>("floating-button-y", getPositionY() / win.height);
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_moved = false;
    }
};

class $modify(UniversePlayLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* hitboxDrawNode = nullptr;
        UniverseFloatingButton* floatingButton = nullptr;
        std::vector<CCPoint> trajectoryPoints;
        std::vector<float> trajectoryTimes;
        float trajectoryClock = 0.f;
    };

    void toggleUniversePanel(CCObject*) {
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

    void updateFloatingButton() {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (scene && scene->getChildByID("universe-panel"_spr)) {
            if (m_fields->floatingButton) {
                m_fields->floatingButton->removeFromParentAndCleanup(true);
                m_fields->floatingButton = nullptr;
            }
            return;
        }
        auto mod = Mod::get();
        bool shouldShow = mod && mod->getSettingValue<bool>("floating-button");
        if (!shouldShow) {
            if (m_fields->floatingButton) {
                m_fields->floatingButton->removeFromParentAndCleanup(true);
                m_fields->floatingButton = nullptr;
            }
            return;
        }
        if (m_fields->floatingButton) return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto button = UniverseFloatingButton::create();
        if (!button) return;
        float x = mod->getSettingValue<float>("floating-button-x");
        float y = mod->getSettingValue<float>("floating-button-y");
        button->setPosition({std::clamp(x, 0.05f, 0.95f) * win.width,
                             std::clamp(y, 0.05f, 0.95f) * win.height});
        this->addChild(button, 10001);
        m_fields->floatingButton = button;
    }

    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();
        if (!m_fields->hitboxDrawNode) {
            m_fields->hitboxDrawNode = CCDrawNode::create();
            if (m_fields->hitboxDrawNode) {
                m_fields->hitboxDrawNode->setID("universe-hitboxes"_spr);
                this->addChild(m_fields->hitboxDrawNode, 10000);
            }
        }

        updateFloatingButton();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        updateFloatingButton();
        auto node = m_fields->hitboxDrawNode;
        if (!node) return;
        node->clear();

        auto mod = Mod::get();
        if (!mod) return;

        if (mod->getSettingValue<bool>("show-hitboxes")) {
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

        bool showTrail = mod->getSettingValue<bool>("show-trajectory");
        bool showProjection = mod->getSettingValue<bool>("show-projection");
        if ((showTrail || showProjection) && m_player1) {
            auto& points = m_fields->trajectoryPoints;
            auto& times = m_fields->trajectoryTimes;
            auto current = m_player1->getPosition();
            m_fields->trajectoryClock += std::max(0.f, dt);

            if (!points.empty() && ccpDistance(points.back(), current) > 260.f) {
                points.clear();
                times.clear();
            }
            if (points.empty() || ccpDistance(points.back(), current) >= 1.5f) {
                points.push_back(current);
                times.push_back(m_fields->trajectoryClock);
            }
            while (points.size() > 12) {
                points.erase(points.begin());
                times.erase(times.begin());
            }

            if (showTrail) {
                for (size_t i = 1; i < points.size(); ++i) {
                    node->drawSegment(points[i - 1], points[i], 1.5f,
                        ccc4f(0.25f, 0.9f, 1.f, 0.8f));
                }
            }

            // Estimate current velocity and acceleration from recent positions.
            // This is a short kinematic projection, not a complete GD physics engine.
            if (showProjection && points.size() >= 3) {
                size_t n = points.size();
                float dt1 = times[n - 2] - times[n - 3];
                float dt2 = times[n - 1] - times[n - 2];
                if (dt1 > 0.001f && dt2 > 0.001f && dt1 < 0.2f && dt2 < 0.2f) {
                    auto p0 = points[n - 3];
                    auto p1 = points[n - 2];
                    auto p2 = points[n - 1];
                    auto v1 = ccp((p1.x - p0.x) / dt1, (p1.y - p0.y) / dt1);
                    auto v2 = ccp((p2.x - p1.x) / dt2, (p2.y - p1.y) / dt2);
                    auto acceleration = ccp(
                        2.f * (v2.x - v1.x) / (dt1 + dt2),
                        2.f * (v2.y - v1.y) / (dt1 + dt2)
                    );
                    float accelLength = ccpLength(acceleration);
                    if (accelLength > 3000.f) {
                        float scale = 3000.f / accelLength;
                        acceleration = ccpMult(acceleration, scale);
                    }

                    CCPoint previous = current;
                    for (int step = 1; step <= 12; ++step) {
                        float t = step * 0.05f;
                        auto projected = ccp(
                            current.x + v2.x * t + 0.5f * acceleration.x * t * t,
                            current.y + v2.y * t + 0.5f * acceleration.y * t * t
                        );
                        node->drawSegment(previous, projected, 2.f,
                            ccc4f(1.f, 0.78f, 0.25f, 0.9f));
                        previous = projected;
                    }
                }
            }
        } else {
            m_fields->trajectoryPoints.clear();
            m_fields->trajectoryTimes.clear();
            m_fields->trajectoryClock = 0.f;
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
            bool noclipRequested = mod->getSettingValue<bool>("practice-tools")
                && mod->getSettingValue<bool>("noclip");
            bool speedhackRequested = mod->getSettingValue<bool>("speedhack-enabled");

            // With Safe Mode enabled, requesting a gameplay modifier in a
            // Normal Mode attempt first converts that attempt to Practice.
            if (mod->getSettingValue<bool>("safe-mode")
                && !this->m_isPracticeMode
                && (noclipRequested || speedhackRequested || attemptUsedCheat)) {
                this->m_isPracticeMode = true;
            }

            if (noclipRequested || speedhackRequested || g_macroPlayingBack)
                attemptUsedCheat = true;

            // Modifiers have no effect in Normal Mode. Safe Mode above can
            // move the attempt to Practice before any modified frame runs.
            bool noclipActive = this->m_isPracticeMode && noclipRequested;
            bool speedhackActive = this->m_isPracticeMode && speedhackRequested;
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

    listenForKeybindSettingPresses("keybind-trajectory", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (mod) mod->setSettingValue<bool>("show-trajectory", !mod->getSettingValue<bool>("show-trajectory"));
    });

    listenForKeybindSettingPresses("keybind-projection", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        auto mod = Mod::get();
        if (mod) mod->setSettingValue<bool>("show-projection", !mod->getSettingValue<bool>("show-projection"));
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