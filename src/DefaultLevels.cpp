#include "DefaultLevels.hpp"
#include "Session.hpp"

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace petus {

    void fetchDefaultLevels(std::function<void(std::vector<DefaultLevel>)> cb) {
        auto url = serverBase() + "/api/defaultlevels";

        static EventListener<web::WebTask> s_listener;
        auto req = web::WebRequest();
        req.header("Accept", "application/json");

        s_listener.bind([cb = std::move(cb)](web::WebTask::Event* e) {
            auto* res = e->getValue();
            if (!res) return;

            std::vector<DefaultLevel> out;
            if (res->ok()) {
                auto json = res->json().unwrapOr(matjson::Value::array());
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
                log::warn("Failed to fetch default levels ({})", res->code());
            }
            cb(std::move(out));
        });

        s_listener.setFilter(req.get(url));
    }

}
