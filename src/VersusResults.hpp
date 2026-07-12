#pragma once

// The Versus results screen: polls GET /api/versus/match/results until ready,
// then shows a native-styled ranking popup. Reached at the end of a match.
namespace petus::versus {
    // Poll results and show the popup when ready. Safe to call from PlayLayer.
    void showResults();
}
