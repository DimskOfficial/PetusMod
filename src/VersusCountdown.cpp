#include "VersusCountdown.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace petus::versus {

    bool VersusCountdown::init(long long countdownMs, std::function<void()> onDone) {
        if (!CCLayerColor::initWithColor({ 0, 0, 0, 165 })) return false;
        m_onDone = std::move(onDone);
        m_remainingMs = countdownMs > 0 ? countdownMs : 5000;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        this->setContentSize(winSize);

        auto title = CCLabelBMFont::create("Versus", "goldFont.fnt");
        title->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 70.f });
        title->setScale(0.9f);
        this->addChild(title);

        m_label = CCLabelBMFont::create("5", "bigFont.fnt");
        m_label->setPosition({ winSize.width / 2.f, winSize.height / 2.f });
        m_label->setScale(2.2f);
        this->addChild(m_label);

        this->setZOrder(10000);
        this->schedule(schedule_selector(VersusCountdown::tick), 0.05f);
        this->tick(0.f);
        return true;
    }

    void VersusCountdown::tick(float dt) {
        m_remainingMs -= static_cast<long long>(dt * 1000.f);
        if (m_remainingMs <= 0) {
            this->unschedule(schedule_selector(VersusCountdown::tick));
            auto done = m_onDone;
            this->removeFromParent();
            if (done) done();
            return;
        }

        int secs = static_cast<int>((m_remainingMs + 999) / 1000); // ceil
        if (secs < 1) secs = 1;
        if (secs != m_lastShown) {
            m_lastShown = secs;
            m_label->setString(fmt::format("{}", secs).c_str());
            // Little pop on each tick.
            m_label->setScale(2.6f);
            m_label->runAction(CCEaseOut::create(CCScaleTo::create(0.3f, 2.2f), 2.f));
        }
    }

    VersusCountdown* VersusCountdown::create(long long countdownMs, std::function<void()> onDone) {
        auto ret = new VersusCountdown();
        if (ret->init(countdownMs, std::move(onDone))) {
            ret->autorelease();
            if (auto scene = CCScene::get()) {
                scene->addChild(ret, 10000);
            }
            return ret;
        }
        delete ret;
        return nullptr;
    }

}
