#pragma once

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

#include "base/Singleton.hpp"

namespace gdc {
    class LinkState final : public base::Singleton<LinkState> {
        using UnlinkResult = geode::Result<>;
        using UnlinkCallback = geode::CopyableFunction<void(UnlinkResult)>;
        using UnlinkFuture = arc::Future<UnlinkResult>;

        struct DiscordLinkInfo final {
            DiscordLink discord;
            bool linked = false;
        };

        struct LinkAttempt final {
            std::string linkState;
            asp::Instant linkStart;
            argon::AccountData acc;
            std::string token;
            bool linking = false;
        };

    private:
        asp::Mutex<DiscordLinkInfo> m_discordLink;

        asp::Mutex<LinkAttempt> m_attempt;

        geode::async::TaskHolder<LinkResult> m_getTask;
        geode::async::TaskHolder<LinkResult> m_startTask;
        geode::async::TaskHolder<UnlinkResult> m_unlinkTask;

        static geode::utils::web::WebRequest baseRequest();

        static std::string getUserAgent();
        static std::string getReqMod();

        // Call only on main thread
        static geode::Result<argon::AccountData> verifyLogin();

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