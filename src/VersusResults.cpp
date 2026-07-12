#include "VersusResults.hpp"
#include "VersusApi.hpp"
#include "VersusUi.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

namespace petus::versus {

    // Popup that renders the final rankings. Built directly from the results JSON.
    class VersusResultsPopup : public geode::Popup {
    protected:
        matjson::Value m_results;

        bool init() override {
            const float w = 380.f, h = 280.f;
            if (!geode::Popup::init(w, h)) return false;
            this->setTitle("Результаты");

            std::string levelName = jstr(m_results, "levelName", "Уровень");
            auto sub = CCLabelBMFont::create(levelName.c_str(), "goldFont.fnt");
            sub->setScale(0.5f);
            sub->setPosition({ w / 2.f, h - 42.f });
            m_mainLayer->addChild(sub);

            auto scroll = ScrollLayer::create({ w - 50.f, h - 80.f });
            scroll->setPosition({ 25.f, 15.f });

            std::vector<matjson::Value> rankings = jarr(m_results, "rankings");

            const float rowH = 44.f;
            const float width = scroll->getContentSize().width;
            const float totalH = std::max<float>(rankings.size() * rowH, scroll->getContentSize().height);
            scroll->m_contentLayer->setContentSize({ width, totalH });

            for (size_t i = 0; i < rankings.size(); i++) {
                auto& r = rankings[i];
                float rowY = totalH - (i + 1) * rowH;
                this->buildRow(scroll->m_contentLayer, r, width, rowY, rowH);
            }

            scroll->moveToTop();
            m_mainLayer->addChild(scroll);
            return true;
        }

        void buildRow(CCNode* parent, matjson::Value& r, float width, float rowY, float rowH) {
            int place = jint(r, "place", 0);
            bool finished = jbool(r, "finished");
            int percent = jint(r, "percent");
            int deaths = jint(r, "deaths");
            long long timeMs = jll(r, "timeMs", 0);
            int reward = jint(r, "reward", 0);
            std::string name = jstr(r, "name", "?");
            std::string icon = jstr(r, "icon");

            auto bg = CCLayerColor::create(
                (place % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20),
                width, rowH - 2.f
            );
            bg->setPosition({ 0.f, rowY });
            parent->addChild(bg);

            float cy = rowY + rowH / 2.f;

            // Place number, medal-tinted for the podium.
            auto placeLabel = CCLabelBMFont::create(fmt::format("{}", place).c_str(), "bigFont.fnt");
            placeLabel->setScale(0.55f);
            placeLabel->setAnchorPoint({ 0.5f, 0.5f });
            placeLabel->setPosition({ 16.f, cy });
            if (place == 1) placeLabel->setColor({ 255, 215, 0 });
            else if (place == 2) placeLabel->setColor({ 192, 192, 192 });
            else if (place == 3) placeLabel->setColor({ 205, 127, 50 });
            parent->addChild(placeLabel);

            // Cube.
            if (auto cube = makeCube(icon)) {
                cube->setScale(0.7f);
                cube->setPosition({ 42.f, cy });
                parent->addChild(cube);
            }

            // Name.
            auto nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
            nameLabel->setAnchorPoint({ 0.f, 0.5f });
            nameLabel->setScale(0.45f);
            nameLabel->setPosition({ 60.f, cy + 8.f });
            if (nameLabel->getContentSize().width * 0.45f > 120.f) {
                nameLabel->setScale(120.f / nameLabel->getContentSize().width);
            }
            parent->addChild(nameLabel);

            // Status line: finished (time) or DNF (percent).
            std::string status = finished
                ? fmt::format("прошёл  {}", formatTime(timeMs))
                : fmt::format("не прошёл ({}%)", percent);
            auto statusLabel = CCLabelBMFont::create(status.c_str(), "chatFont.fnt");
            statusLabel->setAnchorPoint({ 0.f, 0.5f });
            statusLabel->setScale(0.6f);
            statusLabel->setPosition({ 60.f, cy - 8.f });
            statusLabel->setColor(finished ? ccColor3B{ 120, 255, 120 } : ccColor3B{ 255, 150, 150 });
            parent->addChild(statusLabel);

            // Deaths (skull icon + count).
            auto skull = CCSprite::createWithSpriteFrameName("skull_small01_001.png");
            if (!skull) skull = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
            if (skull) {
                skull->setScale(0.6f);
                skull->setPosition({ width - 88.f, cy });
                parent->addChild(skull);
            }
            auto deathsLabel = CCLabelBMFont::create(fmt::format("{}", deaths).c_str(), "bigFont.fnt");
            deathsLabel->setScale(0.4f);
            deathsLabel->setAnchorPoint({ 0.f, 0.5f });
            deathsLabel->setPosition({ width - 76.f, cy });
            parent->addChild(deathsLabel);

            // Reward stars.
            if (reward != 0) {
                auto star = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
                if (star) {
                    star->setScale(0.6f);
                    star->setPosition({ width - 40.f, cy });
                    parent->addChild(star);
                }
                std::string rewardStr = (reward > 0)
                    ? fmt::format("+{}", reward)
                    : fmt::format("{}", reward);
                auto rewardLabel = CCLabelBMFont::create(rewardStr.c_str(), "bigFont.fnt");
                rewardLabel->setScale(0.42f);
                rewardLabel->setAnchorPoint({ 1.f, 0.5f });
                rewardLabel->setPosition({ width - 52.f, cy });
                rewardLabel->setColor(reward > 0 ? ccColor3B{ 120, 255, 120 } : ccColor3B{ 255, 120, 120 });
                parent->addChild(rewardLabel);
            }
        }

        void onClose(CCObject* sender) override {
            geode::Popup::onClose(sender);
            // Return to the main menu after the race.
            auto scene = MenuLayer::scene(false);
            CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
        }

    public:
        static VersusResultsPopup* create(matjson::Value results) {
            auto ret = new VersusResultsPopup();
            ret->m_results = std::move(results);
            if (ret->init()) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    // Poll GET /api/versus/match/results until {ready:true}, then show the popup.
    // A tiny CCNode driven by CCScheduler keeps the retry loop alive without a UI.
    class ResultsPoller : public CCNode {
    public:
        int m_tries = 0;

        static ResultsPoller* start() {
            auto p = new ResultsPoller();
            p->init();
            p->autorelease();
            CCScene::get()->addChild(p);
            p->schedule(schedule_selector(ResultsPoller::tick), 1.0f);
            p->tick(0);
            return p;
        }

        void tick(float) {
            m_tries++;
            Ref<ResultsPoller> self = this;
            petus::versus::get("/api/versus/match/results", [self](bool ok, matjson::Value data) {
                if (!self) return;
                if (ok && jbool(data, "ready")) {
                    self->unschedule(schedule_selector(ResultsPoller::tick));
                    if (auto p = VersusResultsPopup::create(data)) p->show();
                    self->removeFromParent();
                    return;
                }
                // Give up after ~30s and just bail to the menu.
                if (self->m_tries > 30) {
                    self->unschedule(schedule_selector(ResultsPoller::tick));
                    auto scene = MenuLayer::scene(false);
                    CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
                    self->removeFromParent();
                }
            });
        }
    };

    void showResults() {
        ResultsPoller::start();
    }

}
