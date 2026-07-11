#include "Session.hpp"
#include "DefaultLevels.hpp"
#include "VerifyScreenshot.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>

using namespace geode::prelude;

namespace petus {
    // Cache of the admin-editable Play levels, refreshed on the main menu.
    static std::vector<DefaultLevel> g_defaults;

    // A pending level to jump into, taken from the launcher session (which
    // receives the petusgdps://play?level=ID deep link and writes it into
    // session.json as "play").
    static int g_pendingPlayLevel = 0;
}

using namespace petus;

// ---- Mod entry -------------------------------------------------------------
$on_mod(Loaded) {
    petus::initVerifyScreenshot();
    // Load the launcher-provided session immediately, incl. any deep-link level.
    const auto& s = petus::session();
    petus::g_pendingPlayLevel = s.playLevel;
    if (petus::g_pendingPlayLevel > 0) {
        log::info("Deep-link: play level {}", petus::g_pendingPlayLevel);
    }
    log::info("Petus GDPS mod loaded. Server: {}", petus::serverBase());
}

// ---- Menu: apply session, hide in-game login, prefetch defaults -----------
class $modify(PetusMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Make the game act as the launcher-authenticated user.
        petus::applySession();

        // Petus ID only: there is no in-game register/login.
        if (Mod::get()->getSettingValue<bool>("disable-ingame-login")) {
            if (auto* btn = this->querySelector("account-button")) {
                btn->setVisible(false);
            }
        }

        // Refresh the editable Play levels in the background.
        petus::fetchDefaultLevels([](std::vector<DefaultLevel> levels) {
            petus::g_defaults = std::move(levels);
        });

        // If we were opened via a Play deep link, jump straight in.
        if (petus::g_pendingPlayLevel > 0) {
            int id = petus::g_pendingPlayLevel;
            petus::g_pendingPlayLevel = 0;
            Loader::get()->queueInMainThread([id]() {
                auto* glm = GameLevelManager::sharedState();
                glm->downloadLevel(id, false, 0);
                log::info("Requested Play deep-link level {}", id);
                // The full flow (build PlayLayer once the level downloads) is
                // wired through GameLevelManager delegates — hook there for a
                // seamless auto-enter. Downloading here primes the cache.
            });
        }

        return true;
    }
};

// ---- Override the built-in "Play" levels ----------------------------------
// The default main levels (Stereo Madness, ...) are replaced by whatever the
// admin configured through /api/defaultlevels.
class $modify(PetusLevelSelectLayer, LevelSelectLayer) {
    bool init(int page) {
        if (!LevelSelectLayer::init(page)) return false;

        for (auto& d : petus::g_defaults) {
            if (!d.enabled) continue;
            // TODO(2.2081): map d.slot -> the on-screen main level and swap its
            // name/levelString/levelID. LevelSelectLayer keeps the main levels
            // in m_levelPages / BoomScrollLayer; the exact member depends on the
            // GD version. `d` carries everything needed to build a GJGameLevel.
            log::debug("default override: slot {} -> level {} ({})", d.slot, d.levelID, d.name);
        }
        return true;
    }
};
