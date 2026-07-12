#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>

// Global Versus match context + the PlayLayer hook that renders the live race
// (other players' cubes + nicks) and reports our own progress to the core.
namespace petus::versus {

    struct OtherPlayer {
        int accountID = 0;
        std::string name;
        std::string icon;     // "cube:col1:col2:col3:glow"
        double x = 0;
        int percent = 0;
        int deaths = 0;
        bool finished = false;
    };

    // Set when a Versus match is starting so the PlayLayer hook activates for
    // exactly the chosen level. Cleared when the results screen is shown.
    struct MatchContext {
        bool active = false;
        std::string lobbyId;
        int levelID = 0;
        int myAccountID = 0;
        long long remainingMs = 0;
    };

    MatchContext& matchCtx();

    // Begin a match: download the level and enter PlayLayer with the hook armed.
    void enterMatch(const std::string& lobbyId, int levelID, int myAccountID);

    // Called by the results flow to tear the match down.
    void endMatch();

}
