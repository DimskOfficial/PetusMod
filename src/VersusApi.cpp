#include "VersusApi.hpp"
#include "Session.hpp"

#include <Geode/utils/async.hpp>

using namespace geode::prelude;

namespace petus::versus {

    std::string apiBase() {
        // The site host proxies /api/* to the core; serverBase() defaults to it.
        return serverBase();
    }

    bool hasSession() {
        return session().valid && !session().token.empty();
    }

    void get(const std::string& path, JsonCb cb) {
        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + session().token);
        req.header("User-Agent", "PetusMod/1.1");
        auto url = apiBase() + path;
        async::spawn(req.get(url), [cb](web::WebResponse res) {
            bool ok = res.ok();
            matjson::Value data;
            auto parsed = res.json();
            if (parsed) data = parsed.unwrap();
            Loader::get()->queueInMainThread([cb, ok, data]() { cb(ok, data); });
        });
    }

    void post(const std::string& path, matjson::Value body, JsonCb cb) {
        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + session().token);
        req.header("User-Agent", "PetusMod/1.1");
        req.header("Content-Type", "application/json");
        req.bodyString(body.dump());
        auto url = apiBase() + path;
        async::spawn(req.post(url), [cb](web::WebResponse res) {
            bool ok = res.ok();
            matjson::Value data;
            auto parsed = res.json();
            if (parsed) data = parsed.unwrap();
            Loader::get()->queueInMainThread([cb, ok, data]() { cb(ok, data); });
        });
    }

}
