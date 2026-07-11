#include "VerifyScreenshot.hpp"
#include "Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

namespace petus {

    // Register an already-uploaded imgbb URL as the level preview on the core.
    static void registerPreview(int levelID, std::string imgUrl) {
        const auto& s = session();
        if (!s.valid) return;

        auto previewUrl = serverBase() + "/api/levels/" + std::to_string(levelID) + "/preview";
        matjson::Value body = matjson::Value::object();
        body["url"] = imgUrl;

        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + s.token);
        req.header("Content-Type", "application/json");
        req.bodyString(body.dump());

        async::spawn(req.post(previewUrl), [levelID](web::WebResponse response) {
            log::info("preview register (level {}) -> {}", levelID, response.code());
        });
    }

    // Upload the PNG at `png` to imgbb, then register the returned URL.
    static void uploadPreview(int levelID, std::filesystem::path png) {
        auto key = Mod::get()->getSettingValue<std::string>("imgbb-key");
        auto url = "https://api.imgbb.com/1/upload?key=" + key;

        web::MultipartForm form;
        auto added = form.file("image", png, "image/png");
        if (!added) {
            log::warn("verify screenshot: cannot attach {} ({})", png.string(), added.unwrapErr());
            return;
        }

        auto req = web::WebRequest();
        req.bodyMultipart(form);

        async::spawn(req.post(url), [levelID](web::WebResponse response) {
            if (!response.ok()) {
                log::warn("imgbb upload failed ({})", response.code());
                return;
            }
            auto json = response.json().unwrapOr(matjson::Value::object());
            std::string imgUrl;
            if (json.contains("data") && json["data"].contains("url")) {
                imgUrl = json["data"]["url"].asString().unwrapOr("");
            }
            if (imgUrl.empty()) {
                log::warn("imgbb response had no url");
                return;
            }
            log::info("verify screenshot uploaded: {}", imgUrl);
            registerPreview(levelID, imgUrl);
        });
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

        auto path = dirs::getTempDir() / (std::string("petus_verify_") + std::to_string(levelID) + ".png");
        // newCCImage() returns an owned image — grab it once, save, release.
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
        // Test-mode on a real (uploaded) level id. Refine per your needs.
        return this->m_isTestMode && lvl->m_levelID.value() > 0;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (m_fields->m_shot) return;
        if (!isVerifying()) return;

        // Progress 0..100.
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
