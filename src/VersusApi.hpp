#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

// Thin client for the PetusCore Versus REST API. Every call carries the
// launcher bearer token; responses are matjson. Kept callback-based to fit the
// game's main-thread UI model (results are queued back on the main thread).
namespace petus::versus {

    using JsonCb = std::function<void(bool ok, matjson::Value data)>;

    // Base for the Versus REST endpoints (site host proxies /api/* to the core).
    std::string apiBase();

    // GET/POST helpers that attach Authorization: Bearer <token>. `cb` runs on
    // the main thread. POST body is a matjson object serialized to JSON.
    void get(const std::string& path, JsonCb cb);
    void post(const std::string& path, matjson::Value body, JsonCb cb);

    // True if the launcher session carries a token (needed for all Versus calls).
    bool hasSession();

}
