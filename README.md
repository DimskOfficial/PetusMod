# Petus GDPS (Geode mod)

Client-side integration between **Geometry Dash** and the **PetusGDPS**
ecosystem. Built on the [Geode](https://geode-sdk.org) mod loader.

## What it does

| Feature | Where | Notes |
| --- | --- | --- |
| **Petus ID only login** | `Session.cpp`, `main.cpp` | Reads `session.json` written by PetusLauncher, applies the identity to `GJAccountManager`, and hides the in-game account/login UI. No passwords in-game. |
| **Play deep-links** | `main.cpp` | Handles `petusgdps://play?level=<id>` forwarded by the launcher (site "Play" button) and jumps into the level. |
| **Verification screenshot** | `VerifyScreenshot.cpp` | When the creator verifies their level and hits **50%**, captures the frame, uploads it to **imgbb**, and registers the URL as the level preview via `POST /api/levels/{id}/preview`. |
| **Custom default levels** | `DefaultLevels.cpp`, `main.cpp` | Fetches the admin-editable Play list from `GET /api/defaultlevels` and overrides Stereo Madness / Back on Track / … |

## Building

Requires the Geode SDK and CLI. See the
[Geode getting-started guide](https://docs.geode-sdk.org/getting-started/).

```bash
# GEODE_SDK env var must point at your SDK checkout
geode build
# or manually:
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

The resulting `petus.gdps.geode` goes into your GD `geode/mods` folder (or ships
inside the game archive the launcher installs).

## Version notes / TODO

This is a working skeleton. A few hooks touch GD-version-specific members
(marked with `TODO(2.2081)` / `NOTE` in the source) that should be verified
against the exact target build:

- `Session.cpp` — `GJAccountManager` member names.
- `VerifyScreenshot.cpp` — `PlayLayer::isTestMode` / `getCurrentPercent`.
- `main.cpp` `LevelSelectLayer` — swapping the on-screen main levels
  (BoomScrollLayer / `m_levelPages`).

Settings (server URL, imgbb key, disable-login) are configurable in the Geode
mod settings UI; defaults point at the live Petus servers.
