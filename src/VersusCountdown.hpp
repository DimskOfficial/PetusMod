#pragma once

#include <Geode/Geode.hpp>
#include <functional>

// A 5-second countdown overlay shown before a Versus match starts. Syncs to the
// lobby's countdownMs and fires a completion callback when it reaches zero.
namespace petus::versus {

    class VersusCountdown : public cocos2d::CCLayerColor {
    protected:
        std::function<void()> m_onDone;
        int m_lastShown = -1;
        long long m_remainingMs = 5000;
        cocos2d::CCLabelBMFont* m_label = nullptr;

        bool init(long long countdownMs, std::function<void()> onDone);
        void tick(float dt);

    public:
        // countdownMs: ms left until start (from the lobby DTO). onDone runs once
        // the countdown hits zero. Adds itself to the running scene.
        static VersusCountdown* create(long long countdownMs, std::function<void()> onDone);
    };

}
