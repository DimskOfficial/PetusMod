#pragma once

#include <string>

// Reads the session handed over by PetusLauncher and applies it to the game so
// the player is authenticated without ever typing a password. The launcher
// writes %LOCALAPPDATA%\PetusGDPS\session.json before spawning the game.
namespace petus {

    struct Session {
        std::string token;   // core bearer token (site API)
        std::string name;    // display username
        int account = 0;     // GDPS account id
        bool valid = false;
    };

    // Absolute path to the session file the launcher writes.
    std::string sessionFilePath();

    // Load + cache the session (idempotent). Returns the cached copy.
    const Session& session();

    // Apply the session to GJAccountManager so the game acts logged-in.
    void applySession();

    // Base server URL from settings (e.g. https://gdps.petus.ru).
    std::string serverBase();

}
