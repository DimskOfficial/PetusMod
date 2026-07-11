#pragma once

// Hooks PlayLayer so that when a creator is verifying (test-mode / practice on
// their own unverified level) and reaches 50% progress, the mod grabs a
// screenshot, uploads it to imgbb, and registers the resulting URL as the
// level's preview via the core (/api/levels/{id}/preview).
namespace petus {
    // Installed via $modify in the .cpp; this header is just documentation +
    // a hook so the translation unit is linked.
    void initVerifyScreenshot();
}
