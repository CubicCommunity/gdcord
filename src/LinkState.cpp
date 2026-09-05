#include "LinkState.hpp"

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

using namespace gdc;
using namespace geode::prelude;

void LinkState::setDiscordLinkInfo(DiscordLink discord) {
    m_discord = std::move(discord);
    m_discordLinked = !m_discord.id.empty();
};

Result<DiscordLink> LinkState::getDiscord() const {
    if (!argon::signedIn()) return Err("User not signed in");
    if (!m_discordLinked) return Err("Discord account not linked");

    return Ok(m_discord);
};

bool LinkState::isLinkOngoing() const noexcept {
    return argon::signedIn() && m_linking;
};

bool LinkState::isLinked() const noexcept {
    return argon::signedIn() && m_discordLinked;
};

void LinkState::getLink(LinkCallback&& callback) {
    m_linkTask.cancel();

    if (!argon::signedIn()) return m_linkCallback(Err("Player is logged out"));

    if (auto gjam = GJAccountManager::sharedState()) {
        m_linkCallback = std::move(callback);

        auto req = web::WebRequest()
                       .param("id", gjam->m_accountID);

        async::spawn(
            req.get("https://api.cubicstudios.xyz/breakeode/v1/discord"),
            [this](web::WebResponse res) {
                auto const fallback = [](std::string_view err = "") {
                    log::error("(gdcord) Discord link web request failed ({})", err);
                };

                if (res.error()) return fallback(res.errorMessage());

                auto jsonRes = res.json();
                if (jsonRes.isErr()) return fallback(std::move(jsonRes).unwrapErr());

                auto json = std::move(jsonRes).unwrap();

                auto discordRes = json.as<DiscordLink>();
                if (discordRes.isErr()) return fallback(std::move(discordRes).unwrapErr());

                setDiscordLinkInfo(discordRes.unwrap());

                log::info("(gdcord) Authorized as Discord user {}", discordRes.unwrap().username);
                m_linkCallback(std::move(discordRes));
                resetLinkProcess();
            });
    };
};

void LinkState::startLink(LinkCallback&& callback) {
    m_linkTask.cancel();
    m_unlinkTask.cancel();

    m_linkCallback = std::move(callback);

    if (!argon::signedIn()) return m_linkCallback(Err("Player is logged out"));

    if (m_discordLinked) return m_linkCallback(getDiscord());

    getLink([this](Result<DiscordLink> res) {
        if (res.isOk()) {
            m_linkCallback(std::move(res));
            resetLinkProcess();

            return;
        };

        async::spawn(
            argon::startAuth(),
            [this](Result<std::string> res) {
                if (res.isErr()) return m_linkCallback(res.asErr());

                if (auto s = CCScheduler::get()) {
                    m_linkState = utils::random::generateUUID();
                    m_linkStart = asp::Instant::now();

                    m_acc = argon::getGameAccountData();
                    m_token = std::move(res).unwrap();

                    m_linking = true;

                    web::openLinkInBrowser(fmt::format("https://api.cubicstudios.xyz/breakeode/v1/discord/link/auth?state={}", m_linkState));

                    s->scheduleSelector(schedule_selector(LinkState::checkLinkStatus), this, 2.5f, 0, 0.f, false);
                };
            });
    });
};

void LinkState::unlink(UnlinkCallback&& callback) {
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

void LinkState::resetLinkProcess() {
    m_linkTask.cancel();
    m_unlinkTask.cancel();

    m_linking = false;

    m_acc = argon::AccountData();
    m_token.clear();

    m_linkState.clear();
    m_linkStart = asp::Instant();
    m_linkCallback = nullptr;
};

void LinkState::checkLinkStatus(float) {
    auto isErr = false;

    if (m_token.empty() || !m_acc.valid()) {
        if (auto s = CCScheduler::get()) s->unscheduleSelector(schedule_selector(LinkState::checkLinkStatus), this);
        m_linkCallback(Err("Account state invalid"));

        isErr = true;
    } else if (asp::Instant::now().durationSince(m_linkStart).seconds() > 30) {
        if (auto s = CCScheduler::get()) s->unscheduleSelector(schedule_selector(LinkState::checkLinkStatus), this);
        m_linkCallback(Err("Link flow timed out after 30s"));

        isErr = true;
    };

    if (isErr) resetLinkProcess();

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
                    log::error("(gdcord) Discord link check failed ({}), trying again in 1.25s", err);
                    s->scheduleSelector(schedule_selector(LinkState::checkLinkStatus), this, 2.5f, 0, 0.f, false);
                };

                if (res.error()) return fallback(res.errorMessage());

                auto jsonRes = res.json();
                if (jsonRes.isErr()) return fallback(std::move(jsonRes).unwrapErr());

                auto json = std::move(jsonRes).unwrap();

                auto discordRes = json.as<DiscordLink>();
                if (discordRes.isErr()) return fallback(std::move(discordRes).unwrapErr());

                auto discord = discordRes.unwrap();

                log::info("(gdcord) Successfully authorized as {}", discord.username);
                setDiscordLinkInfo(std::move(discord));

                s->unscheduleSelector(schedule_selector(LinkState::checkLinkStatus), this);

                m_linkCallback(std::move(discordRes));
                resetLinkProcess();
            };
        });
};