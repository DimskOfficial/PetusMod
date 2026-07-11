#include "Textboxes.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

namespace petus {

    // Show a popup on the main thread (safe from web callbacks).
    static void popup(std::string title, std::string body) {
        Loader::get()->queueInMainThread([title = std::move(title), body = std::move(body)]() {
            FLAlertLayer::create(title.c_str(), body, "OK")->show();
        });
    }

    // Format the remaining ban time from an absolute unix expiry (0 = permanent).
    static std::string banDuration(int64_t until) {
        if (until <= 0) return "permanent";
        auto now = static_cast<int64_t>(time(nullptr));
        auto left = until - now;
        if (left <= 0) return "expired";
        auto days = left / 86400;
        auto hours = (left % 86400) / 3600;
        if (days > 0) return std::to_string(days) + "d " + std::to_string(hours) + "h";
        auto mins = (left % 3600) / 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }

    void showStartupNotices() {
        const auto& s = session();

        // Not launched through PetusLauncher (or session missing) — cannot play.
        if (!s.valid) {
            popup(
                "You are not registered.",
                "<cl>Log in using our launcher.</c>\n<cl>And restart the game.</c>"
            );
            return;
        }

        // Ask the core what to greet the player with.
        auto url = serverBase() + "/api/game/state";
        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + s.token);

        async::spawn(req.get(url), [](web::WebResponse response) {
            if (!response.ok()) {
                log::warn("game/state -> {}", response.code());
                return;
            }
            auto parsed = response.json();
            if (!parsed) return;
            auto json = parsed.unwrap();

            bool banned = json.contains("banned") && json["banned"].asBool().unwrapOr(false);
            bool newAccount = json.contains("newAccount") && json["newAccount"].asBool().unwrapOr(false);

            if (banned) {
                auto reason = json.contains("banReason") ? json["banReason"].asString().unwrapOr("") : "";
                int64_t until = json.contains("banUntil") ? json["banUntil"].asInt().unwrapOr(0) : 0;
                popup(
                    "You have been blocked.",
                    "Your ban lasts: " + banDuration(until) +
                        "\nReason for blocking: " + (reason.empty() ? "not specified" : reason)
                );
                return;
            }

            if (newAccount) {
                popup(
                    "Welcome!",
                    "You have just created an account...\n"
                    "<cs>Congratulations!</c>\n\n"
                    "<cy>You received a bonus: 50 diamonds!</c>"
                );
            }
        });
    }

}
