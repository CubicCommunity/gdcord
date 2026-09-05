#pragma once

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

#include "base/Singleton.hpp"

namespace gdc {
    class LinkState final : public base::Singleton<LinkState>, public cocos2d::CCObject {
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

        void resetLinkProcess();

    protected:
        void checkLinkStatus(float);

    public:
        void getLink(LinkCallback&& callback);
        void startLink(LinkCallback&& callback);
        void unlink(UnlinkCallback&& callback);

        void setDiscordLinkInfo(DiscordLink discord);

        geode::Result<DiscordLink> getDiscord() const;
        bool isLinkOngoing() const noexcept;
        bool isLinked() const noexcept;
    };
};