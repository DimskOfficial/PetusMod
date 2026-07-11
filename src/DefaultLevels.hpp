#pragma once

#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <vector>

// Fetches the admin-editable "default" levels (the ones shown on the main Play
// screen — Stereo Madness, Back on Track, ...) from the core and lets the game
// override its hard-coded list. The admin panel edits GET/POST /api/defaultlevels.
namespace petus {

    struct DefaultLevel {
        int slot = 0;        // position in the Play list (0-based)
        int levelID = 0;     // catalog level to show there
        std::string name;    // display name
        std::string levelString; // raw level data (for offline main levels)
        bool enabled = true;
    };

    // Async fetch of GET /api/defaultlevels; invokes `cb` on the main thread.
    void fetchDefaultLevels(std::function<void(std::vector<DefaultLevel>)> cb);

}
