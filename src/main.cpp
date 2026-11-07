#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>
#include <regex>
#include <random>
#include <sstream>
#include <vector>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>

using namespace geode::prelude;

namespace {
    double g_originalFrameInterval = 0.0;

    struct UniAIUsage {
        std::string day;
        int requests = 0;
        int buttonTaps = 0;
        bool bonusUnlocked = false;
    };

    std::string uniAIToday() {
        std::time_t now = std::time(nullptr);
        // The server resets quotas at 00:00 UTC; keep the local display counter in sync.
        std::tm* utc = std::gmtime(&now);
        if (!utc) return "unknown-day";
        char buffer[16] = {};
        std::strftime(buffer, sizeof(buffer), "%Y%m%d", utc);
        return buffer;
    }

    std::string uniAIUsagePath() {
        auto mod = Mod::get();
        return mod ? (mod->getSaveDir() / "universe-ai-usage.txt").string() : std::string{};
    }

    UniAIUsage loadUniAIUsage() {
        UniAIUsage usage;
        auto path = uniAIUsagePath();
        if (path.empty()) return usage;
        std::ifstream file(path);
        if (file.is_open()) {
            int bonus = 0;
            file >> usage.day >> usage.requests >> usage.buttonTaps >> bonus;
            usage.bonusUnlocked = bonus != 0;
        }
        auto today = uniAIToday();
        if (usage.day != today) {
            usage.day = today;
            usage.requests = 0;
        }
        usage.requests = std::clamp(usage.requests, 0, 1000);
        usage.buttonTaps = std::clamp(usage.buttonTaps, 0, 7);
        return usage;
    }

    void saveUniAIUsage(UniAIUsage const& usage) {
        auto path = uniAIUsagePath();
        if (path.empty()) return;
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (file.is_open())
            file << usage.day << ' ' << usage.requests << ' '
                 << usage.buttonTaps << ' ' << (usage.bonusUnlocked ? 1 : 0) << '\n';
    }

    void registerUniButtonTap() {
        auto usage = loadUniAIUsage();
        if (usage.bonusUnlocked) return;
        usage.buttonTaps = std::min(7, usage.buttonTaps + 1);
        if (usage.buttonTaps >= 7) {
            usage.bonusUnlocked = true;
            log::info("Uni-1 bonus daily allowance unlocked: 1,000 requests.");
        }
        saveUniAIUsage(usage);
    }

    std::string uniAIQuotaLabel() {
        auto usage = loadUniAIUsage();
        int limit = usage.bonusUnlocked ? 1000 : 100;
        return fmt::format("Uni-1 daily requests: {}/{}{}", usage.requests, limit,
            usage.bonusUnlocked ? "  /  BONUS UNLOCKED" : "");
    }

    std::string uniAIInstallID() {
        auto mod = Mod::get();
        if (!mod) return {};
        auto path = (mod->getSaveDir() / "uni1-install-id.txt").string();
        {
            std::ifstream file(path);
            std::string existing;
            if (file >> existing && existing.size() == 32)
                return existing;
        }
        std::random_device random;
        static constexpr char hex[] = "0123456789abcdef";
        std::string id;
        id.reserve(32);
        for (int i = 0; i < 32; ++i)
            id.push_back(hex[random() & 0x0f]);
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (file.is_open()) file << id;
        return id;
    }

    std::string uni1ServerURL() {
        auto mod = Mod::get();
        if (!mod) return {};
        auto url = mod->getSettingValue<std::string>("ai-base-url");
        while (!url.empty() && url.back() == '/') url.pop_back();
        return url;
    }

    // Remote Uni-1 endpoints must use HTTPS. Plain HTTP is permitted only
    // for loopback development servers on the same desktop.
    bool isAllowedUni1ServerURL(std::string const& url) {
        auto schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos) return false;
        auto scheme = url.substr(0, schemeEnd);
        if (scheme != "https" && scheme != "http") return false;

        auto authorityStart = schemeEnd + 3;
        auto authorityEnd = url.find_first_of("/?#", authorityStart);
        auto authority = url.substr(
            authorityStart,
            authorityEnd == std::string::npos ? std::string::npos : authorityEnd - authorityStart
        );
        if (authority.empty() || authority.find('@') != std::string::npos) return false;
        if (scheme == "https") return true;

        std::string host;
        if (authority.front() == '[') {
            auto closing = authority.find(']');
            if (closing == std::string::npos) return false;
            host = authority.substr(0, closing + 1);
        } else {
            auto port = authority.find(':');
            host = authority.substr(0, port);
        }
        return host == "localhost" || host == "127.0.0.1" || host == "[::1]";
    }

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
        g_macroRecordingLayer = nullptr;
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
        file.flush();
        if (!file.good()) {
            log::error("Universe encountered an error while writing the practice replay.");
            return;
        }
        log::info("Universe saved {} practice replay events.", g_macroEvents.size());
    }

    bool startMacroRecording() {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_isPracticeMode || !layer->m_player1 || g_macroPlayingBack)
            return false;
        g_macroEvents.clear();
        g_macroTime = 0.f;
        g_macroRecordingLayer = layer;
        g_macroRecording = true;
        return true;
    }

    bool startMacroPlayback() {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_isPracticeMode || !layer->m_player1 || g_macroRecording)
            return false;
        auto path = macroFilePath();
        if (path.empty()) return false;
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::vector<MacroEvent> loaded;
        std::string line;
        float previousTime = -1.f;
        while (std::getline(file, line)) {
            if (line.size() > 128 || loaded.size() >= 100000)
                return false;
            std::stringstream row(line);
            MacroEvent event;
            char comma1 = 0;
            char comma2 = 0;
            int down = 0;
            if (row >> event.time >> comma1 >> event.button >> comma2 >> down
                && comma1 == ',' && comma2 == ','
                && std::isfinite(event.time) && event.time >= 0.f
                && event.button >= 0 && event.button <= 32
                && (down == 0 || down == 1)
                && event.time >= previousTime && event.time <= 3600.f) {
                row >> std::ws;
                if (!row.eof()) return false;
                event.down = down != 0;
                previousTime = event.time;
                loaded.push_back(event);
            } else {
                return false;
            }
        }
        if (loaded.empty()) return false;
        g_macroEvents = std::move(loaded);
        g_macroPlaybackIndex = 0;
        g_macroPlaybackTime = 0.f;
        g_macroHeldButtons.clear();
        g_macroPlaybackLayer = layer;
        g_macroPlayingBack = true;
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
        if (!g_macroPlayingBack || !layer || layer != g_macroPlaybackLayer) {
            if (g_macroPlayingBack) {
                // The previous PlayLayer has gone away; never dereference it.
                g_macroHeldButtons.clear();
                g_macroPlayingBack = false;
                g_macroPlaybackIndex = 0;
                g_macroPlaybackLayer = nullptr;
            }
            return;
        }
        if (!layer->m_player1) {
            stopMacroPlayback(layer);
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
        Player, Practice, Level, Visuals, Recorder, Keybinds, Settings, AI
    };

    void playUniverseClick() {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("click-sound")) return;
        auto audioPath = mod->getResourcesDir() / "click.mp3";
        auto audio = FMODAudioEngine::sharedEngine();
        if (audio) audio->playEffect(audioPath.string());
    }

    bool isUniverseNoclipActive() {
        auto mod = Mod::get();
        auto layer = PlayLayer::get();
        return mod && layer && mod->getSettingValue<bool>("noclip");
    }

    void drawRoundedPanel(CCDrawNode* node, CCSize size, ccColor4B color) {
        if (!node || size.width <= 0.f || size.height <= 0.f) return;
        node->clear();
        float r = std::min(20.f, std::min(size.width, size.height) * 0.08f);
        CCPoint points[] = {
            {r, 0.f}, {size.width - r, 0.f},
            {size.width - r * 0.29f, r * 0.29f}, {size.width, r},
            {size.width, size.height - r},
            {size.width - r * 0.29f, size.height - r * 0.29f},
            {size.width - r, size.height}, {r, size.height},
            {r * 0.29f, size.height - r * 0.29f}, {0.f, size.height - r},
            {0.f, r}, {r * 0.29f, r * 0.29f}
        };
        auto fill = ccc4f(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
        node->drawPolygon(points, sizeof(points) / sizeof(points[0]), fill, 0.f, fill);
    }

    void animateClosePanel(CCLayer* panel) {
        if (!panel) return;
        panel->stopAllActions();
        panel->runAction(CCSequence::create(
            CCSpawn::create(
                CCFadeTo::create(0.16f, 0),
                CCScaleTo::create(0.16f, 0.94f),
                nullptr
            ),
            CCCallFunc::create(panel, callfunc_selector(CCNode::removeFromParent)),
            nullptr
        ));
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
            case Page::AI: return "AI";
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

class DraftPlacementRunner : public CCLayer {
    std::vector<std::string> m_chunks;
    LevelEditorLayer* m_editor = nullptr;
    CCLabelBMFont* m_progress = nullptr;
    size_t m_nextChunk = 0;
    size_t m_totalObjects = 0;
    size_t m_processedObjects = 0;
    size_t m_added = 0;
    size_t m_changed = 0;
    size_t m_deleted = 0;

    void finish(bool cancelled, bool editorClosed = false) {
        unscheduleAllSelectors();
        auto currentEditor = LevelEditorLayer::get();
        auto ui = currentEditor == m_editor && currentEditor ? currentEditor->m_editorUI : nullptr;
        if (ui) {
            ui->updateButtons();
            ui->updateObjectInfoLabel();
        }

        std::string message;
        if (editorClosed) {
            message = fmt::format(
                "Placement stopped because the editor closed. {} of {} new objects were placed; existing edits already applied remain. Reopen the editor and use Undo if needed.",
                m_processedObjects, m_totalObjects
            );
        } else if (cancelled) {
            message = fmt::format(
                "Placement cancelled after {} of {} new objects. Objects already placed remain in the editor; use Undo to revert them.",
                m_processedObjects, m_totalObjects
            );
        } else {
            message = fmt::format(
                "Universe applied {} new objects, {} existing-object edits, and {} object deletions. Use the editor Undo button to revert.",
                m_added, m_changed, m_deleted
            );
        }
        FLAlertLayer::create("Universe AI", message.c_str(), "OK")->show();
        removeFromParentAndCleanup(true);
    }

    void step(float) {
        auto editor = LevelEditorLayer::get();
        if (!editor || editor != m_editor || !editor->m_editorUI) {
            finish(false, true);
            return;
        }
        if (m_nextChunk >= m_chunks.size()) {
            finish(false);
            return;
        }

        auto const& chunk = m_chunks[m_nextChunk++];
        editor->m_editorUI->pasteObjects(chunk, false, false);
        m_processedObjects += static_cast<size_t>(std::count(chunk.begin(), chunk.end(), ';'));
        if (m_progress) {
            auto status = fmt::format("PLACING OBJECTS: {} / {}", m_processedObjects, m_totalObjects);
            m_progress->setString(status.c_str());
        }
        if (m_nextChunk >= m_chunks.size())
            finish(false);
    }

    void cancel(CCObject*) {
        finish(true);
    }

public:
    static DraftPlacementRunner* create(
        std::vector<std::string> chunks,
        LevelEditorLayer* editor,
        size_t added,
        size_t changed,
        size_t deleted
    ) {
        auto ret = new DraftPlacementRunner();
        if (ret && ret->init(std::move(chunks), editor, added, changed, deleted)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(
        std::vector<std::string> chunks,
        LevelEditorLayer* editor,
        size_t added,
        size_t changed,
        size_t deleted
    ) {
        if (!CCLayer::init() || !editor || chunks.empty())
            return false;
        m_chunks = std::move(chunks);
        m_editor = editor;
        m_added = added;
        m_changed = changed;
        m_deleted = deleted;
        for (auto const& chunk : m_chunks)
            m_totalObjects += static_cast<size_t>(std::count(chunk.begin(), chunk.end(), ';'));

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto backdrop = CCLayerColor::create(ccc4(12, 15, 28, 235));
        if (backdrop) {
            backdrop->setContentSize(win);
            backdrop->setPosition({0.f, 0.f});
            addChild(backdrop);
        }

        m_progress = CCLabelBMFont::create("PLACING OBJECTS: 0", "bigFont.fnt");
        if (m_progress) {
            m_progress->setScale(0.48f);
            m_progress->setColor({205, 190, 255});
            m_progress->setPosition({win.width / 2.f, win.height / 2.f + 18.f});
            addChild(m_progress, 2);
        }

        auto cancelLabel = CCLabelBMFont::create("CANCEL PLACEMENT", "goldFont.fnt");
        auto cancelItem = cancelLabel
            ? CCMenuItemLabel::create(cancelLabel, this, menu_selector(DraftPlacementRunner::cancel))
            : nullptr;
        auto menu = cancelItem ? CCMenu::create(cancelItem, nullptr) : nullptr;
        if (menu) {
            cancelItem->setScale(0.8f);
            cancelItem->setPosition({0.f, -35.f});
            menu->setPosition({win.width / 2.f, win.height / 2.f});
            addChild(menu, 3);
        }

        schedule(schedule_selector(DraftPlacementRunner::step), 0.f);
        return true;
    }
};

class UniversePanel : public CCLayer, public TextInputDelegate {
protected:
    Page m_page = Page::Player;
    CCTextInputNode* m_aiPrompt = nullptr;
    CCTextInputNode* m_aiModelInput = nullptr;
    CCTextInputNode* m_aiBaseUrlInput = nullptr;
    CCLabelBMFont* m_aiStatus = nullptr;
    async::TaskHolder<web::WebResponse> m_aiTask;
    CCMenu* m_menu = nullptr;
    CCMenu* m_actionMenu = nullptr;
    CCDrawNode* m_panelBackground = nullptr;
    CCSize m_panelSize = {0.f, 0.f};
    CCLayerColor* m_dimBackground = nullptr;
    CCLabelBMFont* m_pageTitle = nullptr;
    std::vector<CCLabelBMFont*> m_tabLabels;
    CCLayer* m_content = nullptr;
    bool m_isDragging = false;
    CCPoint m_dragTouchStart = {0.f, 0.f};
    CCPoint m_dragPanelStart = {0.f, 0.f};

    void addText(const char* text, CCPoint pos, float scale = 0.42f) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});
        label->setScale(scale);
        label->setColor({205, 216, 245});
        label->setPosition(pos);
        m_content->addChild(label);
    }

    void addAction(const char* text, int tag, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "goldFont.fnt");
        label->setScale(0.38f);
        label->setColor({153, 207, 255});
        auto item = CCMenuItemLabel::create(label, this, menu_selector(UniversePanel::onAction));
        item->setTag(tag);
        item->setPosition(pos);
        item->setScale(0.96f);
        if (m_actionMenu) m_actionMenu->addChild(item);
    }

    void switchStartPos(int direction) {
        auto layer = PlayLayer::get();
        if (!layer || !layer->m_startPosObjects || layer->m_startPosObjects->count() == 0)
            return;

        auto objects = layer->m_startPosObjects;
        int index = objects->indexOfObject(layer->m_startPosObject);
        bool hasCurrent = layer->m_startPosObject && index >= 0;
        CCObject* next = nullptr;

        auto count = objects->count();
        if (!hasCurrent) {
            next = direction < 0 ? objects->lastObject() : objects->firstObject();
        } else if (direction < 0) {
            auto previous = (static_cast<unsigned int>(index) + count - 1) % count;
            next = objects->objectAtIndex(previous);
        } else {
            auto following = (static_cast<unsigned int>(index) + 1) % count;
            next = objects->objectAtIndex(following);
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
        if (m_panelBackground) drawRoundedPanel(m_panelBackground, m_panelSize, color);
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
        if (m_actionMenu) m_actionMenu->removeAllChildrenWithCleanup(true);
        m_aiPrompt = nullptr;
        m_aiModelInput = nullptr;
        m_aiBaseUrlInput = nullptr;
        m_aiStatus = nullptr;
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
        m_pageTitle->setColor({190, 145, 255});
        m_pageTitle->runAction(CCEaseBackOut::create(CCScaleTo::create(0.18f, 0.55f)));
        for (size_t i = 0; i < m_tabLabels.size(); ++i) {
            if (!m_tabLabels[i]) continue;
            bool selected = i == static_cast<size_t>(page);
            m_tabLabels[i]->setColor(selected ? ccColor3B{190, 145, 255} : ccColor3B{185, 198, 225});
            m_tabLabels[i]->setScale(selected ? 0.37f : 0.33f);
        }

        switch (page) {
            case Page::Player: {
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
                addText("ICON LAB  /  LOCAL COSMETIC PREVIEW", {-245.f, -90.f}, 0.32f);
                auto mod = Mod::get();
                if (!mod) break;
                if (auto gm = GameManager::get()) {
                    static const char* iconNames[] = {
                        "Cube", "Ship", "Ball", "UFO", "Wave", "Robot", "Spider", "Swing", "Jetpack"
                    };
                    auto typeID = static_cast<int>(std::clamp<int64_t>(
                        mod->getSettingValue<int64_t>("preview-icon-type"), 0, 8
                    ));
                    auto iconType = static_cast<IconType>(typeID);
                    auto previewID = static_cast<int>(std::clamp<int64_t>(
                        mod->getSettingValue<int64_t>("preview-icon-id"), 1, 500
                    ));
                    auto typeText = fmt::format("Type: {}", iconNames[typeID]);
                    addText(typeText.c_str(), {-245.f, -125.f}, 0.32f);
                    addAction("<", 147, {125.f, -125.f});
                    addAction(">", 148, {205.f, -125.f});
                    auto icon = SimplePlayer::create(0);
                    if (icon) {
                        icon->updatePlayerFrame(previewID, iconType);
                        icon->setColor(gm->colorForIdx(gm->getPlayerColor()));
                        icon->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
                        icon->setPosition({0.f, -180.f});
                        icon->setScale(1.25f);
                        m_content->addChild(icon);
                    }
                    auto owned = gm->isIconUnlocked(previewID, iconType);
                    auto iconText = fmt::format("ICON #{}  /  {}", previewID, owned ? "OWNED" : "LOCAL PREVIEW");
                    addText(iconText.c_str(), {-245.f, -235.f}, 0.30f);
                    addAction("-", 124, {125.f, -235.f});
                    addAction("+", 125, {205.f, -235.f});
                }
                bool overrideEnabled = mod->getSettingValue<bool>("local-icon-override");
                addAction(overrideEnabled ? "DISABLE LOCAL OVERRIDE" : "EQUIP LOCAL ICON", 127, {0.f, -280.f});
                addText("LOCAL ONLY: account unlocks and saved icon set are unchanged.", {-245.f, -310.f}, 0.27f);
                addText("Primary, secondary and glow colors: Geode > Universe settings.", {-245.f, -340.f}, 0.24f);
                break;
            }
            case Page::Practice: {
                auto mod = Mod::get();
                addText("Gameplay hacks", {-245.f, 105.f});
                addText("Noclip and Speedhack work in Normal Mode when Safe Mode is OFF.", {-245.f, 65.f}, 0.30f);
                addToggle("Noclip", "noclip", 103, 25.f);
                addToggle("Speedhack", "speedhack-enabled", 106, -25.f);
                auto speed = mod ? std::clamp(mod->getSettingValue<float>("speedhack-speed"), 0.25f, 3.f) : 1.f;
                auto speedText = fmt::format("Speed: {:.2f}x", speed);
                addText(speedText.c_str(), {-245.f, -110.f}, 0.36f);
                addAction("-", 107, {125.f, -110.f});
                addAction("+", 108, {205.f, -110.f});
                addAction("RESET TO 1.00x", 134, {0.f, -205.f});
                addText("Speedhack changes local gameplay timing.", {-245.f, -100.f}, 0.28f);
                addText("Safe Mode ON switches modified attempts to Practice Mode.", {-245.f, -135.f}, 0.28f);
                addText("Safe Mode OFF leaves the selected hacks active in Normal Mode.", {-245.f, -170.f}, 0.27f);
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
                addText("Online level-design helper: scripts/level_maker.py", {-245.f, -165.f}, 0.29f);
                addText("Set a self-hosted Uni-1 server URL in Geode > Universe settings.", {-245.f, -200.f}, 0.27f);
                addText("Blueprint output is not a playable level save.", {-245.f, -235.f}, 0.27f);
                break;
            }
            case Page::Visuals: {
                addText("Visual helpers", {-245.f, 105.f});
                addToggle("Show object hitboxes", "show-hitboxes", 111, 65.f);
                addToggle("FPS Bypass", "fps-bypass-enabled", 112, 15.f);
                auto mod = Mod::get();
                auto target = mod ? std::clamp<int64_t>(mod->getSettingValue<int64_t>("fps-target"), 60, 360) : 240;
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
                addToggle("Show player bounds", "show-player-bounds", 151, -310.f);
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
            case Page::AI: {
                addText("UNI-1  /  AI LEVEL CREATOR", {-245.f, 105.f});
                addText("Server quota: 100/day; 1,000/day after 7 UNIV taps.", {-245.f, 82.f}, 0.29f);
                addText("Level idea", {-245.f, 72.f}, 0.31f);
                m_aiPrompt = CCTextInputNode::create(430.f, 42.f, "Paste or type your full level prompt here...", "bigFont.fnt");
                if (m_aiPrompt) {
                    m_aiPrompt->setDelegate(this);
                    m_aiPrompt->setPosition({55.f, 42.f});
                    m_content->addChild(m_aiPrompt, 4);
                }
                addText("No player API key required.", {-245.f, -5.f}, 0.34f);
                addText("Uni-1 server URL: Geode > Universe settings.", {-245.f, -38.f}, 0.29f);
                addText("The server is authoritative for quota and usage.", {-245.f, -70.f}, 0.27f);
                addText("The server must be online for generation.", {-245.f, -102.f}, 0.26f);
                addAction("SAVE PROMPT", 141, {-175.f, -150.f});
                addAction("LOAD SAVED", 143, {0.f, -150.f});
                addAction("CLEAR", 142, {175.f, -150.f});
                addAction("GENERATE ONLINE", 126, {-115.f, -200.f});
                addAction("CANCEL REQUEST", 152, {125.f, -200.f});
                m_aiStatus = CCLabelBMFont::create("STATUS: IDLE", "bigFont.fnt");
                if (m_aiStatus) {
                    m_aiStatus->setScale(0.28f);
                    m_aiStatus->setColor({190, 145, 255});
                    m_aiStatus->setPosition({0.f, -175.f});
                    m_content->addChild(m_aiStatus, 4);
                }
                addText("Describe the entire level: layout, gameplay flow, and difficulty.", {-245.f, -238.f}, 0.26f);
                addText("Online AI creates a complete object layout.", {-245.f, -268.f}, 0.25f);
                addAction("CREATE FULL LEVEL IN EDITOR", 149, {0.f, -285.f});
                addText("Open an empty level first; creation adds the generated layout.", {-245.f, -315.f}, 0.24f);
                addText("Cube-mode blocks, spikes, saws, orbs and pads are supported.", {-245.f, -345.f}, 0.23f);
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
                addAction("RUN IN-GAME SELF TEST", 150, {0.f, -325.f});
                break;
        }
    }

    void textChanged(CCTextInputNode* input) override {
        auto mod = Mod::get();
        if (!mod || !input) return;
        if (input == m_aiModelInput)
            mod->setSettingValue<std::string>("ai-model", input->getString());
        else if (input == m_aiBaseUrlInput)
            mod->setSettingValue<std::string>("ai-base-url", input->getString());
    }

    // Adapted from GD-AI-bridge's compact ASCII map/object-priority implementation.
    // Upstream: https://github.com/RibomX/GD-AI-bridge (MIT; see uni1/vendor/gd-ai-bridge/LICENSE).
    char uni1MapChar(GameObject* obj) {
        if (!obj || obj->m_isTrigger || obj->m_isDecoration || obj->m_isNoTouch) return 0;
        switch (obj->m_objectType) {
            case GameObjectType::Solid: return '#';
            case GameObjectType::Slope: return '/';
            case GameObjectType::Breakable: return '%';
            case GameObjectType::Hazard:
            case GameObjectType::AnimatedHazard: return '^';
            case GameObjectType::YellowJumpPad:
            case GameObjectType::PinkJumpPad:
            case GameObjectType::GravityPad:
            case GameObjectType::RedJumpPad:
            case GameObjectType::SpiderPad: return '_';
            case GameObjectType::YellowJumpRing:
            case GameObjectType::PinkJumpRing:
            case GameObjectType::GravityRing:
            case GameObjectType::GreenRing:
            case GameObjectType::DropRing:
            case GameObjectType::RedJumpRing:
            case GameObjectType::CustomRing:
            case GameObjectType::DashRing:
            case GameObjectType::GravityDashRing:
            case GameObjectType::SpiderOrb:
            case GameObjectType::TeleportOrb: return 'o';
            case GameObjectType::Modifier: return '>';
            case GameObjectType::SecretCoin:
            case GameObjectType::UserCoin:
            case GameObjectType::Collectible: return '$';
            case GameObjectType::Decoration: return 0;
            default: return 'P';
        }
    }

    int uni1MapPriority(char symbol) {
        switch (symbol) {
            case '^': return 6;
            case 'P': return 5;
            case 'o': return 4;
            case '_': return 3;
            case '>': return 2;
            case '$': return 1;
            default: return 0;
        }
    }

    std::string buildEditorContext() {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_editorUI)
            return "Editor state: no level is currently open in the editor.";

        auto objects = editor->getAllObjects();
        auto ui = editor->m_editorUI;
        auto selected = ui->m_selectedObjects;
        size_t selectedCount = selected ? selected->count() : 0;
        if (selectedCount == 0 && ui->m_selectedObject) selectedCount = 1;

        std::string context = fmt::format(
            "Editor state: {} objects in the open level; {} currently selected. "
            "This context is read-only; generation must not claim it changed existing objects. ",
            objects ? objects->count() : 0, selectedCount
        );
        context += "Compact object sample (uid,id,x,y; coordinates are editor coordinates; maximum 60): ";
        if (objects) {
            for (unsigned int i = 0; i < objects->count() && i < 60; ++i) {
                auto object = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (!object) continue;
                auto p = object->getPosition();
                context += fmt::format("{},{},{:.0f},{:.0f};",
                    object->m_uniqueID, object->m_objectID, p.x, p.y);
            }
        }

        context += " Selected object sample (maximum 20): ";
        size_t shown = 0;
        if (selected && selected->count() > 0) {
            for (unsigned int i = 0; i < selected->count() && shown < 20; ++i) {
                auto object = static_cast<GameObject*>(selected->objectAtIndex(i));
                if (!object) continue;
                auto p = object->getPosition();
                context += fmt::format("[uid={},id={},x={:.0f},y={:.0f},rot={:.0f}] ",
                    object->m_uniqueID, object->m_objectID, p.x, p.y, object->getRotation());
                ++shown;
            }
        } else if (ui->m_selectedObject) {
            auto object = ui->m_selectedObject;
            auto p = object->getPosition();
            context += fmt::format("[uid={},id={},x={:.0f},y={:.0f},rot={:.0f}]",
                object->m_uniqueID, object->m_objectID, p.x, p.y, object->getRotation());
        } else {
            context += "none.";
        }

        // GD-AI-bridge map convention: y=0 is the ground/data baseline
        // (editor-space y=90). One character represents a 30x30 editor cell.
        constexpr int kCell = 30;
        constexpr int kRows = 10;
        constexpr int kMaxColumns = 40;
        int columns = 1;
        if (objects) {
            float maxX = 0.f;
            for (unsigned int i = 0; i < objects->count(); ++i) {
                auto object = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (object) maxX = std::max(maxX, object->getPositionX());
            }
            columns = std::clamp(static_cast<int>(std::ceil((maxX + kCell) / kCell)), 1, kMaxColumns);
        }

        std::vector<std::string> grid(kRows, std::string(columns, '.'));
        if (objects) {
            for (unsigned int i = 0; i < objects->count(); ++i) {
                auto object = static_cast<GameObject*>(objects->objectAtIndex(i));
                char symbol = uni1MapChar(object);
                if (!symbol) continue;
                auto p = object->getPosition();
                int column = static_cast<int>(std::floor(p.x / kCell));
                int row = static_cast<int>(std::floor((p.y - 90.f + 90.f) / kCell));
                if (column < 0 || column >= columns || row < 0 || row >= kRows) continue;
                auto& cell = grid[row][column];
                if (cell == '.' || uni1MapPriority(symbol) > uni1MapPriority(cell))
                    cell = symbol;
            }
        }

        context += fmt::format(
            " ASCII gameplay map ({} columns x {} rows, 30px cells; # solid, / slope, % breakable, "
            "^ hazard, _ pad, o orb, P portal, > modifier, $ coin, . empty; top row first):\n",
            columns, kRows
        );
        for (int row = kRows - 1; row >= 0; --row)
            context += fmt::format("{:>3}|{}\n", row, grid[row]);
        context += "Map is a coarse overview only; object sample is authoritative. ";
        return context;
    }

    void generateAIDraft() {
        if (m_aiTask.isPending()) {
            FLAlertLayer::create("Uni-1", "A generation request is already running. Cancel it before starting another.", "OK")->show();
            return;
        }
        if (!m_aiPrompt) {
            FLAlertLayer::create("Universe AI", "The prompt field could not be created. Reopen the AI tab and try again.", "OK")->show();
            return;
        }

        auto prompt = std::string(m_aiPrompt->getString());
        if (prompt.empty()) {
            FLAlertLayer::create("Universe AI", "Enter a level idea first.", "OK")->show();
            return;
        }
        if (prompt.size() > 4000) {
            FLAlertLayer::create("Universe AI", "Please keep the prompt under 4000 characters.", "OK")->show();
            return;
        }

        auto baseUrl = uni1ServerURL();
        auto installID = uniAIInstallID();
        if (!isAllowedUni1ServerURL(baseUrl) || installID.empty()) {
            FLAlertLayer::create(
                "Uni-1",
                "Use HTTPS for remote servers, or http://127.0.0.1:8000 for a local desktop server. No provider API key is needed.",
                "OK"
            )->show();
            return;
        }

        matjson::Value payload = matjson::Value::object();
        payload["model"] = "uni-1";
        payload["max_tokens"] = 4096;
        matjson::Value messages = matjson::Value::array();
        matjson::Value userMessage = matjson::Value::object();
        userMessage["role"] = "user";
        userMessage["content"] =
            "Create a COMPLETE NEW GEOMETRY DASH LEVEL FROM SCRATCH. Return ONLY one valid JSON object "
            "matching the Universe Level Draft schema: title, prompt, difficulty, style, sections, notes. "
            "Create 6 to 8 connected sections with 96 to 144 ordered object placements. Events may use "
            "named primitive types block, spike, saw, orb, pad, OR type object with an exact numeric id "
            "from the verified editor catalogue supplied by the server. Generic object events may include "
            "a properties string containing comma-separated numeric GD save-string key/value pairs. "
            "Use actual decoration objects, portals, color/pulse/alpha/move triggers, effects, and other "
            "editor objects when appropriate to the requested concept. Do not invent IDs or trigger property "
            "keys; if a trigger configuration is unknown, omit its properties rather than guessing. "
            "Use sensible spacing, traversable routes, varied fair obstacle patterns, readable decoration, "
            "and clear transitions. Keep coordinates finite and within +/-10000000. Default to cube mode, "
            "but use verified mode portals when the concept requests a mode change. Never claim the level "
            "was playtested. The user will apply this to an empty editor; do not claim it clears existing "
            "contents. The following editor context is read-only background information; do not output "
            "edit/delete operations or claim existing objects were changed: " + buildEditorContext() + "\nConcept: " + prompt;
        messages.push(userMessage);
        payload["messages"] = messages;

        auto request = web::WebRequest()
            .header("X-Uni1-Install-ID", installID)
            .header("content-type", "application/json")
            .timeout(std::chrono::seconds(180))
            .bodyJSON(payload);

        m_aiTask.spawn(
            request.post(baseUrl + "/v1/chat/completions"),
            [this](web::WebResponse response) {
                if (!response.ok()) {
                    if (m_aiStatus) m_aiStatus->setString("STATUS: SERVER ERROR");
                    auto details = response.string().unwrapOr("No response details were provided.");
                    if (details.size() > 500) details.resize(500);
                    auto message = fmt::format("Uni-1 server error (HTTP {}). {}", response.code(), details);
                    FLAlertLayer::create("Uni-1", message.c_str(), "OK")->show();
                    return;
                }

                auto parsed = response.json();
                if (!parsed) {
                    if (m_aiStatus) m_aiStatus->setString("STATUS: INVALID RESPONSE");
                    FLAlertLayer::create("Uni-1", "The Uni-1 server returned invalid JSON.", "OK")->show();
                    return;
                }

                std::string draft;
                auto choices = parsed.unwrap()["choices"].asArray();
                if (choices && !choices.unwrap().empty()) {
                    auto content = choices.unwrap().front()["message"]["content"].asString();
                    if (content) draft = content.unwrap();
                }
                if (draft.empty()) {
                    if (m_aiStatus) m_aiStatus->setString("STATUS: EMPTY DRAFT");
                    FLAlertLayer::create("Uni-1", "The model response did not contain a level draft.", "OK")->show();
                    return;
                }

                auto mod = Mod::get();
                if (!mod) return;
                auto path = mod->getSaveDir() / "universe-ai-draft.json";
                std::ofstream file(path.string(), std::ios::out | std::ios::trunc);
                if (!file.is_open()) {
                    FLAlertLayer::create("Universe AI", "The draft was generated, but Universe could not save the file.", "OK")->show();
                    return;
                }
                file << draft;
                file.close();
                if (m_aiStatus) m_aiStatus->setString("STATUS: DRAFT SAVED");

                auto message = fmt::format(
                    "AI draft generated. Open the level editor and click CREATE FULL LEVEL IN EDITOR.\n\nDraft: {}",
                    path.string()
                );
                FLAlertLayer::create("Universe AI", message.c_str(), "OK")->show();
            }
        );
        if (m_aiStatus) m_aiStatus->setString("STATUS: GENERATING");
    }

    void applySavedDraftToEditor() {
        auto mod = Mod::get();
        auto editor = LevelEditorLayer::get();
        if (!mod || !editor || !editor->m_editorUI) {
            FLAlertLayer::create("Universe AI", "Open a Geometry Dash level in the editor first, then apply the generated draft.", "OK")->show();
            return;
        }

        auto path = mod->getSaveDir() / "universe-ai-draft.json";
        std::ifstream file(path.string(), std::ios::in);
        if (!file.is_open()) {
            FLAlertLayer::create("Universe AI", "No generated draft was found. Generate an online AI draft first.", "OK")->show();
            return;
        }
        std::string draft((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (draft.empty() || draft.size() > 2'000'000) {
            FLAlertLayer::create("Universe AI", "The saved draft is empty or exceeds the 2 MB safety limit.", "OK")->show();
            return;
        }

        // Keep the editor-side ceiling aligned with Uni-1's server validator.
        // Large single-frame paste operations can freeze or exhaust mobile memory.
        static constexpr size_t kMaxGeneratedObjects = 1000;
        static const std::regex eventPattern(R"(\{[^{}]*\})");
        static const std::regex xPattern(R"("x"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?))");
        static const std::regex yPattern(R"("y"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?))");
        static const std::regex typePattern(R"rx("type"\s*:\s*"([A-Za-z]+)")rx");
        static const std::regex objectIDPattern(R"("id"\s*:\s*([0-9]+))");
        static const std::regex propertiesPattern(R"rx("properties"\s*:\s*"([^"]*)")rx");
        static const std::regex actionPattern(R"rx("action"\s*:\s*"([A-Za-z_]+)")rx");
        static const std::regex dxPattern(R"("dx"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?))");
        static const std::regex dyPattern(R"("dy"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?))");
        static const std::regex degreesPattern(R"("degrees"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?))");
        static const std::regex targetPattern(R"("target"\s*:\s*([0-9]+))");
        // Preflight the entire draft before touching the editor. Count placement
        // events (the default action is "add") so an oversized draft is rejected
        // atomically instead of being partially applied.
        size_t requestedPlacements = 0;
        for (std::sregex_iterator it(draft.begin(), draft.end(), eventPattern), end; it != end; ++it) {
            auto event = it->str();
            std::smatch actionMatch, typeMatch;
            std::string action = "add";
            if (std::regex_search(event, actionMatch, actionPattern))
                action = actionMatch[1].str();
            if (action == "add" && std::regex_search(event, typeMatch, typePattern))
                ++requestedPlacements;
            if (requestedPlacements > kMaxGeneratedObjects) {
                FLAlertLayer::create(
                    "Universe AI",
                    "This draft exceeds the hard limit of 1,000 generated objects. Nothing was placed.",
                    "OK"
                )->show();
                return;
            }
        }

        std::string serialized;
        std::vector<std::string> placementChunks;
        size_t chunkObjectCount = 0;
        size_t added = 0;
        size_t changed = 0;
        size_t deleted = 0;

        auto selectedObjects = CCArray::create();
        auto ui = editor->m_editorUI;
        if (ui->m_selectedObjects && ui->m_selectedObjects->count() > 0) {
            for (unsigned int i = 0; i < ui->m_selectedObjects->count() && selectedObjects->count() < 1000; ++i)
                selectedObjects->addObject(ui->m_selectedObjects->objectAtIndex(i));
        } else if (ui->m_selectedObject) {
            selectedObjects->addObject(ui->m_selectedObject);
        }

        auto replaceObjectsWithTransform = [&](CCArray* targets, float dx, float dy, float degrees) {
            if (!targets || targets->count() == 0) return false;
            std::string replacements;
            for (unsigned int i = 0; i < targets->count(); ++i) {
                auto object = static_cast<GameObject*>(targets->objectAtIndex(i));
                if (!object) continue;
                auto oldPosition = object->getPosition();
                auto oldRotation = object->getRotation();
                object->setPosition({oldPosition.x + dx, oldPosition.y + dy});
                object->setRotation(oldRotation + degrees);
                replacements += object->getSaveString(editor) + ";";
                object->setPosition(oldPosition);
                object->setRotation(oldRotation);
            }
            if (replacements.empty()) return false;
            for (unsigned int i = 0; i < targets->count(); ++i)
                ui->deleteObject(static_cast<GameObject*>(targets->objectAtIndex(i)), false);
            ui->pasteObjects(replacements, false, false);
            ui->updateButtons();
            ui->updateObjectInfoLabel();
            return true;
        };

        for (std::sregex_iterator it(draft.begin(), draft.end(), eventPattern), end; it != end; ++it) {
            if (added + changed + deleted >= 1000) break;
            auto event = it->str();
            std::smatch actionMatch, xMatch, yMatch, typeMatch, dxMatch, dyMatch, degreesMatch;
            std::string action = "add";
            if (std::regex_search(event, actionMatch, actionPattern))
                action = actionMatch[1].str();

            if (action == "delete_object" || action == "move_object" || action == "rotate_object") {
                std::smatch targetMatch;
                if (!std::regex_search(event, targetMatch, targetPattern)) continue;
                int targetID = 0;
                try { targetID = std::stoi(targetMatch[1].str()); } catch (...) { continue; }
                GameObject* target = nullptr;
                auto allObjects = editor->getAllObjects();
                if (allObjects) {
                    for (unsigned int i = 0; i < allObjects->count(); ++i) {
                        auto candidate = static_cast<GameObject*>(allObjects->objectAtIndex(i));
                        if (candidate && candidate->m_uniqueID == targetID) {
                            target = candidate;
                            break;
                        }
                    }
                }
                if (!target) continue;
                if (action == "delete_object") {
                    ui->deleteObject(target, false);
                    ++deleted;
                    ui->updateButtons();
                    ui->updateObjectInfoLabel();
                    continue;
                }
                auto targets = CCArray::create();
                targets->addObject(target);
                float dx = 0.f, dy = 0.f, degrees = 0.f;
                try {
                    if (action == "move_object") {
                        if (!std::regex_search(event, dxMatch, dxPattern)
                            || !std::regex_search(event, dyMatch, dyPattern)) continue;
                        dx = std::stof(dxMatch[1].str());
                        dy = std::stof(dyMatch[1].str());
                    } else {
                        if (!std::regex_search(event, degreesMatch, degreesPattern)) continue;
                        degrees = std::stof(degreesMatch[1].str());
                    }
                } catch (...) { continue; }
                if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(degrees)) continue;
                dx = std::clamp(dx, -100000.f, 100000.f);
                dy = std::clamp(dy, -100000.f, 100000.f);
                degrees = std::clamp(degrees, -3600.f, 3600.f);
                if (replaceObjectsWithTransform(targets, dx, dy, degrees)) ++changed;
                continue;
            }

            if (action == "delete_selected") {
                if (selectedObjects->count() == 0) continue;
                for (unsigned int i = 0; i < selectedObjects->count(); ++i) {
                    auto object = static_cast<GameObject*>(selectedObjects->objectAtIndex(i));
                    if (object) ui->deleteObject(object, false);
                }
                deleted += selectedObjects->count();
                selectedObjects->removeAllObjects();
                ui->updateButtons();
                ui->updateObjectInfoLabel();
                continue;
            }

            if (action == "move_selected" || action == "rotate_selected") {
                float dx = 0.f, dy = 0.f, degrees = 0.f;
                try {
                    if (action == "move_selected") {
                        if (std::regex_search(event, dxMatch, dxPattern)) dx = std::stof(dxMatch[1].str());
                        if (std::regex_search(event, dyMatch, dyPattern)) dy = std::stof(dyMatch[1].str());
                    } else if (std::regex_search(event, degreesMatch, degreesPattern)) {
                        degrees = std::stof(degreesMatch[1].str());
                    }
                } catch (...) { continue; }
                if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(degrees)) continue;
                dx = std::clamp(dx, -100000.f, 100000.f);
                dy = std::clamp(dy, -100000.f, 100000.f);
                degrees = std::clamp(degrees, -3600.f, 3600.f);
                auto selectedBeforeEdit = selectedObjects->count();
                if (replaceObjectsWithTransform(selectedObjects, dx, dy, degrees)) {
                    changed += selectedBeforeEdit;
                    selectedObjects->removeAllObjects();
                }
                continue;
            }

            if (action != "add" || !std::regex_search(event, xMatch, xPattern)
                || !std::regex_search(event, yMatch, yPattern)
                || !std::regex_search(event, typeMatch, typePattern))
                continue;
            float x = 0.f, y = 0.f;
            try {
                x = std::stof(xMatch[1].str());
                y = std::stof(yMatch[1].str());
            } catch (...) { continue; }
            if (!std::isfinite(x) || !std::isfinite(y)) continue;
            x = std::clamp(x, -10000000.f, 10000000.f);
            y = std::clamp(y, -10000000.f, 10000000.f);

            auto type = typeMatch[1].str();
            int objectID = 1;
            std::string extraProperties;
            if (type == "spike") objectID = 8;
            else if (type == "saw") objectID = 39;
            else if (type == "orb") objectID = 36;
            else if (type == "pad") objectID = 35;
            else if (type == "object") {
                std::smatch idMatch, propertiesMatch;
                if (!std::regex_search(event, idMatch, objectIDPattern)) continue;
                try { objectID = std::stoi(idMatch[1].str()); } catch (...) { continue; }
                if (objectID < 1 || objectID > 5000) continue;

                if (std::regex_search(event, propertiesMatch, propertiesPattern)) {
                    extraProperties = propertiesMatch[1].str();
                    if (extraProperties.size() > 2000) continue;
                    if (!extraProperties.empty()) {
                        std::stringstream propertyStream(extraProperties);
                        std::string key, value;
                        bool valid = true;
                        size_t pairCount = 0;
                        while (std::getline(propertyStream, key, ',')) {
                            if (!std::getline(propertyStream, value, ',')) { valid = false; break; }
                            if (key.empty() || !std::all_of(key.begin(), key.end(), [](unsigned char ch) { return std::isdigit(ch); })
                                || key == "1" || key == "2" || key == "3" || value.empty()
                                || value.find(';') != std::string::npos
                                || value.find('"') != std::string::npos
                                || value.find('\\n') != std::string::npos
                                || value.find('\\r') != std::string::npos) {
                                valid = false;
                                break;
                            }
                            if (++pairCount > 64) { valid = false; break; }
                        }
                        if (!valid) continue;
                    }
                }
            } else if (type != "block") continue;

            serialized += fmt::format("1,{},2,{:.0f},3,{:.0f}{};", objectID, x, y,
                extraProperties.empty() ? "" : "," + extraProperties);
            ++added;
            ++chunkObjectCount;
            if (chunkObjectCount >= 100) {
                placementChunks.push_back(std::move(serialized));
                serialized.clear();
                chunkObjectCount = 0;
            }
        }

        if (!serialized.empty())
            placementChunks.push_back(std::move(serialized));

        if (added + changed + deleted == 0) {
            FLAlertLayer::create("Universe AI", "No supported add/move/rotate/delete operations were found. Select the objects you want edited, then generate a new instruction.", "OK")->show();
            return;
        }

        if (!placementChunks.empty()) {
            auto scene = CCDirector::sharedDirector()->getRunningScene();
            auto runner = DraftPlacementRunner::create(
                std::move(placementChunks), editor, added, changed, deleted
            );
            if (!scene || !runner) {
                FLAlertLayer::create("Universe AI", "Could not start the placement task. No new objects were placed; any existing-object edits already applied remain.", "OK")->show();
                return;
            }
            scene->addChild(runner, 20000);
            return;
        }

        ui->updateButtons();
        ui->updateObjectInfoLabel();
        auto message = fmt::format(
            "Universe applied {} existing-object edits and {} object deletions. Use the editor Undo button to revert.",
            changed, deleted
        );
        FLAlertLayer::create("Universe AI", message.c_str(), "OK")->show();
    }

    void runInGameSelfTest() {
        auto mod = Mod::get();
        if (!mod) {
            FLAlertLayer::create("Universe Self-Test", "FAIL: Geode did not provide the Universe mod instance.", "OK")->show();
            return;
        }

        auto director = CCDirector::sharedDirector();
        auto scene = director ? director->getRunningScene() : nullptr;
        auto gameManager = GameManager::get();
        auto playLayer = PlayLayer::get();
        auto editor = LevelEditorLayer::get();
        bool settingsReadable = true;
        try {
            (void)mod->getSettingValue<bool>("safe-mode");
            (void)mod->getSettingValue<bool>("noclip");
            (void)mod->getSettingValue<bool>("speedhack-enabled");
            (void)mod->getSettingValue<bool>("show-hitboxes");
            (void)mod->getSettingValue<bool>("show-trajectory");
            (void)mod->getSettingValue<bool>("show-projection");
            (void)mod->getSettingValue<bool>("show-player-bounds");
            (void)mod->getSettingValue<bool>("click-sound");
            (void)mod->getSettingValue<bool>("floating-button");
        } catch (...) {
            settingsReadable = false;
        }

        auto status = [](bool ok) { return ok ? "PASS" : "FAIL"; };
        auto message = fmt::format(
            "Universe runtime self-test\n\n"
            "Mod instance: {}\n"
            "Game director and scene: {}\n"
            "GameManager: {}\n"
            "Settings access: {}\n\n"
            "Gameplay context: {}\n"
            "Editor context: {}\n\n"
            "Context lines are informational. This smoke test checks runtime access; it does not simulate gameplay or prove every feature works.",
            status(mod != nullptr),
            status(director != nullptr && scene != nullptr),
            status(gameManager != nullptr),
            status(settingsReadable),
            playLayer ? "PlayLayer available" : "No active PlayLayer",
            editor && editor->m_editorUI ? "Editor and EditorUI available" : "No open editor"
        );
        FLAlertLayer::create("Universe Self-Test", message.c_str(), "OK")->show();
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

        if (tag == 150) {
            runInGameSelfTest();
            return;
        }
        if (tag == 126) {
            generateAIDraft();
            return;
        }
        if (tag == 152) {
            if (m_aiTask.isPending()) {
                m_aiTask.cancel();
                if (m_aiStatus) m_aiStatus->setString("STATUS: CANCELLED");
                FLAlertLayer::create("Uni-1", "The generation request was cancelled. No new draft was saved.", "OK")->show();
            } else {
                FLAlertLayer::create("Uni-1", "There is no generation request to cancel.", "OK")->show();
            }
            return;
        }
        if (tag == 149) {
            applySavedDraftToEditor();
            return;
        }
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
            if (!m_aiPrompt || std::string(m_aiPrompt->getString()).empty()) {
                FLAlertLayer::create("Universe AI", "Write or paste a prompt before saving it.", "OK")->show();
                return;
            }
            auto path = mod->getSaveDir() / "universe-ai-prompt.txt";
            std::ofstream file(path.string(), std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                FLAlertLayer::create("Universe AI", "Universe could not save the prompt file.", "OK")->show();
                return;
            }
            file << m_aiPrompt->getString();
            file.close();
            FLAlertLayer::create("Universe AI", "Prompt saved in the Universe mod folder.", "OK")->show();
            return;
        }
        if (tag == 142) {
            if (m_aiPrompt) m_aiPrompt->setString("");
            return;
        }
        if (tag == 143) {
            if (!m_aiPrompt) return;
            auto path = mod->getSaveDir() / "universe-ai-prompt.txt";
            std::ifstream file(path.string());
            if (!file.is_open()) {
                FLAlertLayer::create("Universe AI", "No saved prompt was found yet.", "OK")->show();
                return;
            }
            std::string prompt((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (prompt.size() > 4000) prompt.resize(4000);
            m_aiPrompt->setString(prompt.c_str());
            FLAlertLayer::create("Universe AI", "Saved prompt loaded.", "OK")->show();
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
            bool enabled = mod->getSettingValue<bool>("local-icon-override");
            mod->setSettingValue<bool>("local-icon-override", !enabled);
            auto message = enabled
                ? "Local icon override disabled. Your official Geometry Dash icon selection was not changed."
                : "Local icon override enabled. The selected icon is rendered locally only; account unlocks and saved icon data are untouched.";
            FLAlertLayer::create("Universe Icon Lab", message, "OK")->show();
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
        if (tag == 147 || tag == 148) {
            auto typeID = mod->getSettingValue<int64_t>("preview-icon-type");
            typeID += tag == 148 ? 1 : -1;
            typeID = (typeID + 9) % 9;
            mod->setSettingValue<int64_t>("preview-icon-type", typeID);
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
            case 151: key = "show-player-bounds"; break;
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
        animateClosePanel(this);
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

        auto mod = Mod::get();
        if (!mod) return false;
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

        auto themeColor = mod->getSettingValue<ccColor4B>("panel-color");
        auto panel = CCDrawNode::create();
        if (!panel) return false;
        m_panelBackground = panel;
        m_panelSize.width = win.width * 0.82f;
        m_panelSize.height = win.height * 0.78f;
        panel->setContentSize(m_panelSize);
        panel->setPosition({win.width * 0.09f, win.height * 0.11f});
        drawRoundedPanel(panel, m_panelSize, themeColor);
        panel->setScale(0.94f);
        panel->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.f)));
        this->addChild(panel, 1);

        auto panelTitle = mod->getSettingValue<std::string>("panel-title");
        if (panelTitle.empty()) panelTitle = "UNIVERSE";
        auto title = CCLabelBMFont::create(panelTitle.c_str(), "bigFont.fnt");
        title->setScale(0.72f);
        title->setPosition({win.width * 0.5f, win.height * 0.81f});
        this->addChild(title, 2);

        m_pageTitle = CCLabelBMFont::create("Player", "goldFont.fnt");
        m_pageTitle->setScale(0.55f);
        m_pageTitle->setColor({190, 145, 255});
        m_pageTitle->setPosition({win.width * 0.5f, win.height * 0.72f});
        this->addChild(m_pageTitle, 3);

        auto accent = CCDrawNode::create();
        if (accent) {
            accent->drawSegment({18.f, m_panelSize.height - 13.f}, {m_panelSize.width - 18.f, m_panelSize.height - 13.f}, 2.5f, ccc4f(0.62f, 0.40f, 1.f, 0.95f));
            accent->drawSegment({18.f, 13.f}, {m_panelSize.width - 18.f, 13.f}, 1.2f, ccc4f(0.25f, 0.75f, 1.f, 0.48f));
            accent->setPosition(panel->getPosition());
            this->addChild(accent, 2);
        }

        auto status = CCLabelBMFont::create("LOCAL CLIENT  /  GD 2.2091", "bigFont.fnt");
        if (status) {
            status->setScale(0.25f);
            status->setColor({115, 225, 255});
            status->setPosition({win.width * 0.5f, win.height * 0.875f});
            this->addChild(status, 3);
        }

        m_menu = CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        this->addChild(m_menu, 5);
        m_tabLabels.clear();

        m_actionMenu = CCMenu::create();
        m_actionMenu->setPosition({0.f, 0.f});
        this->addChild(m_actionMenu, 6);

        float left = win.width * 0.12f;
        float tabY = win.height * 0.64f;
        Page pages[] = {Page::Player, Page::Practice, Page::Level, Page::Visuals,
                        Page::Recorder, Page::Keybinds, Page::Settings, Page::AI};
        int pageCount = 8;
        for (int i = 0; i < pageCount; ++i) {
            auto label = CCLabelBMFont::create(pageName(pages[i]), "bigFont.fnt");
            label->setScale(0.33f);
            label->setColor({185, 198, 225});
            m_tabLabels.push_back(label);
            auto item = CCMenuItemLabel::create(label, this, menu_selector(UniversePanel::onTab));
            item->setTag(200 + i);
            item->setPosition({left + i * (win.width * 0.105f), tabY});
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
        auto mod = Mod::get();
        if (!mod) return;
        showPage(static_cast<Page>(index));
    }
};

// Noclip and Speedhack are local gameplay modifiers. Safe Mode converts an
// attempt to Practice Mode before a requested modifier is applied; with Safe
// Mode disabled, the modifiers remain local client-side gameplay changes.
class $modify(UniversePlayerObject, PlayerObject) {
    bool isUniverseLocalPlayer() {
        auto layer = PlayLayer::get();
        return layer && layer->m_player1 == this;
    }

    int universeIconFrame(int frame, int type) {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("local-icon-override") || !isUniverseLocalPlayer())
            return frame;
        auto selectedType = static_cast<int>(std::clamp<int64_t>(
            mod->getSettingValue<int64_t>("preview-icon-type"), 0, 8
        ));
        if (selectedType != type) return frame;
        return static_cast<int>(std::clamp<int64_t>(
            mod->getSettingValue<int64_t>("preview-icon-id"), 1, 500
        ));
    }

    void applyUniverseLocalIconColors() {
        auto mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("local-icon-override") || !isUniverseLocalPlayer())
            return;
        auto primary = mod->getSettingValue<ccColor4B>("icon-primary-color");
        auto secondary = mod->getSettingValue<ccColor4B>("icon-secondary-color");
        auto glow = mod->getSettingValue<ccColor4B>("icon-glow-color");
        PlayerObject::setColor({primary.r, primary.g, primary.b});
        PlayerObject::setSecondColor({secondary.r, secondary.g, secondary.b});
        bool showGlow = mod->getSettingValue<bool>("local-icon-glow");
        if (m_iconGlow) {
            m_iconGlow->setColor({glow.r, glow.g, glow.b});
            m_iconGlow->setVisible(showGlow);
        }
        if (m_vehicleGlow) {
            m_vehicleGlow->setColor({glow.r, glow.g, glow.b});
            m_vehicleGlow->setVisible(showGlow);
        }
    }

    void updatePlayerFrame(int frame) {
        PlayerObject::updatePlayerFrame(universeIconFrame(frame, 0));
        applyUniverseLocalIconColors();
    }

    void updatePlayerGlow() {
        PlayerObject::updatePlayerGlow();
        applyUniverseLocalIconColors();
    }
    void updatePlayerShipFrame(int frame) {
        PlayerObject::updatePlayerShipFrame(universeIconFrame(frame, 1));
        applyUniverseLocalIconColors();
    }
    void updatePlayerRollFrame(int frame) {
        PlayerObject::updatePlayerRollFrame(universeIconFrame(frame, 2));
        applyUniverseLocalIconColors();
    }
    void updatePlayerBirdFrame(int frame) {
        PlayerObject::updatePlayerBirdFrame(universeIconFrame(frame, 3));
        applyUniverseLocalIconColors();
    }
    void updatePlayerDartFrame(int frame) {
        PlayerObject::updatePlayerDartFrame(universeIconFrame(frame, 4));
        applyUniverseLocalIconColors();
    }
    void createRobot(int frame) {
        PlayerObject::createRobot(universeIconFrame(frame, 5));
        applyUniverseLocalIconColors();
    }
    void createSpider(int frame) {
        PlayerObject::createSpider(universeIconFrame(frame, 6));
        applyUniverseLocalIconColors();
    }
    void updatePlayerSwingFrame(int frame) {
        PlayerObject::updatePlayerSwingFrame(universeIconFrame(frame, 7));
        applyUniverseLocalIconColors();
    }
    void updatePlayerJetpackFrame(int frame) {
        PlayerObject::updatePlayerJetpackFrame(universeIconFrame(frame, 8));
        applyUniverseLocalIconColors();
    }

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
    std::array<async::TaskHolder<web::WebResponse>, 7> m_bonusTasks;
    size_t m_bonusTaskIndex = 0;

    void notifyUni1Tap() {
        registerUniButtonTap();
        auto baseUrl = uni1ServerURL();
        auto installID = uniAIInstallID();
        if (!isAllowedUni1ServerURL(baseUrl) || installID.empty()) return;
        auto request = web::WebRequest()
            .header("X-Uni1-Install-ID", installID)
            .header("content-type", "application/json")
            .timeout(std::chrono::seconds(15));
        auto& task = m_bonusTasks[m_bonusTaskIndex++ % m_bonusTasks.size()];
        task.spawn(request.post(baseUrl + "/v1/bonus/tap"), [](web::WebResponse response) {
            if (!response.ok())
                log::warn("Uni-1 bonus tap could not be recorded by the server (HTTP {}).", response.code());
        });
    }

    void togglePanel() {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;
        if (auto existing = scene->getChildByID("universe-panel"_spr)) {
            animateClosePanel(static_cast<CCLayer*>(existing));
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
        setContentSize({68.f, 68.f});
        setTouchEnabled(true);
        auto background = CCDrawNode::create();
        if (background) {
            background->drawDot({34.f, 34.f}, 34.f, ccc4f(0.20f, 0.16f, 0.38f, 0.96f));
            this->addChild(background);
        }
        auto mod = Mod::get();
        if (!mod) return false;
        auto logoPath = (mod->getResourcesDir() / "logo.png").string();
        auto logo = CCSprite::create(logoPath.c_str());
        if (logo) {
            auto size = logo->getContentSize();
            if (size.width > 0.f && size.height > 0.f)
                logo->setScale(std::min(38.f / size.width, 38.f / size.height));
            logo->setPosition({34.f, 42.f});
            this->addChild(logo, 1);
        }
        auto buttonText = mod->getSettingValue<std::string>("floating-button-label");
        if (buttonText.empty()) buttonText = "UNIV";
        auto label = CCLabelBMFont::create(buttonText.c_str(), "goldFont.fnt");
        if (label) {
            label->setScale(0.24f);
            label->setPosition({34.f, 13.f});
            this->addChild(label, 2);
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
        if (parent && ccpDistance(getPosition(), local) > 4.f) {
            auto ghost = CCDrawNode::create();
            if (ghost) {
                ghost->drawDot({34.f, 34.f}, 31.f, ccc4f(0.48f, 0.36f, 0.92f, 0.42f));
                ghost->setPosition(getPosition());
                parent->addChild(ghost, getZOrder() - 1);
                ghost->runAction(CCSequence::create(
                    CCFadeTo::create(0.20f, 0),
                    CCCallFunc::create(ghost, callfunc_selector(CCNode::removeFromParent)),
                    nullptr
                ));
            }
        }
        setPosition({
            std::clamp(local.x, 34.f, win.width - 34.f),
            std::clamp(local.y, 34.f, win.height - 34.f)
        });
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_moved) {
            notifyUni1Tap();
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
            animateClosePanel(static_cast<CCLayer*>(existing));
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

        if (mod->getSettingValue<bool>("show-player-bounds") && m_player1) {
            auto bounds = m_player1->boundingBox();
            node->drawRect(
                bounds,
                ccc4f(0.f, 0.f, 0.f, 0.f),
                1.5f,
                ccc4f(0.22f, 1.f, 0.72f, 0.95f)
            );
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
            bool noclipRequested = mod->getSettingValue<bool>("noclip");
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

            // With Safe Mode disabled, modifiers remain active in Normal Mode.
            // With Safe Mode enabled, the attempt is marked Practice first.
            bool speedhackActive = speedhackRequested;
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
            animateClosePanel(static_cast<CCLayer*>(existing));
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
            animateClosePanel(static_cast<CCLayer*>(existing));
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
        if (enabled && mod->getSettingValue<bool>("safe-mode")) {
            if (auto layer = PlayLayer::get(); layer && !layer->m_isPracticeMode)
                layer->m_isPracticeMode = true;
        }
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