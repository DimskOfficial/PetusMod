#include "VersusLobbyPopup.hpp"
#include "VersusApi.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

// Adds the "Versus" entry to the main menu. Kept in its own $modify so it never
// clashes with the primary PetusMenuLayer hook in main.cpp.
class $modify(VersusMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Bottom-row style button; placed alongside the main menu buttons.
        auto* menu = this->getChildByID("bottom-menu");
        if (!menu) menu = this->getChildByID("main-menu");
        if (!menu) return true; // node-ids missing; skip rather than crash

        auto spr = ButtonSprite::create("Versus", "goldFont.fnt", "GJ_button_04.png", 1.0f);
        spr->setScale(0.8f);
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
            VersusMenuLayer::onVersus();
        });
        btn->setID("petus-versus-button");
        menu->addChild(btn);
        menu->updateLayout();

        return true;
    }

    // Static so the button callback needs no captured `this`.
    static void onVersus() {
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
