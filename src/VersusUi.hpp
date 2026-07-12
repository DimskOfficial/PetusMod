#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <sstream>

// Small shared helpers for the Versus UI: safe JSON field reads and building a
// SimplePlayer cube from the server's "cube:c1:c2:c3:glow" icon string.
namespace petus::versus {

    // ---- JSON accessors (matjson Result-based getters made ergonomic) ------
    inline std::string jstr(const matjson::Value& v, const char* key, const std::string& def = "") {
        if (!v.contains(key)) return def;
        return v[key].asString().unwrapOr(def);
    }
    inline int jint(const matjson::Value& v, const char* key, int def = 0) {
        if (!v.contains(key)) return def;
        return static_cast<int>(v[key].asInt().unwrapOr(def));
    }
    inline long long jll(const matjson::Value& v, const char* key, long long def = 0) {
        if (!v.contains(key)) return def;
        return static_cast<long long>(v[key].asInt().unwrapOr(def));
    }
    inline double jdbl(const matjson::Value& v, const char* key, double def = 0) {
        if (!v.contains(key)) return def;
        return v[key].asDouble().unwrapOr(def);
    }
    inline bool jbool(const matjson::Value& v, const char* key, bool def = false) {
        if (!v.contains(key)) return def;
        // Accept both real booleans and 0/1 integers.
        auto b = v[key].asBool();
        if (b) return b.unwrap();
        return v[key].asInt().unwrapOr(def ? 1 : 0) != 0;
    }

    // Copy an array field into a vector (asArray() on a reference yields a
    // reference Result that can't take a fallback, so we copy element-wise).
    inline std::vector<matjson::Value> jarr(const matjson::Value& v, const char* key) {
        std::vector<matjson::Value> out;
        if (v.contains(key) && v[key].isArray()) {
            for (auto const& e : v[key]) out.push_back(e);
        }
        return out;
    }
    // Same for a Value that is itself an array.
    inline std::vector<matjson::Value> jarr(const matjson::Value& v) {
        std::vector<matjson::Value> out;
        if (v.isArray()) {
            for (auto const& e : v) out.push_back(e);
        }
        return out;
    }

    // ---- Icon string -------------------------------------------------------
    struct IconParts {
        int cube = 1;
        int col1 = 0;
        int col2 = 3;
        int col3 = 12;
        bool glow = false;
    };

    inline IconParts parseIcon(const std::string& icon) {
        IconParts p;
        std::vector<int> nums;
        std::stringstream ss(icon);
        std::string tok;
        while (std::getline(ss, tok, ':')) {
            try { nums.push_back(tok.empty() ? 0 : std::stoi(tok)); }
            catch (...) { nums.push_back(0); }
        }
        if (nums.size() > 0 && nums[0] > 0) p.cube = nums[0];
        if (nums.size() > 1) p.col1 = nums[1];
        if (nums.size() > 2) p.col2 = nums[2];
        if (nums.size() > 3) p.col3 = nums[3];
        if (nums.size() > 4) p.glow = nums[4] != 0;
        return p;
    }

    // Build a rendered cube SimplePlayer from an icon string. Returns nullptr
    // on failure (caller should null-check). Scale to taste after.
    inline cocos2d::CCNode* makeCube(const std::string& icon) {
        using namespace geode::prelude;
        auto parts = parseIcon(icon);
        auto* sp = SimplePlayer::create(parts.cube);
        if (!sp) return nullptr;
        auto* gm = GameManager::sharedState();
        if (gm) {
            sp->setColor(gm->colorForIdx(parts.col1));
            sp->setSecondColor(gm->colorForIdx(parts.col2));
            if (parts.glow) {
                sp->setGlowOutline(gm->colorForIdx(parts.col3));
            }
        }
        sp->updatePlayerFrame(parts.cube, IconType::Cube);
        return sp;
    }

    // ms -> "m:ss.mmm" (for finish times).
    inline std::string formatTime(long long ms) {
        if (ms < 0) ms = 0;
        long long totalMs = ms % 1000;
        long long totalSec = ms / 1000;
        long long sec = totalSec % 60;
        long long min = totalSec / 60;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%lld:%02lld.%03lld", min, sec, totalMs);
        return std::string(buf);
    }

    // ms -> "mm:ss" (for remaining clock).
    inline std::string formatClock(long long ms) {
        if (ms < 0) ms = 0;
        long long totalSec = ms / 1000;
        long long sec = totalSec % 60;
        long long min = totalSec / 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld", min, sec);
        return std::string(buf);
    }

}
