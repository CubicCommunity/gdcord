#include <gdcord/gdc.h>

#include "LinkState.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

gdc::LinkResult matjson::Serialize<gdc::DiscordLink>::fromJson(matjson::Value const& value) {
    if (!value.isObject()) return Err("Expected an object");

    GEODE_UNWRAP_INTO(std::string id, value["id"].asString());
    GEODE_UNWRAP_INTO(std::string username, value["username"].asString());
    GEODE_UNWRAP_INTO(std::string avatar, value["avatar"].asString());

    return Ok(gdc::DiscordLink{
        std::move(id),
        std::move(username),
        std::move(avatar),
    });
};

matjson::Value matjson::Serialize<gdc::DiscordLink>::toJson(gdc::DiscordLink const& value) {
    auto obj = matjson::Value();

    obj["id"] = value.id;
    obj["username"] = value.username;
    obj["avatar"] = value.avatar;

    return obj;
};

bool gdc::isLinkOngoing() noexcept {
    if (auto ls = gdc::LinkState::get()) return ls->isLinkOngoing();
    return false;
};

bool gdc::isLinked() noexcept {
    if (auto ls = gdc::LinkState::get()) return ls->isLinked();
    return false;
};

gdc::LinkResult gdc::getDiscordLink() {
    if (auto ls = gdc::LinkState::get()) return ls->getDiscord();
    return Err("Auth state not found");
};

gdc::LinkFuture gdc::getLink() {
    if (auto ls = LinkState::get()) co_return ls->getLink().getOutput();
    co_return Err("Auth state not found");
};

void gdc::getLinkAsync(gdc::LinkCallback&& cb) {
    if (auto ls = gdc::LinkState::get()) return ls->getLinkAsync(std::move(cb));
    return cb(Err("Auth state not found"));
};

gdc::LinkFuture gdc::startLink() {
    if (auto ls = LinkState::get()) co_return ls->startLink().getOutput();
    co_return Err("Auth state not found");
};

void gdc::startLinkAsync(gdc::LinkCallback&& cb) {
    if (auto ls = gdc::LinkState::get()) return ls->startLinkAsync(std::move(cb));
    return cb(Err("Auth state not found"));
};