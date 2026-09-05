#include "LinkState.hpp"

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

#include <arc/time/Sleep.hpp>

using namespace gdc;
using namespace geode::prelude;

web::WebRequest LinkState::baseRequest() const {
    return web::WebRequest()
        .userAgent(getUserAgent())
        .timeout(std::chrono::seconds(10));
};

std::string LinkState::getUserAgent() const {  // thx argon owo
    if (auto loader = Loader::get()) {
        return fmt::format("gdcord/v{} ({}, Geode {}, GD {})",
            GDC_VERSION,
            platform::getString(),
            loader->getVersion(),
            loader->getGameVersion());
    };

    return "";
};

std::string LinkState::getReqMod() const {
    if (auto mod = Mod::get()) return fmt::format("{}/{}", mod->getID(), mod->getVersion().toVString());
    return "";
};

void LinkState::setDiscordLinkInfo(DiscordLink discord) {
    m_discord = std::move(discord);
    m_discordLinked = !m_discord.id.empty();
};

gdc::LinkResult LinkState::getDiscord() const {
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

LinkFuture LinkState::getLink() {
    auto acc = *co_await async::waitForMainThread<Result<int>>([this]() -> Result<int> {
        if (auto gjam = GJAccountManager::sharedState()) {
            if (!argon::signedIn()) {
                m_discordLinked = false;
                return Err("User logged out");
            };

            return Ok(gjam->m_accountID);
        };

        return Err("GJAccountManager not found");
    });

    if (acc.isErr()) co_return Err(std::move(acc).unwrapErr());
    if (isLinked()) co_return getDiscord();

    auto accountID = std::move(acc).unwrap();

    auto req = baseRequest()
                   .param("id", accountID);

    auto res = co_await req.get("https://api.cubicstudios.xyz/breakeode/v1/discord");
    if (res.error()) co_return Err(res.errorMessage());

    auto jsonRes = res.json();
    if (jsonRes.isErr()) co_return Err(std::move(jsonRes).unwrapErr());

    auto json = std::move(jsonRes).unwrap();

    auto discordRes = json.as<DiscordLink>();
    if (discordRes.isErr()) co_return Err(std::move(discordRes).unwrapErr());

    setDiscordLinkInfo(discordRes.unwrap());

    log::info("(gdcord) Authorized as Discord user {}", discordRes.unwrap().username);
    co_return std::move(discordRes);
};

void LinkState::getLinkAsync(LinkCallback&& callback) {
    async::spawn(
        getLink(),
        [cb = std::move(callback)](LinkResult res) {
            cb(std::move(res));
        });
};

LinkFuture LinkState::startLink() {
    auto acc = *co_await async::waitForMainThread<Result<argon::AccountData>>([this]() -> Result<argon::AccountData> {
        if (auto gjam = GJAccountManager::sharedState()) {
            if (!argon::signedIn()) {
                m_discordLinked = false;
                return Err("User logged out");
            };

            return Ok(argon::getGameAccountData());
        };

        return Err("GJAccountManager not found");
    });

    if (acc.isErr()) co_return Err(std::move(acc).unwrapErr());
    if (isLinked()) co_return getDiscord();

    auto dcRes = co_await getLink();

    if (dcRes.isOk()) {
        resetLinkProcess();
        co_return std::move(dcRes);
    };

    auto res = co_await argon::startAuth();
    if (res.isErr()) co_return Err(std::move(res).unwrapErr());

    m_linkState = utils::random::generateUUID();
    m_linkStart = asp::Instant::now();

    m_acc = std::move(acc).unwrap();
    m_token = std::move(res).unwrap();

    web::openLinkInBrowser(fmt::format("https://api.cubicstudios.xyz/breakeode/v1/discord/link/auth?state={}", m_linkState));

    m_linking = true;

    auto ok = false;
    while (!ok) {
        if (m_linkState.empty()) {  // who knows lol
            co_return Err("Invalid state");
        } else if (m_token.empty() || !m_acc.valid()) {
            co_return Err("Account login state invalid");
        } else if (asp::Instant::now().durationSince(m_linkStart).seconds() > 20) {
            co_return Err("Link flow timed out after 20 seconds");
        };

        auto dRes = co_await checkLinkStatus();
        if (dRes.isErr()) {
            log::error("(gdcord) Discord link check failed ({}), trying again in 2.5s", std::move(dRes).unwrapErr());
            co_await arc::sleepFor(asp::Duration::fromMillis(2500));

            continue;
        };

        resetLinkProcess();
        co_return std::move(dRes);
    };

    co_return Err("Unknown error");
};

void LinkState::startLinkAsync(LinkCallback&& callback) {
    resetLinkProcess();

    async::spawn(
        startLink(),
        [this, cb = std::move(callback)](LinkResult res) {
            cb(std::move(res));
        });
};

LinkFuture LinkState::checkLinkStatus() {
    auto reqJson = matjson::Value();
    reqJson["account_id"] = m_acc.accountId;
    reqJson["user_id"] = m_acc.userId;
    reqJson["username"] = m_acc.username;
    reqJson["authtoken"] = m_token;
    reqJson["mod"] = getReqMod();
    reqJson["state"] = m_linkState;

    auto req = baseRequest()
                   .bodyJSON(reqJson);

    auto res = co_await req.post("https://api.cubicstudios.xyz/breakeode/v1/discord/link/check");
    if (res.error()) co_return Err(std::string{res.errorMessage()});

    auto jsonRes = res.json();
    if (jsonRes.isErr()) co_return Err(std::move(jsonRes).unwrapErr());

    auto json = std::move(jsonRes).unwrap();

    auto discordRes = json.as<DiscordLink>();
    if (discordRes.isErr()) co_return Err(std::move(discordRes).unwrapErr());

    auto discord = discordRes.unwrap();

    log::info("(gdcord) Successfully authorized as {}", discord.username);
    setDiscordLinkInfo(std::move(discord));

    resetLinkProcess();
    co_return std::move(discordRes);
};

LinkState::UnlinkFuture LinkState::unlink() {
    auto res = co_await argon::startAuth();
    if (res.isErr()) co_return Err(std::move(res).unwrapErr());

    auto accRes = *co_await async::waitForMainThread<Result<argon::AccountData>>([this]() -> Result<argon::AccountData> {
        if (auto gjam = GJAccountManager::sharedState()) {
            if (!argon::signedIn()) {
                m_discordLinked = false;
                return Err("User logged out");
            };

            return Ok(argon::getGameAccountData());
        };

        return Err("GJAccountManager not found");
    });
    if (accRes.isErr()) co_return Err(std::move(accRes).unwrapErr());

    auto const acc = std::move(accRes).unwrap();

    auto reqJson = matjson::Value();
    reqJson["account_id"] = acc.accountId;
    reqJson["user_id"] = acc.userId;
    reqJson["username"] = acc.username;
    reqJson["authtoken"] = m_token;
    reqJson["mod"] = getReqMod();

    auto req = baseRequest()
                   .bodyJSON(reqJson);

    auto reqRes = co_await req.post("https://api.cubicstudios.xyz/breakeode/v1/discord/unlink");
    if (reqRes.error()) co_return Err(reqRes.errorMessage());

    m_discord = DiscordLink();
    m_discordLinked = false;

    resetLinkProcess();
    co_return Ok();
};

void LinkState::unlinkAsync(UnlinkCallback&& callback) {
    resetLinkProcess();

    async::spawn(
        unlink(),
        [this, cb = std::move(callback)](UnlinkResult res) {
            cb(std::move(res));
        });
};

void LinkState::resetLinkProcess() {
    m_linking = false;

    m_acc = argon::AccountData();
    m_token.clear();

    m_linkState.clear();
    m_linkStart = asp::Instant();
};