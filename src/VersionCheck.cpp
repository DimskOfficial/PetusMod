#include "VersionCheck.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>

using namespace geode::prelude;

namespace petus {

    // Read the installed build version the launcher recorded next to the game.
    static std::string installedVersion() {
        if (auto local = std::getenv("LOCALAPPDATA")) {
            auto path = std::string(local) + "\\PetusGDPS\\installed.json";
            auto res = utils::file::readString(path);
            if (res) {
                auto parsed = matjson::parse(res.unwrap());
                if (parsed) {
                    auto j = parsed.unwrap();
                    if (j.contains("version")) return j["version"].asString().unwrapOr("");
                }
            }
        }
        return "";
    }

    void checkGameVersion() {
        auto local = installedVersion();
        if (local.empty()) return; // not launcher-installed; skip

        auto url = serverBase() + "/api/game/manifest";
        async::spawn(web::WebRequest().get(url), [local](web::WebResponse response) {
            if (!response.ok()) return;
            auto parsed = response.json();
            if (!parsed) return;
            auto j = parsed.unwrap();
            auto latest = j.contains("version") ? j["version"].asString().unwrapOr("") : "";
            if (latest.empty() || latest == local) return;

            Loader::get()->queueInMainThread([latest, local]() {
                FLAlertLayer::create(
                    "Доступно обновление",
                    "<cy>Установлена версия " + local + ".</c>\n"
                    "<cg>Доступна " + latest + ".</c>\n\n"
                    "Обнови игру через <cl>PetusLauncher</c>.",
                    "OK"
                )->show();
            });
        });
    }

}
