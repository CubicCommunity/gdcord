#pragma once

#include <gdcord/gdcord.h>

#include <Geode/Geode.hpp>

#include "base/Singleton.hpp"

namespace gdcord {
    class AuthState final : public base::Singleton<AuthState>, public cocos2d::CCObject {
        using UnlinkCallback = geode::CopyableFunction<void(geode::Result<>)>;

    private:
        DiscordLink m_discord;
        bool m_discordLinked = false;

        bool m_linking = false;

        std::string m_linkState;
        asp::Instant m_linkStart;
        LinkCallback m_linkCallback;

        argon::AccountData m_acc;
        std::string m_token;

        geode::async::TaskHolder<geode::utils::web::WebResponse> m_linkTask;
        geode::async::TaskHolder<geode::utils::web::WebResponse> m_unlinkTask;

        void resetLinkState();

    protected:
        void checkLinkStatus(float);

    public:
        void startLink(LinkCallback&& callback);
        void unlink(UnlinkCallback&& callback);

        void setDiscordLinkInfo(DiscordLink discord);

        geode::Result<DiscordLink> getDiscord() const;
        bool isLinkOngoing() const noexcept;
        bool isLinked() const noexcept;
    };
};