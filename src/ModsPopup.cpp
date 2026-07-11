#include "ModsPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace petus {

    // Our own mods list (not Geode's index tab). Each row: mod name + a
    // "Settings" button that opens that mod's settings popup.
    class ModsPopup : public geode::Popup {
    protected:
        bool init() {
            const float w = 340.f, h = 260.f;
            if (!Popup::init(w, h)) return false;
            this->setTitle("Mods");

            auto scroll = ScrollLayer::create({ w - 50.f, h - 70.f });
            scroll->setPosition({ 25.f, 20.f });

            auto mods = Loader::get()->getAllMods();
            std::sort(mods.begin(), mods.end(), [](Mod* a, Mod* b) {
                if (a->getID() == "petus.gdps") return true;
                if (b->getID() == "petus.gdps") return false;
                return a->getName() < b->getName();
            });

            const float rowH = 34.f;
            const float width = scroll->getContentSize().width;
            const float totalH = std::max<float>(mods.size() * rowH, scroll->getContentSize().height);
            scroll->m_contentLayer->setContentSize({ width, totalH });

            for (size_t i = 0; i < mods.size(); i++) {
                auto* mod = mods[i];
                float rowY = totalH - (i + 1) * rowH;

                auto bg = CCLayerColor::create(
                    (i % 2 == 0) ? ccc4(0, 0, 0, 40) : ccc4(0, 0, 0, 20),
                    width, rowH - 2.f
                );
                bg->setPosition({ 0.f, rowY });
                scroll->m_contentLayer->addChild(bg);

                auto label = CCLabelBMFont::create(mod->getName().c_str(), "bigFont.fnt");
                label->setAnchorPoint({ 0.f, 0.5f });
                label->setScale(0.45f);
                label->setPosition({ 8.f, rowY + rowH / 2.f });
                if (!mod->isOrWillBeEnabled()) label->setOpacity(120);
                scroll->m_contentLayer->addChild(label);

                if (mod->hasSettings()) {
                    auto btnSpr = ButtonSprite::create("Settings", "goldFont.fnt", "GJ_button_05.png", 0.7f);
                    btnSpr->setScale(0.55f);
                    auto btn = CCMenuItemExt::createSpriteExtra(btnSpr, [mod](CCMenuItemSpriteExtra*) {
                        openSettingsPopup(mod);
                    });
                    auto menu = CCMenu::create();
                    menu->addChild(btn);
                    menu->setPosition({ width - 45.f, rowY + rowH / 2.f });
                    scroll->m_contentLayer->addChild(menu);
                }
            }

            scroll->moveToTop();
            this->m_mainLayer->addChild(scroll);
            return true;
        }

    public:
        static ModsPopup* create() {
            auto ret = new ModsPopup();
            if (ret->init()) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
    };

    void openModsPopup() {
        if (auto p = ModsPopup::create()) p->show();
    }

}
