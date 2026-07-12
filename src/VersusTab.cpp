#include "VersusLobbyPopup.hpp"
#include "VersusApi.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

namespace {
    // True if `node` is a CCSprite currently displaying the given sprite frame.
    bool spriteShowsFrame(CCNode* node, CCSpriteFrame* frame) {
        auto* spr = typeinfo_cast<CCSprite*>(node);
        return spr && frame && spr->isFrameDisplayed(frame);
    }

    // Depth-first search for the native Versus button: a CCMenuItemSpriteExtra
    // whose normal image is (or contains) a sprite showing GJ_versusBtn_001.png.
    CCMenuItemSpriteExtra* findVersusButton(CCNode* node, CCSpriteFrame* frame) {
        if (!node) return nullptr;

        if (auto* item = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
            auto* image = item->getNormalImage();
            if (spriteShowsFrame(image, frame)) return item;
            // Some buttons wrap the frame sprite inside a ButtonSprite/child.
            if (image) {
                for (auto* child : CCArrayExt<CCNode*>(image->getChildren())) {
                    if (spriteShowsFrame(child, frame)) return item;
                }
            }
        }

        for (auto* child : CCArrayExt<CCNode*>(node->getChildren())) {
            if (auto* found = findVersusButton(child, frame)) return found;
        }
        return nullptr;
    }
}

// Repoints the game's native Versus button (in the "Create" tab) at our popup.
// Vanilla, that button shows a "Versus mode has been delayed..." dialog.
class $modify(PetusCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        auto* frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName("GJ_versusBtn_001.png");
        if (auto* item = findVersusButton(this, frame)) {
            item->setTarget(this, menu_selector(PetusCreatorLayer::onPetusVersus));
        }
        // If the button isn't found (layout changed), leave the layer untouched.

        return true;
    }

    void onPetusVersus(CCObject*) {
        if (!petus::versus::hasSession()) {
            FLAlertLayer::create(
                "Versus",
                "Войдите через PetusLauncher, чтобы играть в Versus.",
                "OK")->show();
            return;
        }

        petus::versus::get("/api/versus/eligible", [](bool ok, matjson::Value data) {
            bool eligible = ok && data.contains("eligible") && data["eligible"].asBool().unwrapOr(false);
            if (!eligible) {
                FLAlertLayer::create(
                    "Versus",
                    "Versus доступен только для аккаунтов с прогрессом.",
                    "OK")->show();
                return;
            }
            petus::versus::openVersusMenu();
        });
    }
};
