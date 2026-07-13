#include "VersusLobbyPopup.hpp"
#include "VersusApi.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

namespace {
    // True if `node` (or any descendant) is a CCSprite currently displaying the
    // given sprite frame. RobTop sometimes wraps the frame inside a ButtonSprite,
    // so we search the whole subtree, not just the immediate node.
    bool subtreeShowsFrame(CCNode* node, CCSpriteFrame* frame) {
        if (!node || !frame) return false;
        if (auto* spr = typeinfo_cast<CCSprite*>(node)) {
            if (spr->isFrameDisplayed(frame)) return true;
        }
        for (auto* child : CCArrayExt<CCNode*>(node->getChildren())) {
            if (subtreeShowsFrame(child, frame)) return true;
        }
        return false;
    }

    // Depth-first search for the native Versus button: any CCMenuItemSpriteExtra
    // whose normal image subtree shows GJ_versusBtn_001.png.
    CCMenuItemSpriteExtra* findVersusButton(CCNode* node, CCSpriteFrame* frame) {
        if (!node) return nullptr;
        if (auto* item = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
            if (subtreeShowsFrame(item->getNormalImage(), frame)) return item;
        }
        for (auto* child : CCArrayExt<CCNode*>(node->getChildren())) {
            if (auto* found = findVersusButton(child, frame)) return found;
        }
        return nullptr;
    }

    // The greyed-out look RobTop gives the delayed button: restore full colour,
    // opacity and enabled state so it reads as a live button.
    void unlockButton(CCMenuItemSpriteExtra* item) {
        item->setEnabled(true);
        auto restore = [](CCNode* n) {
            if (auto* rgba = typeinfo_cast<CCRGBAProtocol*>(n)) {
                rgba->setColor(ccColor3B{ 255, 255, 255 });
                rgba->setOpacity(255);
            }
        };
        restore(item);
        if (auto* img = item->getNormalImage()) {
            restore(img);
            for (auto* child : CCArrayExt<CCNode*>(img->getChildren())) restore(child);
        }
    }
}

// Repoints the game's native Versus button (in the "Create" tab) at our popup.
// Vanilla, that button is greyed and shows a "Versus mode has been delayed..."
// dialog. We find it, un-grey + enable it, and swap its callback.
class $modify(PetusCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
        auto* frame = cache->spriteFrameByName("GJ_versusBtn_001.png");
        auto* item = findVersusButton(this, frame);
        if (item) {
            unlockButton(item);
            item->setTarget(this, menu_selector(PetusCreatorLayer::onPetusVersus));
            log::info("Petus: Versus button hooked in CreatorLayer.");
        } else {
            log::warn("Petus: Versus button (GJ_versusBtn_001.png) not found in CreatorLayer.");
        }
        return true;
    }

    void onPetusVersus(CCObject*) {
        if (!petus::versus::hasSession()) {
            FLAlertLayer::create(
                "Versus",
                "Войдите через <cy>PetusLauncher</c>, чтобы играть в Versus.",
                "OK")->show();
            return;
        }

        petus::versus::get("/api/versus/eligible", [](bool ok, matjson::Value data) {
            bool eligible = ok && data.contains("eligible") && data["eligible"].asBool().unwrapOr(false);
            if (!eligible) {
                FLAlertLayer::create(
                    "Versus",
                    "Versus доступен только для аккаунтов <cy>с прогрессом</c> "
                    "(звёзды, алмазы, монеты или пройденные уровни).",
                    "OK")->show();
                return;
            }
            petus::versus::openVersusMenu();
        });
    }
};
