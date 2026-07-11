#pragma once

// Shows the PetusGDPS in-game notices (as textbox-style popups):
//   * not logged in (opened without the launcher) -> "log in via launcher"
//   * banned                                       -> reason + remaining time
//   * first login after registration              -> welcome + bonus
// The state comes from the core (GET /api/game/state) using the launcher token.
namespace petus {
    // Call once from MenuLayer::init.
    void showStartupNotices();
}
