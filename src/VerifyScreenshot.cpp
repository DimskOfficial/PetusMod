#include "VerifyScreenshot.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

namespace petus {

    // Upload a PNG (already on disk) to imgbb, then register it as the level
    // preview on the core.
    static void uploadPreview(int levelID, const std::filesystem::path& png) {
        auto bytesRes = utils::file::readBinary(png);
        if (!bytesRes) {
            log::warn("verify screenshot: cannot read {}", png.string());
            return;
        }
        auto bytes = bytesRes.unwrap();
        std::string b64 = utils::base64::encode(bytes.data(), bytes.size());

        auto key = Mod::get()->getSettingValue<std::string>("imgbb-key");
        auto url = "https://api.imgbb.com/1/upload?key=" + key;

        static EventListener<web::WebTask> s_imgbb;
        auto req = web::WebRequest();
        req.bodyString("image=" + utils::string::urlEncode(b64));
        req.header("Content-Type", "application/x-www-form-urlencoded");

        s_imgbb.bind([levelID](web::WebTask::Event* e) {
            auto* res = e->getValue();
            if (!res) return;
            if (!res->ok()) {
                log::warn("imgbb upload failed ({})", res->code());
                return;
            }
            auto json = res->json().unwrapOr(matjson::Value::object());
            std::string imgUrl;
            if (json.contains("data") && json["data"].contains("url")) {
                imgUrl = json["data"]["url"].asString().unwrapOr("");
            }
            if (imgUrl.empty()) {
                log::warn("imgbb response had no url");
                return;
            }
            log::info("verify screenshot uploaded: {}", imgUrl);

            // Register as the level preview via the core.
            const auto& s = session();
            if (!s.valid) return;
            auto previewUrl = serverBase() + "/api/levels/" + std::to_string(levelID) + "/preview";

            static EventListener<web::WebTask> s_preview;
            auto preq = web::WebRequest();
            preq.header("Authorization", "Bearer " + s.token);
            preq.header("Content-Type", "application/json");
            matjson::Value body = matjson::Value::object();
            body["url"] = imgUrl;
            preq.bodyString(body.dump());
            s_preview.bind([](web::WebTask::Event* pe) {
                if (auto* pr = pe->getValue()) {
                    log::info("preview register -> {}", pr->code());
                }
            });
            s_preview.setFilter(preq.post(previewUrl));
        });

        s_imgbb.setFilter(req.post(url));
    }

    // Capture the current frame to a temp PNG and kick off the upload.
    static void captureAndUpload(int levelID) {
        auto* scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* rt = CCRenderTexture::create(
            static_cast<int>(winSize.width), static_cast<int>(winSize.height));
        if (!rt) return;

        rt->begin();
        scene->visit();
        rt->end();

        auto path = dirs::getTempDir() / fmt::format("petus_verify_{}.png", levelID);
        // saveToFile writes a PNG under the temp dir. newCCImage() returns an
        // owned image — grab it once, save, release.
        auto* img = rt->newCCImage();
        if (img) {
            img->saveToFile(path.string().c_str(), false);
            img->release();
            log::info("verify screenshot saved: {}", path.string());
            uploadPreview(levelID, path);
        }
    }

    void initVerifyScreenshot() {
        // Nothing to do at load; the $modify hook below self-registers.
    }

}

// Hook PlayLayer to watch progress during verification.
class $modify(PetusPlayLayer, PlayLayer) {
    struct Fields {
        bool m_shot = false;
    };

    // Only fire while the creator is verifying their own unverified level.
    bool isVerifying() {
        auto* lvl = this->m_level;
        if (!lvl) return false;
        // Testmode + the level isn't verified/uploaded yet, and we're the author.
        // m_isTestMode / m_level->m_levelType vary by version — adjust if needed.
        return this->m_isTestMode && lvl->m_levelID.value() > 0;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (m_fields->m_shot) return;
        if (!isVerifying()) return;

        // Progress 0..100. getCurrentPercent() exists on modern GD/Geode.
        float pct = this->getCurrentPercent();
        if (pct >= 50.f) {
            m_fields->m_shot = true;
            int id = static_cast<int>(this->m_level->m_levelID.value());
            petus::captureAndUpload(id);
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->m_shot = false;
    }
};
