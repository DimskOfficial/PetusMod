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

On Windows the mod **must** be built with **clang-cl** (plain MSVC crashes on
Geode's headers). The repo includes `build-clang.bat`, which sets up the MSVC
environment, forces clang-cl + Ninja, and points CMake at a local bindings
clone so a flaky network doesn't abort the build:

```bat
:: one-time: clone bindings so CMake never re-fetches them
git clone --depth 1 https://github.com/geode-sdk/bindings.git C:\Users\<you>\geode-bindings

:: then just:
build-clang.bat
```

It produces `build/petus.gdps.geode` and installs it into your GD profile.

Toolchain used (all installable via winget): `GeodeSDK.GeodeCLI`, `LLVM.LLVM`,
plus VS 2022 Build Tools (MSVC + bundled CMake/Ninja). Set `GEODE_SDK` to your
SDK path (`geode sdk install` does this automatically).

Plain `geode build` also works **only if** your default CMake generator uses
clang-cl; otherwise use `build-clang.bat`.

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
