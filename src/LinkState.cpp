#include "LinkState.hpp"

#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

#include <arc/time/Sleep.hpp>

using namespace geode::prelude;
using namespace gdc;

web::WebRequest LinkState::baseRequest() {
    return web::WebRequest()
        .userAgent(LinkState::getUserAgent())
        .timeout(std::chrono::seconds(10));
};

std::string LinkState::getUserAgent() {  // thx argon owo
    if (auto loader = Loader::get()) {
        return fmt::format("gdcord/v{} ({}, Geode {}, GD {})",
            GDC_VERSION,
            platform::getString(),
            loader->getVersion(),
            loader->getGameVersion());
    };

    return "gdcord/v1";
};

std::string LinkState::getReqMod() {
    if (auto mod = Mod::get()) return fmt::format("{}/{}", mod->getID(), mod->getVersion().toVString());
    return "";
};

Result<argon::AccountData> LinkState::verifyLogin() {
    if (auto gjam = GJAccountManager::sharedState()) {
        if (!argon::signedIn()) {
            if (auto ls = LinkState::get()) ls->setDiscordLinkInfo(DiscordLink());
            return Err("User logged out");
        };

        return Ok(argon::getGameAccountData());
    };

    return Err("GJAccountManager not found");
};

void LinkState::setDiscordLinkInfo(DiscordLink discord) {
    auto dc = m_discordLink.lock();

    dc->discord = std::move(discord);
    dc->linked = !dc->discord.id.empty();
};

gdc::LinkResult LinkState::getDiscord() const {
    if (!argon::signedIn()) return Err("User not signed in");

    auto const dc = m_discordLink.lock();
    if (!dc->linked) return Err("Discord account not linked");

    return Ok(dc->discord);
};

bool LinkState::isLinkOngoing() const noexcept {
    return argon::signedIn() && m_attempt.lock()->linking;
};

bool LinkState::isLinked() const noexcept {
    return argon::signedIn() && m_discordLink.lock()->linked;
};

LinkFuture LinkState::getLink() {
    auto acc = *co_await async::waitForMainThread<Result<int>>([this]() -> Result<int> {
        if (auto gjam = GJAccountManager::sharedState()) {
            if (!argon::signedIn()) {
                m_discordLink.lock()->linked = false;
                return Err("User logged out");
            };

            return Ok(gjam->m_accountID);
        };

        return Err("GJAccountManager not found");
    });
    if (acc.isErr()) co_return Err(std::move(acc).unwrapErr());

    auto linked = *co_await async::waitForMainThread<gdc::LinkResult>([this]() -> gdc::LinkResult {
        if (isLinked()) return getDiscord();
        return Err("Discord account linked");
    });
    if (linked.isOk()) co_return std::move(linked);

    auto accountID = std::move(acc).unwrap();

    auto req = LinkState::baseRequest()
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
    m_getTask.cancel();

    m_getTask.spawn(
        getLink(),
        [cb = std::move(callback)](LinkResult res) {
            cb(std::move(res));
        });
};

LinkFuture LinkState::startLink() {
    auto acc = *co_await async::waitForMainThread<Result<argon::AccountData>>(verifyLogin);
    if (acc.isErr()) co_return Err(std::move(acc).unwrapErr());

    auto linked = *co_await async::waitForMainThread<gdc::LinkResult>([this]() -> gdc::LinkResult {
        if (isLinked()) return getDiscord();
        return Err("Discord account linked");
    });
    if (linked.isOk()) co_return std::move(linked);

    auto dcRes = co_await getLink();

    if (dcRes.isOk()) {
        resetLinkProcess();
        co_return std::move(dcRes);
    };

    auto res = co_await argon::startAuth();
    if (res.isErr()) co_return Err(std::move(res).unwrapErr());

    std::string urlState;

    {
        auto lock = m_attempt.lock();

        lock->linkState = utils::random::generateUUID();
        lock->linkStart = asp::Instant::now();
        lock->acc = std::move(acc).unwrap();
        lock->token = std::move(res).unwrap();
        lock->linking = true;

        urlState = lock->linkState;
    };

    web::openLinkInBrowser(fmt::format("https://api.cubicstudios.xyz/breakeode/v1/discord/link/auth?state={}", urlState));

    while (true) {
        std::string stateSnap, tokenSnap;
        argon::AccountData accSnap;
        asp::Instant startSnap;

        {
            auto lock = m_attempt.lock();

            stateSnap = lock->linkState;
            tokenSnap = lock->token;
            accSnap = lock->acc;
            startSnap = lock->linkStart;
        };

        if (stateSnap.empty()) {
            co_return Err("Invalid state");
        } else if (tokenSnap.empty() || !accSnap.valid()) {
            co_return Err("Account login state invalid");
        } else if (asp::Instant::now().durationSince(startSnap).seconds() > 20) {
            co_return Err("Link flow timed out after 20 seconds");
        };

        auto dRes = co_await checkLinkStatus();
        if (dRes.isErr()) {
            log::error("(gdcord) Discord link check failed ({}), trying again in 2.5s", std::move(dRes).unwrapErr());
            co_await arc::sleepFor(asp::Duration::fromMillis(2500));

            continue;
        };

        log::info("(gdcord) Successfully authorized and linked Discord account {}", dRes.unwrap().username);

        resetLinkProcess();
        co_return std::move(dRes);
    };
};

void LinkState::startLinkAsync(LinkCallback&& callback) {
    resetLinkProcess();

    m_startTask.cancel();

    m_startTask.spawn(
        startLink(),
        [this, cb = std::move(callback)](LinkResult res) {
            cb(std::move(res));
        });
};

LinkFuture LinkState::checkLinkStatus() {
    matjson::Value reqJson;

    {
        auto lock = m_attempt.lock();

        reqJson["account_id"] = lock->acc.accountId;
        reqJson["user_id"] = lock->acc.userId;
        reqJson["username"] = lock->acc.username;
        reqJson["authtoken"] = lock->token;
        reqJson["state"] = lock->linkState;
    };

    reqJson["mod"] = getReqMod();

    auto req = LinkState::baseRequest()
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

UnlinkFuture LinkState::unlink() {
    auto res = co_await argon::startAuth();
    if (res.isErr()) co_return Err(std::move(res).unwrapErr());

    auto accRes = *co_await async::waitForMainThread<Result<argon::AccountData>>(verifyLogin);
    if (accRes.isErr()) co_return Err(std::move(accRes).unwrapErr());

    auto const acc = std::move(accRes).unwrap();

    std::string tokenSnap;
    {
        auto lock = m_attempt.lock();

        tokenSnap = lock->token;
    };

    matjson::Value reqJson;
    reqJson["account_id"] = acc.accountId;
    reqJson["user_id"] = acc.userId;
    reqJson["username"] = acc.username;
    reqJson["authtoken"] = tokenSnap;
    reqJson["mod"] = getReqMod();

    auto req = baseRequest().bodyJSON(reqJson);

    auto reqRes = co_await req.post("https://api.cubicstudios.xyz/breakeode/v1/discord/unlink");
    if (reqRes.error()) co_return Err(reqRes.errorMessage());

    {
        auto lock = m_discordLink.lock();

        lock->discord = DiscordLink();
        lock->linked = false;
    };

    log::info("(gdcord) Successfully unlinked Discord account");

    resetLinkProcess();
    co_return Ok();
};

void LinkState::unlinkAsync(UnlinkCallback&& callback) {
    resetLinkProcess();

    m_unlinkTask.cancel();

    m_unlinkTask.spawn(
        unlink(),
        [this, cb = std::move(callback)](UnlinkResult res) {
            cb(std::move(res));
        });
};

void LinkState::resetLinkProcess() {
    auto lock = m_attempt.lock();

    lock->linkState.clear();
    lock->linkStart = asp::Instant();

    lock->acc = argon::AccountData();
    lock->token.clear();

    lock->linking = false;
};