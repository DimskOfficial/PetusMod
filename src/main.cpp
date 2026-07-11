#include "Session.hpp"
#include "DefaultLevels.hpp"
#include "VerifyScreenshot.hpp"
#include "Textboxes.hpp"
#include "ModsPopup.hpp"
#include "VersionCheck.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/modify/OptionsLayer.hpp>

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

        // Startup notices: not-registered / welcome+bonus / ban (once per open).
        static bool s_noticesShown = false;
        if (!s_noticesShown) {
            s_noticesShown = true;
            petus::showStartupNotices();
            petus::checkGameVersion();
        }

        // Petus ID only: there is no in-game register/login.
        if (Mod::get()->getSettingValue<bool>("disable-ingame-login")) {
            if (auto* btn = this->querySelector("account-button")) {
                btn->setVisible(false);
            }
        }

        // Remove Geode's own mods button from the main menu — mods are reached
        // through Options -> Mods instead. Geode adds its button AFTER
        // MenuLayer::init, so defer the removal to the next frame and search the
        // whole tree (the button lives inside a child menu).
        this->runAction(CCSequence::create(
            CCDelayTime::create(0.05f),
            CCCallFunc::create(this, callfunc_selector(PetusMenuLayer::removeGeodeButton)),
            nullptr
        ));

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

    // Recursively find and hide Geode's mods button (its ID varies by version;
    // match by id substring). Called a frame after init so the loader has added it.
    void removeGeodeButton() {
        removeGeodeButtonIn(this);
    }
    void removeGeodeButtonIn(CCNode* node) {
        if (!node) return;
        auto* children = node->getChildren();
        if (!children) return;
        for (int i = 0; i < children->count(); i++) {
            auto* child = static_cast<CCNode*>(children->objectAtIndex(i));
            std::string id = child->getID();
            if (id.find("geode") != std::string::npos &&
                (id.find("button") != std::string::npos || id.find("mods") != std::string::npos)) {
                child->setVisible(false);
                child->removeFromParent();
                return;
            }
            removeGeodeButtonIn(child);
        }
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

// ---- Options: add a "Mods" button that opens our own mods list -------------
class $modify(PetusOptionsLayer, OptionsLayer) {
    void customSetup() {
        OptionsLayer::customSetup();

        // Find the options button menu and append a "Mods" row.
        auto* menu = this->getChildByID("options-menu");
        if (!menu) {
            // Fallback: first CCMenu child.
            for (auto* c : CCArrayExt<CCNode*>(this->getChildren())) {
                if (auto* m = typeinfo_cast<CCMenu*>(c)) { menu = m; break; }
            }
        }
        if (!menu) return;

        auto spr = ButtonSprite::create("Mods", "goldFont.fnt", "GJ_button_01.png", 1.0f);
        spr->setScale(0.7f);
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
            petus::openModsPopup();
        });
        btn->setID("petus-mods-button");
        menu->addChild(btn);
        menu->updateLayout();
    }
};
