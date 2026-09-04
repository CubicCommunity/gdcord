#include "AuthState.hpp"

#include <gdcord/gdcord.h>

#include <Geode/Geode.hpp>

using namespace gdcord;
using namespace geode::prelude;

void AuthState::setDiscordLinkInfo(DiscordLink discord) {
    m_discord = std::move(discord);
    m_discordLinked = !m_discord.id.empty();
};

Result<DiscordLink> AuthState::getDiscord() const {
    if (!m_discordLinked) return Err("Discord account not linked");
    return Ok(m_discord);
};

bool AuthState::isLinkOngoing() const noexcept {
    return m_linking;
};

bool AuthState::isLinked() const noexcept {
    return m_discordLinked;
};

void AuthState::startLink(LinkCallback&& callback) {
    m_linkCallback = std::move(callback);

    if (!argon::signedIn()) return m_linkCallback(Err("Player is logged out"));

    if (m_discordLinked) return m_linkCallback(getDiscord());

    async::spawn(
        argon::startAuth(),
        [this](Result<std::string> res) {
            if (res.isErr()) return m_linkCallback(res.asErr());

            if (auto s = CCScheduler::get()) {
                m_linkState = utils::random::generateUUID();
                m_linkStart = asp::Instant::now();

                m_acc = argon::getGameAccountData();

                m_linking = true;

                s->scheduleSelector(schedule_selector(AuthState::checkLinkStatus), this, 2.5f, 0, 0.f, false);
            };
        });
};

void AuthState::unlink(UnlinkCallback&& callback) {
    m_linkTask.cancel();

    if (!argon::signedIn()) return m_linkCallback(Err("Player is logged out"));

    async::spawn(
        argon::startAuth(),
        [this, cb = std::move(callback)](Result<std::string> res) {
            auto reqJson = matjson::Value();
            reqJson["account_id"] = m_acc.accountId;
            reqJson["user_id"] = m_acc.userId;
            reqJson["username"] = m_acc.username;
            reqJson["authtoken"] = m_token;

            auto req = web::WebRequest()
                           .bodyJSON(reqJson);

            m_unlinkTask.spawn(
                req.post("https://api.cubicstudios.xyz/breakeode/v1/discord/unlink"),
                [this, cb = std::move(cb)](web::WebResponse res) {
                    if (res.error()) return cb(Err(res.errorMessage()));

                    m_discord = DiscordLink();
                    m_discordLinked = false;

                    cb(Ok());
                });
        });
};

void AuthState::resetLinkState() {
    m_linking = false;

    m_acc = argon::AccountData();
    m_token.clear();

    m_linkState.clear();
    m_linkStart = asp::Instant();
    m_linkCallback = nullptr;
};

void AuthState::checkLinkStatus(float) {
    auto isErr = false;

    if (m_token.empty() || !m_acc.valid()) {
        if (auto s = CCScheduler::get()) s->unscheduleSelector(schedule_selector(AuthState::checkLinkStatus), this);
        m_linkCallback(Err("Account state invalid"));

        isErr = true;
    } else if (asp::Instant::now().durationSince(m_linkStart).seconds() > 30) {
        if (auto s = CCScheduler::get()) s->unscheduleSelector(schedule_selector(AuthState::checkLinkStatus), this);
        m_linkCallback(Err("Link flow timed out after 30s"));

        isErr = true;
    };

    if (isErr) resetLinkState();

    web::openLinkInBrowser(fmt::format("https://api.cubicstudios.xyz/breakeode/v1/discord/link/auth?state={}", m_linkState));

    auto reqJson = matjson::Value();
    reqJson["account_id"] = m_acc.accountId;
    reqJson["user_id"] = m_acc.userId;
    reqJson["username"] = m_acc.username;
    reqJson["authtoken"] = m_token;
    reqJson["state"] = m_linkState;

    auto req = web::WebRequest()
                   .bodyJSON(reqJson);

    m_linkTask.spawn(
        req.post("https://api.cubicstudios.xyz/breakeode/v1/discord/link/check"),
        [this](web::WebResponse res) {
            if (auto s = CCScheduler::get()) {
                auto const fallback = [this, s](std::string_view err = "") {
                    log::error("Discord link check failed ({}), trying again in 1.25s", err);
                    s->scheduleSelector(schedule_selector(AuthState::checkLinkStatus), this, 2.5f, 0, 0.f, false);
                };

                if (res.error()) return fallback(res.errorMessage());

                auto jsonRes = res.json();
                if (jsonRes.isErr()) return fallback(std::move(jsonRes).unwrapErr());

                auto json = std::move(jsonRes).unwrap();

                auto discordRes = json.as<DiscordLink>();
                if (discordRes.isErr()) return fallback(std::move(discordRes).unwrapErr());

                auto discord = std::move(discordRes).unwrap();

                log::info("Successfully authorized as {}", discord.username);
                setDiscordLinkInfo(std::move(discord));

                s->unscheduleSelector(schedule_selector(AuthState::checkLinkStatus), this);

                m_linkCallback(getDiscord());
                resetLinkState();
            };
        });
};