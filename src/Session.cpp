#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

namespace petus {

    static Session g_session;
    static bool g_loaded = false;

    std::string sessionFilePath() {
        // %LOCALAPPDATA%\PetusGDPS\session.json (matches the launcher).
        if (auto local = std::getenv("LOCALAPPDATA")) {
            return std::string(local) + "\\PetusGDPS\\session.json";
        }
        // Fallback next to the save dir.
        return (dirs::getSaveDir() / "petus_session.json").string();
    }

    const Session& session() {
        if (g_loaded) return g_session;
        g_loaded = true;

        auto path = sessionFilePath();
        auto res = utils::file::readString(path);
        if (!res) {
            log::warn("No Petus session file at {}", path);
            return g_session;
        }

        auto parsed = matjson::parse(res.unwrap());
        if (!parsed) {
            log::warn("Petus session file is not valid JSON");
            return g_session;
        }
        auto json = parsed.unwrap();
        g_session.token = json.contains("token") ? json["token"].asString().unwrapOr("") : "";
        g_session.name = json.contains("name") ? json["name"].asString().unwrapOr("Player") : "Player";
        g_session.account = json.contains("account")
            ? static_cast<int>(std::atoi(json["account"].asString().unwrapOr("0").c_str()))
            : 0;
        g_session.valid = !g_session.token.empty();

        log::info("Loaded Petus session for '{}' (account {})", g_session.name, g_session.account);
        return g_session;
    }

    std::string serverBase() {
        auto base = Mod::get()->getSettingValue<std::string>("server-base");
        while (!base.empty() && base.back() == '/') base.pop_back();
        return base;
    }

    void applySession() {
        const auto& s = session();
        if (!s.valid) {
            log::warn("Petus session invalid — launch the game via PetusLauncher.");
            return;
        }

        // Reflect the identity into GJAccountManager so the game shows the user
        // as logged in. The actual GDPS auth is carried by the launcher token;
        // the mod attaches it to site API calls (comments/likes/preview).
        //
        // NOTE: exact GJAccountManager members can shift between GD versions —
        // verify against your target (2.2081). These are the common ones.
        auto* am = GJAccountManager::sharedState();
        if (am) {
            am->m_accountID = s.account;
            am->m_username = s.name;
            // A real token/GJP is not needed for password login because the
            // launcher already authenticated us; the server trusts the token.
        }
    }

}
