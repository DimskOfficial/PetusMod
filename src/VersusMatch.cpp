#include "VersusMatch.hpp"
#include "VersusApi.hpp"
#include "VersusUi.hpp"
#include "VersusResults.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/Mod.hpp>
#include <map>

using namespace geode::prelude;

namespace petus::versus {

    MatchContext& matchCtx() {
        static MatchContext ctx;
        return ctx;
    }

    // Delegate that waits for the chosen level to download, then enters PlayLayer.
    // Kept as a heap object owning itself for the duration of the request.
    class VersusDownloadDelegate : public LevelDownloadDelegate {
    public:
        int m_levelID = 0;

        static VersusDownloadDelegate* create(int levelID) {
            auto d = new VersusDownloadDelegate();
            d->m_levelID = levelID;
            return d;
        }

        void enterWith(GJGameLevel* level) {
            if (!level) return;
            auto scene = PlayLayer::scene(level, false, false);
            CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
        }

        void levelDownloadFinished(GJGameLevel* level) override {
            auto* glm = GameLevelManager::sharedState();
            if (glm && glm->m_levelDownloadDelegate == this) glm->m_levelDownloadDelegate = nullptr;
            enterWith(level);
            delete this;
        }

        void levelDownloadFailed(int response) override {
            log::warn("Versus: level {} download failed ({})", m_levelID, response);
            auto* glm = GameLevelManager::sharedState();
            if (glm && glm->m_levelDownloadDelegate == this) glm->m_levelDownloadDelegate = nullptr;
            FLAlertLayer::create("Versus", "Не удалось загрузить уровень матча.", "OK")->show();
            endMatch();
            delete this;
        }
    };

    void enterMatch(const std::string& lobbyId, int levelID, int myAccountID) {
        auto& ctx = matchCtx();
        ctx.active = true;
        ctx.lobbyId = lobbyId;
        ctx.levelID = levelID;
        ctx.myAccountID = myAccountID;
        ctx.remainingMs = 0;

        auto* glm = GameLevelManager::sharedState();
        if (!glm) {
            FLAlertLayer::create("Versus", "GameLevelManager недоступен.", "OK")->show();
            endMatch();
            return;
        }

        // If we already have the level string cached, enter straight away.
        if (auto* saved = glm->getSavedLevel(levelID)) {
            if (saved->m_levelString.size() > 0) {
                auto scene = PlayLayer::scene(saved, false, false);
                CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
                return;
            }
        }

        // Otherwise download it, then enter on completion.
        auto delegate = VersusDownloadDelegate::create(levelID);
        glm->m_levelDownloadDelegate = delegate;
        glm->downloadLevel(levelID, false, 0);
    }

    void endMatch() {
        auto& ctx = matchCtx();
        ctx.active = false;
        ctx.lobbyId.clear();
        ctx.levelID = 0;
        ctx.remainingMs = 0;
    }

}

using namespace petus::versus;

// ---- PlayLayer hook: live race overlay + progress reporting ----------------
class $modify(VersusPlayLayer, PlayLayer) {
    struct Fields {
        bool armed = false;
        bool finishedReported = false;
        int localDeaths = 0;
        bool deathPending = false;     // a death happened since the last pos POST
        Ref<CCNode> overlay = nullptr; // container inside m_objectLayer for ghosts
        // accountID -> its ghost node (holds cube + name label)
        std::map<int, Ref<CCNode>> ghosts;
        long long remainingMs = -1;
    };

    // Build (once) a ghost node for another player: cube + optional nick label.
    CCNode* makeGhost(const OtherPlayer& op) {
        auto container = CCNode::create();
        container->setContentSize({ 0, 0 });

        if (!Mod::get()->getSettingValue<bool>("versus-hide-cubes")) {
            if (auto cube = petus::versus::makeCube(op.icon)) {
                cube->setScale(0.9f);
                if (auto spr = typeinfo_cast<CCSprite*>(cube)) spr->setOpacity(150);
                cube->setPosition({ 0.f, 0.f });
                container->addChild(cube);
            }
        }
        if (!Mod::get()->getSettingValue<bool>("versus-hide-nicks") && !op.name.empty()) {
            auto label = CCLabelBMFont::create(op.name.c_str(), "chatFont.fnt");
            label->setScale(0.5f);
            label->setOpacity(180);
            label->setPosition({ 0.f, 22.f });
            container->addChild(label);
        }
        return container;
    }

    void updateGhosts(std::vector<OtherPlayer> const& players) {
        auto f = m_fields.self();
        if (!f->overlay) return;

        float baselineY = m_player1 ? m_player1->getPositionY() : 120.f;

        for (auto const& op : players) {
            if (op.accountID == matchCtx().myAccountID) continue;

            auto it = f->ghosts.find(op.accountID);
            CCNode* ghost = nullptr;
            if (it == f->ghosts.end()) {
                ghost = makeGhost(op);
                f->overlay->addChild(ghost);
                f->ghosts[op.accountID] = ghost;
            } else {
                ghost = it->second;
            }
            if (!ghost) continue;

            // Ghosts live inside m_objectLayer, so world X scrolls naturally.
            ghost->setPosition({ static_cast<float>(op.x), baselineY });
            ghost->setVisible(!op.finished);
        }
    }

    void sendPos(float) {
        auto f = m_fields.self();
        if (!matchCtx().active) return;

        float x = m_player1 ? m_player1->getPositionX() : 0.f;
        int percent = this->getCurrentPercentInt();
        int attempts = m_attempts;
        bool dead = f->deathPending;
        f->deathPending = false;

        matjson::Value body;
        body["x"] = x;
        body["percent"] = percent;
        body["attempts"] = attempts;
        body["dead"] = dead ? 1 : 0;
        petus::versus::post("/api/versus/match/pos", body, [](bool, matjson::Value) {});
    }

    void pollState(float) {
        if (!matchCtx().active) return;

        // Capture a weak-ish guard: this VersusPlayLayer via Ref.
        Ref<VersusPlayLayer> self = this;
        petus::versus::get("/api/versus/match/state", [self](bool ok, matjson::Value data) {
            if (!ok || !self) return;
            auto layer = self.data();
            if (!matchCtx().active) return;
            auto f = layer->m_fields.self();

            long long remaining = jll(data, "remainingMs", -1);
            f->remainingMs = remaining;
            matchCtx().remainingMs = remaining;

            std::vector<OtherPlayer> players;
            if (data.contains("players") && data["players"].isArray()) {
                for (auto const& pv : data["players"]) {
                    OtherPlayer op;
                    op.accountID = jint(pv, "accountID");
                    op.name = jstr(pv, "name");
                    op.icon = jstr(pv, "icon");
                    op.x = jdbl(pv, "x");
                    op.percent = jint(pv, "percent");
                    op.deaths = jint(pv, "deaths");
                    op.finished = jbool(pv, "finished");
                    players.push_back(op);
                }
            }
            layer->updateGhosts(players);

            // Time-limit reached and we haven't finished -> submit a DNF.
            if (remaining == 0 && !f->finishedReported) {
                layer->reportFinish();
            }
        });
    }

    void reportFinish() {
        auto f = m_fields.self();
        if (f->finishedReported) return;
        f->finishedReported = true;

        matjson::Value body;
        body["deaths"] = f->localDeaths;
        petus::versus::post("/api/versus/match/finish", body, [](bool, matjson::Value) {});

        // Stop reporting our position; results flow takes over.
        this->unschedule(schedule_selector(VersusPlayLayer::sendPos));
        this->unschedule(schedule_selector(VersusPlayLayer::pollState));

        petus::versus::showResults();
        // The match itself is over for us; the results poller owns the rest.
        petus::versus::endMatch();
    }

    void teardown() {
        auto f = m_fields.self();
        if (!f->armed) return;
        f->armed = false;
        this->unschedule(schedule_selector(VersusPlayLayer::sendPos));
        this->unschedule(schedule_selector(VersusPlayLayer::pollState));
        if (f->overlay) {
            f->overlay->removeFromParent();
            f->overlay = nullptr;
        }
        f->ghosts.clear();
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto& ctx = matchCtx();
        if (!ctx.active || !level) return true;
        int thisLevelID = level->m_levelID.value();
        if (thisLevelID != ctx.levelID) return true;

        auto f = m_fields.self();
        f->armed = true;
        f->finishedReported = false;
        f->localDeaths = 0;

        // Ghost container rides inside the object layer so it scrolls with the level.
        auto overlay = CCNode::create();
        overlay->setZOrder(1000);
        if (m_objectLayer) {
            m_objectLayer->addChild(overlay);
        } else {
            this->addChild(overlay);
        }
        f->overlay = overlay;

        this->schedule(schedule_selector(VersusPlayLayer::sendPos), 0.1f);
        this->schedule(schedule_selector(VersusPlayLayer::pollState), 0.12f);

        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        auto f = m_fields.self();
        if (!f->armed) return;
        // Ignore the "fake" death used by the anticheat spike (p1 == the safe obj).
        if (player == m_player1) {
            f->localDeaths++;
            f->deathPending = true;
        }
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        auto f = m_fields.self();
        if (!f->armed) return;
        this->reportFinish();
    }

    void onExit() {
        // Player bailed out (pause -> quit) mid-match: tear the overlay down.
        this->teardown();
        PlayLayer::onExit();
    }
};
