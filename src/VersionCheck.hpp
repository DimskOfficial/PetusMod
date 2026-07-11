#pragma once

// Compares the installed game build against the server manifest. If the local
// build is older, shows a textbox telling the player to update via the launcher.
namespace petus {
    void checkGameVersion();
}
