#include "VersusLobbyPopup.hpp"
#include "VersusApi.hpp"
#include "VersusUi.hpp"
#include "VersusCountdown.hpp"
#include "VersusMatch.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/cocos.hpp>
#include <vector>

using namespace geode::prelude;

namespace petus::versus {

    // ======================================================================
    //  Invite sub-popup: nick field + scrollable "top players" invite list
    // ======================================================================
    class InviteSubPopup : public geode::Popup {
    protected:
        matjson::Value m_top;
        TextInput* m_input = nullptr;

        bool init() override {
            const float w = 320.f, h = 260.f;
            if (!geode::Popup::init(w, h)) return false;
            this->setTitle("Пригласить игрока");

            m_input = TextInput::create(w - 120.f, "Ник игрока", "bigFont.fnt");
            m_input->setPosition({ w / 2.f - 30.f, h - 50.f });
            m_input->setCommonFilter(CommonFilter::Any);
            m_mainLayer->addChild(m_input);

            auto sendSpr = ButtonSprite::create("Позвать", "goldFont.fnt", "GJ_button_01.png", 0.8f);
            sendSpr->setScale(0.6f);
            auto sendBtn = CCMenuItemExt::createSpriteExtra(sendSpr, [this](CCObject*) {
                auto nick = std::string(m_input->getString());
                if (nick.empty()) return;
                this->inviteByName(nick);
            });
            auto menu = CCMenu::create();
            menu->addChild(sendBtn);
            menu->setPosition({ w - 45.f, h - 50.f });
            m_mainLayer->addChild(menu);

            auto scroll = ScrollLayer::create({ w - 50.f, h - 100.f });
            scroll->setPosition({ 25.f, 15.f });
            m_mainLayer->addChild(scroll);
            m_scroll = scroll;

            this->loadTop();
            return true;
        }

        ScrollLayer* m_scroll = nullptr;

        void loadTop() {
            Ref<InviteSubPopup> self = this;
            petus::versus::get("/api/versus/top", [self](bool ok, matjson::Value data) {
                if (!self || !ok) return;
                self->m_top = data;
                self->fillTop();
            });
        }

        void fillTop() {
            if (!m_scroll) return;
            m_scroll->m_contentLayer->removeAllChildren();

            std::vector<matjson::Value> players = jarr(m_top);

            const float rowH = 34.f;
            const float width = m_scroll->getContentSize().width;
            const float totalH = std::max<float>(players.size() * rowH, m_scroll->getContentSize().height);
            m_scroll->m_contentLayer->setContentSize({ width, totalH });

            for (size_t i = 0; i < players.size(); i++) {
                auto& p = players[i];
                float rowY = totalH - (i + 1) * rowH;
                int accountID = jint(p, "accountID");
                std::string name = jstr(p, "name", "?");
                int stars = jint(p, "stars");

                auto bg = CCLayerColor::create(
                    (i % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20), width, rowH - 2.f);
                bg->setPosition({ 0.f, rowY });
                m_scroll->m_contentLayer->addChild(bg);

                auto label = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
                label->setAnchorPoint({ 0.f, 0.5f });
                label->setScale(0.45f);
                label->setPosition({ 8.f, rowY + rowH / 2.f });
                m_scroll->m_contentLayer->addChild(label);

                auto starLabel = CCLabelBMFont::create(fmt::format("{}★", stars).c_str(), "goldFont.fnt");
                starLabel->setAnchorPoint({ 0.f, 0.5f });
                starLabel->setScale(0.4f);
                starLabel->setPosition({ width * 0.55f, rowY + rowH / 2.f });
                m_scroll->m_contentLayer->addChild(starLabel);

                auto invSpr = ButtonSprite::create("+", "goldFont.fnt", "GJ_button_05.png", 0.9f);
                invSpr->setScale(0.6f);
                auto invBtn = CCMenuItemExt::createSpriteExtra(invSpr, [this, accountID](CCObject*) {
                    this->inviteByAccount(accountID);
                });
                auto menu = CCMenu::create();
                menu->addChild(invBtn);
                menu->setPosition({ width - 24.f, rowY + rowH / 2.f });
                m_scroll->m_contentLayer->addChild(menu);
            }
            m_scroll->moveToTop();
        }

        void inviteByName(const std::string& nick) {
            matjson::Value body;
            body["name"] = nick;
            this->sendInvite(body);
        }
        void inviteByAccount(int accountID) {
            matjson::Value body;
            body["accountID"] = accountID;
            this->sendInvite(body);
        }
        void sendInvite(matjson::Value body) {
            Ref<InviteSubPopup> self = this;
            petus::versus::post("/api/versus/lobby/invite", body, [self](bool ok, matjson::Value data) {
                if (!self) return;
                if (ok && jbool(data, "ok")) {
                    Notification::create("Приглашение отправлено", NotificationIcon::Success)->show();
                } else {
                    Notification::create("Не удалось пригласить", NotificationIcon::Error)->show();
                }
            });
        }

    public:
        static InviteSubPopup* create() {
            auto ret = new InviteSubPopup();
            if (ret->init()) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }
    };

    // ======================================================================
    //  Levels sub-popup: host enters up to 10 level IDs to form the pool
    // ======================================================================
    class LevelsSubPopup : public geode::Popup {
    protected:
        struct PoolItem { int id; std::string name; int seconds; };
        std::vector<PoolItem> m_items;
        TextInput* m_input = nullptr;
        ScrollLayer* m_scroll = nullptr;

        bool init() override {
            const float w = 340.f, h = 280.f;
            if (!geode::Popup::init(w, h)) return false;
            this->setTitle("Выбрать уровни (до 10)");

            m_input = TextInput::create(w - 140.f, "ID уровня", "bigFont.fnt");
            m_input->setPosition({ w / 2.f - 40.f, h - 50.f });
            m_input->setCommonFilter(CommonFilter::Uint);
            m_mainLayer->addChild(m_input);

            auto addSpr = ButtonSprite::create("Добавить", "goldFont.fnt", "GJ_button_01.png", 0.8f);
            addSpr->setScale(0.55f);
            auto addBtn = CCMenuItemExt::createSpriteExtra(addSpr, [this](CCObject*) {
                this->addLevel();
            });
            auto menuTop = CCMenu::create();
            menuTop->addChild(addBtn);
            menuTop->setPosition({ w - 50.f, h - 50.f });
            m_mainLayer->addChild(menuTop);

            m_scroll = ScrollLayer::create({ w - 50.f, h - 120.f });
            m_scroll->setPosition({ 25.f, 50.f });
            m_mainLayer->addChild(m_scroll);

            auto saveSpr = ButtonSprite::create("Сохранить пул", "goldFont.fnt", "GJ_button_01.png", 0.9f);
            saveSpr->setScale(0.7f);
            auto saveBtn = CCMenuItemExt::createSpriteExtra(saveSpr, [this](CCObject*) {
                this->save();
            });
            auto menuBottom = CCMenu::create();
            menuBottom->addChild(saveBtn);
            menuBottom->setPosition({ w / 2.f, 25.f });
            m_mainLayer->addChild(menuBottom);

            this->fill();
            return true;
        }

        void addLevel() {
            if (m_items.size() >= 10) {
                Notification::create("Максимум 10 уровней", NotificationIcon::Warning)->show();
                return;
            }
            auto idStr = std::string(m_input->getString());
            if (idStr.empty()) return;
            int id = 0;
            try { id = std::stoi(idStr); } catch (...) { return; }
            if (id <= 0) return;
            for (auto& it : m_items) if (it.id == id) return; // dedupe

            // Try to resolve a name/length from an already-known level; else default.
            std::string name = fmt::format("Уровень {}", id);
            int seconds = 30;
            if (auto* glm = GameLevelManager::sharedState()) {
                if (auto* lvl = glm->getSavedLevel(id)) {
                    if (lvl->m_levelName.size() > 0) name = lvl->m_levelName;
                }
            }
            m_items.push_back({ id, name, seconds });
            m_input->setString("");
            this->fill();
        }

        void removeAt(size_t idx) {
            if (idx < m_items.size()) {
                m_items.erase(m_items.begin() + idx);
                this->fill();
            }
        }

        void fill() {
            m_scroll->m_contentLayer->removeAllChildren();
            const float rowH = 32.f;
            const float width = m_scroll->getContentSize().width;
            const float totalH = std::max<float>(m_items.size() * rowH, m_scroll->getContentSize().height);
            m_scroll->m_contentLayer->setContentSize({ width, totalH });

            for (size_t i = 0; i < m_items.size(); i++) {
                float rowY = totalH - (i + 1) * rowH;
                auto& it = m_items[i];

                auto bg = CCLayerColor::create(
                    (i % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20), width, rowH - 2.f);
                bg->setPosition({ 0.f, rowY });
                m_scroll->m_contentLayer->addChild(bg);

                auto label = CCLabelBMFont::create(
                    fmt::format("{} (#{})", it.name, it.id).c_str(), "bigFont.fnt");
                label->setAnchorPoint({ 0.f, 0.5f });
                label->setScale(0.4f);
                label->setPosition({ 8.f, rowY + rowH / 2.f });
                m_scroll->m_contentLayer->addChild(label);

                auto delSpr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
                if (delSpr) delSpr->setScale(0.6f);
                auto delBtn = CCMenuItemExt::createSpriteExtra(delSpr, [this, i](CCObject*) {
                    this->removeAt(i);
                });
                auto menu = CCMenu::create();
                menu->addChild(delBtn);
                menu->setPosition({ width - 20.f, rowY + rowH / 2.f });
                m_scroll->m_contentLayer->addChild(menu);
            }
            m_scroll->moveToTop();
        }

        void save() {
            if (m_items.empty()) {
                Notification::create("Добавьте хотя бы один уровень", NotificationIcon::Warning)->show();
                return;
            }
            matjson::Value levels = matjson::Value::array();
            for (auto& it : m_items) {
                matjson::Value l;
                l["id"] = it.id;
                l["name"] = it.name;
                l["seconds"] = it.seconds;
                levels.push(l);
            }
            matjson::Value body;
            body["levels"] = levels;

            Ref<LevelsSubPopup> self = this;
            petus::versus::post("/api/versus/lobby/levels", body, [self](bool ok, matjson::Value) {
                if (!self) return;
                if (ok) {
                    Notification::create("Пул уровней сохранён", NotificationIcon::Success)->show();
                    self->onClose(nullptr);
                } else {
                    Notification::create("Ошибка сохранения пула", NotificationIcon::Error)->show();
                }
            });
        }

    public:
        static LevelsSubPopup* create() {
            auto ret = new LevelsSubPopup();
            if (ret->init()) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }
    };

    // ======================================================================
    //  Main lobby popup
    // ======================================================================
    bool VersusLobbyPopup::init() {
        const float w = 420.f, h = 300.f;
        if (!geode::Popup::init(w, h)) return false;
        this->setTitle("Versus");

        m_content = CCNode::create();
        m_content->setContentSize({ w, h });
        m_content->setAnchorPoint({ 0.f, 0.f });
        m_content->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_content);

        this->rebuild();

        // Poll: invites in menu mode (~3s), lobby DTO in lobby mode (~0.5s). One
        // scheduler at 0.5s, throttled for the menu case by a frame counter.
        this->schedule(schedule_selector(VersusLobbyPopup::pollTick), 0.5f);
        return true;
    }

    bool VersusLobbyPopup::amHost() const {
        return jbool(m_lobby, "isHost");
    }
    int VersusLobbyPopup::myReady() const {
        if (!m_lobby.contains("members") || !m_lobby["members"].isArray()) return 0;
        for (auto const& m : m_lobby["members"]) {
            if (jint(m, "accountID") == session().account) return jbool(m, "ready") ? 1 : 0;
        }
        return 0;
    }
    int VersusLobbyPopup::myVote() const {
        if (!m_lobby.contains("members") || !m_lobby["members"].isArray()) return 0;
        for (auto const& m : m_lobby["members"]) {
            if (jint(m, "accountID") == session().account) return jint(m, "vote");
        }
        return 0;
    }

    void VersusLobbyPopup::pollTick(float) {
        if (m_closing) return;
        if (m_mode == Mode::Menu) {
            static int s_frame = 0;
            // ~every 3s (0.5s * 6).
            if (++s_frame % 6 == 0) this->refreshInvites();
        } else {
            this->refreshLobby();
        }
    }

    void VersusLobbyPopup::refreshInvites() {
        Ref<VersusLobbyPopup> self = this;
        petus::versus::get("/api/versus/invites", [self](bool ok, matjson::Value data) {
            if (!self || self->m_closing || !ok) return;
            if (self->m_mode != Mode::Menu) return;
            self->m_invites = data;
            self->buildMenu();
        });
    }

    void VersusLobbyPopup::refreshLobby() {
        if (m_lobbyId.empty()) return;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::get("/api/versus/lobby/" + m_lobbyId, [self](bool ok, matjson::Value data) {
            if (!self || self->m_closing || !ok) return;
            if (self->m_mode != Mode::Lobby) return;
            self->m_lobby = data;

            std::string state = jstr(data, "state", "waiting");
            if (state == "countdown") {
                // Transition into the match. Stop polling, close, run countdown.
                self->m_closing = true;
                self->unschedule(schedule_selector(VersusLobbyPopup::pollTick));

                long long cdMs = jll(data, "countdownMs", 5000);
                int levelID = jint(data, "chosenLevel");
                std::string lobbyId = self->m_lobbyId;
                int myAcc = session().account;

                self->onClose(nullptr);

                VersusCountdown::create(cdMs, [lobbyId, levelID, myAcc]() {
                    enterMatch(lobbyId, levelID, myAcc);
                });
                return;
            }
            self->buildLobby();
        });
    }

    void VersusLobbyPopup::enterLobbyDTO(matjson::Value dto) {
        m_lobby = dto;
        m_lobbyId = jstr(dto, "id");
        m_mode = Mode::Lobby;
        this->rebuild();
    }

    void VersusLobbyPopup::rebuild() {
        if (!m_content) return;
        m_content->removeAllChildren();
        if (m_mode == Mode::Menu) this->buildMenu();
        else this->buildLobby();
    }

    // ---- Menu view: create lobby + pending invites -----------------------
    void VersusLobbyPopup::buildMenu() {
        if (!m_content) return;
        m_content->removeAllChildren();
        const float w = m_content->getContentSize().width;
        const float h = m_content->getContentSize().height;

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_content->addChild(menu);

        auto hint = CCLabelBMFont::create("Создайте лобби или примите приглашение", "bigFont.fnt");
        hint->setScale(0.4f);
        hint->setPosition({ w / 2.f, h - 55.f });
        m_content->addChild(hint);

        auto createRankedSpr = ButtonSprite::create("Создать (ranked)", "goldFont.fnt", "GJ_button_01.png", 1.0f);
        createRankedSpr->setScale(0.7f);
        auto createRankedBtn = CCMenuItemExt::createSpriteExtra(createRankedSpr, [this](CCObject*) {
            this->createLobby(true);
        });
        createRankedBtn->setPosition({ w / 2.f - 90.f, h - 90.f });
        menu->addChild(createRankedBtn);

        auto createSpr = ButtonSprite::create("Создать (обычное)", "goldFont.fnt", "GJ_button_05.png", 1.0f);
        createSpr->setScale(0.7f);
        auto createBtn = CCMenuItemExt::createSpriteExtra(createSpr, [this](CCObject*) {
            this->createLobby(false);
        });
        createBtn->setPosition({ w / 2.f + 90.f, h - 90.f });
        menu->addChild(createBtn);

        auto invTitle = CCLabelBMFont::create("Приглашения:", "goldFont.fnt");
        invTitle->setScale(0.5f);
        invTitle->setAnchorPoint({ 0.f, 0.5f });
        invTitle->setPosition({ 30.f, h - 120.f });
        m_content->addChild(invTitle);

        auto scroll = ScrollLayer::create({ w - 60.f, h - 160.f });
        scroll->setPosition({ 30.f, 15.f });
        m_content->addChild(scroll);

        std::vector<matjson::Value> invites = jarr(m_invites);

        const float rowH = 40.f;
        const float width = scroll->getContentSize().width;
        const float totalH = std::max<float>(invites.size() * rowH, scroll->getContentSize().height);
        scroll->m_contentLayer->setContentSize({ width, totalH });

        for (size_t i = 0; i < invites.size(); i++) {
            auto& inv = invites[i];
            float rowY = totalH - (i + 1) * rowH;
            std::string lobbyId = jstr(inv, "lobbyId");
            std::string host = jstr(inv, "host", "?");
            int players = 0;
            if (inv.contains("players")) {
                if (inv["players"].isArray())
                    players = static_cast<int>(jarr(inv, "players").size());
                else players = jint(inv, "players");
            }
            bool ranked = jbool(inv, "ranked");

            auto bg = CCLayerColor::create(
                (i % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20), width, rowH - 2.f);
            bg->setPosition({ 0.f, rowY });
            scroll->m_contentLayer->addChild(bg);

            auto label = CCLabelBMFont::create(
                fmt::format("{} {} ({} игр.)", host, ranked ? "[ranked]" : "", players).c_str(),
                "bigFont.fnt");
            label->setAnchorPoint({ 0.f, 0.5f });
            label->setScale(0.4f);
            label->setPosition({ 8.f, rowY + rowH / 2.f });
            scroll->m_contentLayer->addChild(label);

            auto rowMenu = CCMenu::create();
            rowMenu->setPosition({ 0.f, 0.f });
            scroll->m_contentLayer->addChild(rowMenu);

            auto acceptSpr = ButtonSprite::create("Войти", "goldFont.fnt", "GJ_button_01.png", 0.8f);
            acceptSpr->setScale(0.5f);
            auto acceptBtn = CCMenuItemExt::createSpriteExtra(acceptSpr, [this, lobbyId](CCObject*) {
                this->respondInvite(lobbyId, true);
            });
            acceptBtn->setPosition({ width - 70.f, rowY + rowH / 2.f });
            rowMenu->addChild(acceptBtn);

            auto declineSpr = ButtonSprite::create("X", "goldFont.fnt", "GJ_button_06.png", 0.8f);
            declineSpr->setScale(0.5f);
            auto declineBtn = CCMenuItemExt::createSpriteExtra(declineSpr, [this, lobbyId](CCObject*) {
                this->respondInvite(lobbyId, false);
            });
            declineBtn->setPosition({ width - 24.f, rowY + rowH / 2.f });
            rowMenu->addChild(declineBtn);
        }
        scroll->moveToTop();

        this->refreshInvites();
    }

    // ---- Lobby view ------------------------------------------------------
    void VersusLobbyPopup::buildLobby() {
        if (!m_content) return;
        m_content->removeAllChildren();
        const float w = m_content->getContentSize().width;
        const float h = m_content->getContentSize().height;

        std::string state = jstr(m_lobby, "state", "waiting");
        bool ranked = jbool(m_lobby, "ranked");
        int timeMult = jint(m_lobby, "timeMultiplier", 5);
        std::string chosenName = jstr(m_lobby, "chosenLevelName");

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_content->addChild(menu);

        // Header line.
        auto header = CCLabelBMFont::create(
            fmt::format("#{}  {}  [{}]", m_lobbyId, ranked ? "ranked" : "обычное", state).c_str(),
            "bigFont.fnt");
        header->setScale(0.4f);
        header->setAnchorPoint({ 0.f, 0.5f });
        header->setPosition({ 20.f, h - 50.f });
        m_content->addChild(header);

        // Members list (left half).
        auto membersScroll = ScrollLayer::create({ w * 0.52f - 20.f, h - 110.f });
        membersScroll->setPosition({ 18.f, 50.f });
        m_content->addChild(membersScroll);

        std::vector<matjson::Value> members = jarr(m_lobby, "members");

        const float rowH = 40.f;
        const float mwidth = membersScroll->getContentSize().width;
        const float mtotalH = std::max<float>(members.size() * rowH, membersScroll->getContentSize().height);
        membersScroll->m_contentLayer->setContentSize({ mwidth, mtotalH });

        for (size_t i = 0; i < members.size(); i++) {
            auto& m = members[i];
            float rowY = mtotalH - (i + 1) * rowH;
            std::string name = jstr(m, "name", "?");
            std::string icon = jstr(m, "icon");
            bool ready = jbool(m, "ready");
            int vote = jint(m, "vote");

            auto bg = CCLayerColor::create(
                (i % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20), mwidth, rowH - 2.f);
            bg->setPosition({ 0.f, rowY });
            membersScroll->m_contentLayer->addChild(bg);

            if (auto cube = makeCube(icon)) {
                cube->setScale(0.55f);
                cube->setPosition({ 18.f, rowY + rowH / 2.f });
                membersScroll->m_contentLayer->addChild(cube);
            }

            auto label = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
            label->setAnchorPoint({ 0.f, 0.5f });
            label->setScale(0.4f);
            label->setPosition({ 36.f, rowY + rowH / 2.f + 7.f });
            membersScroll->m_contentLayer->addChild(label);

            if (vote > 0) {
                auto voteLabel = CCLabelBMFont::create(fmt::format("голос: #{}", vote).c_str(), "chatFont.fnt");
                voteLabel->setAnchorPoint({ 0.f, 0.5f });
                voteLabel->setScale(0.55f);
                voteLabel->setPosition({ 36.f, rowY + rowH / 2.f - 8.f });
                membersScroll->m_contentLayer->addChild(voteLabel);
            }

            auto check = CCSprite::createWithSpriteFrameName(
                ready ? "GJ_completesIcon_001.png" : "GJ_deleteIcon_001.png");
            if (check) {
                check->setScale(0.6f);
                check->setPosition({ mwidth - 16.f, rowY + rowH / 2.f });
                membersScroll->m_contentLayer->addChild(check);
            }
        }
        membersScroll->moveToTop();

        // Right column: contextual controls.
        float rx = w * 0.56f;
        float ry = h - 70.f;
        const float step = 34.f;

        if (state == "waiting" && amHost()) {
            auto inviteSpr = ButtonSprite::create("Пригласить", "goldFont.fnt", "GJ_button_01.png", 1.0f);
            inviteSpr->setScale(0.55f);
            auto inviteBtn = CCMenuItemExt::createSpriteExtra(inviteSpr, [this](CCObject*) { this->openInviteSub(); });
            inviteBtn->setPosition({ rx + 80.f, ry });
            menu->addChild(inviteBtn); ry -= step;

            auto levelsSpr = ButtonSprite::create("Выбрать уровни", "goldFont.fnt", "GJ_button_01.png", 1.0f);
            levelsSpr->setScale(0.55f);
            auto levelsBtn = CCMenuItemExt::createSpriteExtra(levelsSpr, [this](CCObject*) { this->openLevelsSub(); });
            levelsBtn->setPosition({ rx + 80.f, ry });
            menu->addChild(levelsBtn); ry -= step;

            // Time multiplier selector.
            auto timeLabel = CCLabelBMFont::create(fmt::format("Время: x{}", timeMult).c_str(), "bigFont.fnt");
            timeLabel->setScale(0.4f);
            timeLabel->setPosition({ rx + 80.f, ry });
            m_content->addChild(timeLabel); ry -= 26.f;

            auto timeMenu = CCMenu::create();
            timeMenu->setPosition({ 0.f, 0.f });
            m_content->addChild(timeMenu);
            int mults[3] = { 2, 5, 10 };
            for (int k = 0; k < 3; k++) {
                int mult = mults[k];
                auto spr = ButtonSprite::create(fmt::format("x{}", mult).c_str(), "goldFont.fnt",
                    mult == timeMult ? "GJ_button_02.png" : "GJ_button_04.png", 1.0f);
                spr->setScale(0.5f);
                auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, mult](CCObject*) { this->doTimeLimit(mult); });
                btn->setPosition({ rx + 30.f + k * 50.f, ry });
                timeMenu->addChild(btn);
            }
            ry -= step;

            auto startSpr = ButtonSprite::create("Начать", "goldFont.fnt", "GJ_button_01.png", 1.0f);
            startSpr->setScale(0.65f);
            auto startBtn = CCMenuItemExt::createSpriteExtra(startSpr, [this](CCObject*) { this->doStart(); });
            startBtn->setPosition({ rx + 80.f, ry });
            menu->addChild(startBtn); ry -= step;
        } else if (state == "waiting") {
            auto waitLabel = CCLabelBMFont::create("Ждём хоста...", "bigFont.fnt");
            waitLabel->setScale(0.5f);
            waitLabel->setPosition({ rx + 80.f, ry });
            m_content->addChild(waitLabel); ry -= step;
        }

        // Voting: show pool as tappable list.
        if (state == "voting") {
            auto voteTitle = CCLabelBMFont::create("Голосование за уровень:", "goldFont.fnt");
            voteTitle->setScale(0.4f);
            voteTitle->setPosition({ rx + 80.f, ry });
            m_content->addChild(voteTitle); ry -= 24.f;

            std::vector<matjson::Value> pool = jarr(m_lobby, "pool");

            // Tally votes per level.
            std::map<int, int> tally;
            for (auto const& m : members) {
                int v = jint(m, "vote");
                if (v > 0) tally[v]++;
            }

            auto poolMenu = CCMenu::create();
            poolMenu->setPosition({ 0.f, 0.f });
            m_content->addChild(poolMenu);

            int mine = myVote();
            for (size_t i = 0; i < pool.size() && i < 6; i++) {
                auto& p = pool[i];
                int id = jint(p, "id");
                std::string name = jstr(p, "name", fmt::format("Уровень {}", id));
                int count = tally.count(id) ? tally[id] : 0;

                auto spr = ButtonSprite::create(
                    fmt::format("{} ({})", name, count).c_str(), "goldFont.fnt",
                    id == mine ? "GJ_button_02.png" : "GJ_button_04.png", 1.0f);
                spr->setScale(0.45f);
                auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, id](CCObject*) { this->doVote(id); });
                btn->setPosition({ rx + 80.f, ry });
                poolMenu->addChild(btn);
                ry -= 28.f;
            }
        }

        // Ready toggle + Leave (always available in waiting/voting).
        if (state == "waiting" || state == "voting") {
            bool ready = myReady() != 0;
            auto readySpr = ButtonSprite::create(
                ready ? "Не готов" : "Готов", "goldFont.fnt",
                ready ? "GJ_button_06.png" : "GJ_button_01.png", 1.0f);
            readySpr->setScale(0.6f);
            auto readyBtn = CCMenuItemExt::createSpriteExtra(readySpr, [this](CCObject*) { this->doReady(); });
            readyBtn->setPosition({ w - 70.f, 30.f });
            menu->addChild(readyBtn);
        }

        auto leaveSpr = ButtonSprite::create("Выйти", "goldFont.fnt", "GJ_button_06.png", 1.0f);
        leaveSpr->setScale(0.55f);
        auto leaveBtn = CCMenuItemExt::createSpriteExtra(leaveSpr, [this](CCObject*) { this->doLeave(); });
        leaveBtn->setPosition({ 60.f, 30.f });
        menu->addChild(leaveBtn);

        if (!chosenName.empty()) {
            auto chosen = CCLabelBMFont::create(fmt::format("Уровень: {}", chosenName).c_str(), "bigFont.fnt");
            chosen->setScale(0.4f);
            chosen->setAnchorPoint({ 0.f, 0.5f });
            chosen->setPosition({ 20.f, h - 66.f });
            m_content->addChild(chosen);
        }
    }

    // ---- actions ---------------------------------------------------------
    void VersusLobbyPopup::createLobby(bool ranked) {
        if (m_busy) return;
        m_busy = true;
        matjson::Value body;
        body["ranked"] = ranked ? 1 : 0;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/create", body, [self](bool ok, matjson::Value data) {
            if (!self) return;
            self->m_busy = false;
            if (!ok) {
                Notification::create("Не удалось создать лобби", NotificationIcon::Error)->show();
                return;
            }
            self->enterLobbyDTO(data);
        });
    }

    void VersusLobbyPopup::respondInvite(const std::string& lobbyId, bool accept) {
        if (m_busy) return;
        m_busy = true;
        matjson::Value body;
        body["lobbyId"] = lobbyId;
        body["accept"] = accept ? 1 : 0;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/invite/respond", body, [self, accept](bool ok, matjson::Value data) {
            if (!self) return;
            self->m_busy = false;
            if (!ok) {
                Notification::create("Ошибка ответа на приглашение", NotificationIcon::Error)->show();
                return;
            }
            if (accept) self->enterLobbyDTO(data);
            else self->refreshInvites();
        });
    }

    void VersusLobbyPopup::doReady() {
        matjson::Value body;
        body["ready"] = myReady() ? 0 : 1;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/ready", body, [self](bool ok, matjson::Value data) {
            if (!self || !ok) return;
            self->m_lobby = data;
            self->buildLobby();
        });
    }

    void VersusLobbyPopup::doVote(int levelID) {
        matjson::Value body;
        body["levelID"] = levelID;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/vote", body, [self](bool ok, matjson::Value data) {
            if (!self || !ok) return;
            self->m_lobby = data;
            self->buildLobby();
        });
    }

    void VersusLobbyPopup::doStart() {
        if (m_busy) return;
        m_busy = true;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/start", matjson::Value::object(), [self](bool ok, matjson::Value data) {
            if (!self) return;
            self->m_busy = false;
            if (!ok) {
                Notification::create("Не удалось начать", NotificationIcon::Error)->show();
                return;
            }
            self->m_lobby = data;
        });
    }

    void VersusLobbyPopup::doLeave() {
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/leave", matjson::Value::object(), [self](bool, matjson::Value) {
            if (!self) return;
            self->m_lobbyId.clear();
            self->m_lobby = matjson::Value::object();
            self->m_mode = Mode::Menu;
            self->rebuild();
        });
    }

    void VersusLobbyPopup::doTimeLimit(int multiplier) {
        matjson::Value body;
        body["multiplier"] = multiplier;
        Ref<VersusLobbyPopup> self = this;
        petus::versus::post("/api/versus/lobby/timelimit", body, [self](bool ok, matjson::Value data) {
            if (!self || !ok) return;
            self->m_lobby = data;
            self->buildLobby();
        });
    }

    void VersusLobbyPopup::openInviteSub() {
        if (auto p = InviteSubPopup::create()) p->show();
    }
    void VersusLobbyPopup::openLevelsSub() {
        if (auto p = LevelsSubPopup::create()) p->show();
    }

    void VersusLobbyPopup::onClose(CCObject* sender) {
        m_closing = true;
        this->unschedule(schedule_selector(VersusLobbyPopup::pollTick));
        geode::Popup::onClose(sender);
    }

    VersusLobbyPopup* VersusLobbyPopup::create() {
        auto ret = new VersusLobbyPopup();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    void openVersusMenu() {
        if (auto p = VersusLobbyPopup::create()) p->show();
    }

}
