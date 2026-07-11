#include "DefaultLevels.hpp"
#include "Session.hpp"

#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

namespace petus {

    void fetchDefaultLevels(std::function<void(std::vector<DefaultLevel>)> cb) {
        auto url = serverBase() + "/api/defaultlevels";

        auto req = web::WebRequest();
        req.header("Accept", "application/json");

        async::spawn(req.get(url), [cb = std::move(cb)](web::WebResponse response) {
            std::vector<DefaultLevel> out;
            if (response.ok()) {
                auto json = response.json().unwrapOr(matjson::Value::array());
                // Core returns { ok: true, data: [ {slot, levelID, name, levelString} ] }
                auto arr = json.contains("data") ? json["data"] : json;
                if (arr.isArray()) {
                    for (auto& item : arr) {
                        DefaultLevel d;
                        d.slot = item.contains("slot") ? item["slot"].asInt().unwrapOr(0) : 0;
                        d.levelID = item.contains("levelID") ? item["levelID"].asInt().unwrapOr(0) : 0;
                        d.name = item.contains("name") ? item["name"].asString().unwrapOr("") : "";
                        d.levelString = item.contains("levelString") ? item["levelString"].asString().unwrapOr("") : "";
                        out.push_back(std::move(d));
                    }
                }
                log::info("Fetched {} default Play levels", out.size());
            } else {
                log::warn("Failed to fetch default levels ({})", response.code());
            }
            cb(std::move(out));
        });
    }

}
