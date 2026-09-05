#include <gdcord/gdc.h>

#include "LinkState.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

Result<gdc::DiscordLink> matjson::Serialize<gdc::DiscordLink>::fromJson(matjson::Value const& value) {
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

bool gdc::isLinked() noexcept {
    if (auto as = gdc::LinkState::get()) return as->isLinked();
    return false;
};

Result<gdc::DiscordLink> gdc::getDiscordLink() {
    if (auto as = gdc::LinkState::get()) return as->getDiscord();
    return Err("Auth state not found");
};

void gdc::getLink(gdc::LinkCallback cb) {
    if (auto as = gdc::LinkState::get()) return as->getLink(std::move(cb));
    return cb(Err("Auth state not found"));
};

void gdc::startLink(gdc::LinkCallback cb) {
    if (auto as = gdc::LinkState::get()) return as->startLink(std::move(cb));
    return cb(Err("Auth state not found"));
};