#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <string>

// The Versus lobby / matchmaking UI. Opened from the main-menu Versus button in
// "menu" mode (create a lobby or accept an invite), then transitions to the
// live lobby view (members, level voting, ready-up, start).
namespace petus::versus {

    class VersusLobbyPopup : public geode::Popup {
    protected:
        enum class Mode { Menu, Lobby };
        Mode m_mode = Mode::Menu;

        std::string m_lobbyId;
        matjson::Value m_lobby;         // last lobby DTO (Lobby mode)
        matjson::Value m_invites;       // last invites list (Menu mode)
        matjson::Value m_top;           // top players (invite sub-popup)

        cocos2d::CCNode* m_content = nullptr;   // cleared + rebuilt each refresh
        bool m_closing = false;
        bool m_busy = false;            // guards double-submits on buttons

        bool init() override;
        void rebuild();
        void buildMenu();
        void buildLobby();

        // ---- networking --------------------------------------------------
        void pollTick(float);
        void refreshInvites();
        void refreshLobby();
        void enterLobbyDTO(matjson::Value dto);

        // ---- actions -----------------------------------------------------
        void createLobby(bool ranked);
        void respondInvite(const std::string& lobbyId, bool accept);
        void doReady();
        void doVote(int levelID);
        void doStart();
        void doLeave();
        void doTimeLimit(int multiplier);
        void openInviteSub();
        void openLevelsSub();

        void onClose(cocos2d::CCObject*) override;

        // small helpers
        bool amHost() const;
        int myReady() const;
        int myVote() const;

    public:
        static VersusLobbyPopup* create();
    };

    // Entry point used by the menu button.
    void openVersusMenu();

}
