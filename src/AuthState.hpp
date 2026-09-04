#pragma once

#include <gdcord/gdcord.h>

#include <Geode/Geode.hpp>

#include "base/Singleton.hpp"

namespace gdcord {
    class AuthState final : public base::Singleton<AuthState>, public cocos2d::CCObject {
    private:
        DiscordLink m_discord;
        bool m_discordLinked = false;

        std::string m_linkState;
        asp::Instant m_linkStart;
        LinkCallback m_linkCallback;

        argon::AccountData m_acc;
        std::string m_token;

        void resetLinkState();

    protected:
        void checkLinkStatus(float);

    public:
        void startLink(LinkCallback&& callback);

        void setDiscordLinkInfo(DiscordLink discord);

        geode::Result<DiscordLink> getDiscord() const;
        bool isLinked() const noexcept;
    };
};