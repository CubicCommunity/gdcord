#pragma once

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

#include "base/Singleton.hpp"

namespace gdc {
    class LinkState final : public base::Singleton<LinkState> {
        using UnlinkResult = geode::Result<>;
        using UnlinkCallback = geode::CopyableFunction<void(UnlinkResult)>;
        using UnlinkFuture = arc::Future<UnlinkResult>;

    private:
        DiscordLink m_discord;
        bool m_discordLinked = false;

        bool m_linking = false;

        std::string m_linkState;
        asp::Instant m_linkStart;

        argon::AccountData m_acc;
        std::string m_token;

        geode::utils::web::WebRequest baseRequest() const;

        std::string getUserAgent() const;
        std::string getReqMod() const;

    protected:
        void resetLinkProcess();

        LinkFuture checkLinkStatus();

    public:
        void getLinkAsync(LinkCallback&& callback);
        void startLinkAsync(LinkCallback&& callback);
        void unlinkAsync(UnlinkCallback&& callback);

        LinkFuture getLink();
        LinkFuture startLink();
        UnlinkFuture unlink();

        void setDiscordLinkInfo(DiscordLink discord);

        LinkResult getDiscord() const;
        bool isLinkOngoing() const noexcept;
        bool isLinked() const noexcept;
    };
};