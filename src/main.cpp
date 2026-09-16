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
    return gdc::LinkState::get()->isLinkOngoing();
};

bool gdc::isLinked() noexcept {
    return gdc::LinkState::get()->isLinked();
};

gdc::LinkResult gdc::getDiscordLink() {
    return gdc::LinkState::get()->getDiscord();
};

gdc::LinkFuture gdc::getLink() {
    co_return gdc::LinkState::get()->getLink().getOutput();
};

void gdc::getLinkAsync(gdc::LinkCallback&& cb) {
    return gdc::LinkState::get()->getLinkAsync(std::move(cb));
};

gdc::LinkFuture gdc::startLink() {
    co_return gdc::LinkState::get()->startLink().getOutput();
};

void gdc::startLinkAsync(gdc::LinkCallback&& cb) {
    return gdc::LinkState::get()->startLinkAsync(std::move(cb));
};

gdc::UnlinkFuture gdc::unlink() {
    co_return gdc::LinkState::get()->unlink().getOutput();
};

void gdc::unlinkAsync(gdc::UnlinkCallback&& cb) {
    return gdc::LinkState::get()->unlinkAsync(std::move(cb));
};

std::string gdc::DiscordLink::getAvatarInFormat(gdc::DiscordImgFmt format, bool animated) const {
    std::string ext;
    ext.reserve(4);

    switch (format) {
        default: return animated ? fmt::format("{}?animated=true", avatar) : avatar;

        case DiscordImgFmt::PNG: ext = ".png"; break;
        case DiscordImgFmt::JPEG: ext = ".jpg"; break;
        case DiscordImgFmt::GIF: ext = ".gif"; break;
    };

    return fmt::format("{}{}", geode::utils::string::replace(avatar, ".webp", ext), animated ? "?animated=true" : "");
};