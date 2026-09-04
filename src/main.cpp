#include <gdcord/gdcord.h>

#include "AuthState.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

bool gdcord::isLinked() noexcept {
    if (auto as = gdcord::AuthState::get()) return as->isLinked();
    return false;
};

Result<gdcord::DiscordLink> gdcord::getDiscordLink() {
    if (auto as = gdcord::AuthState::get()) return as->getDiscord();
    return Err("Auth state not found");
};

void gdcord::startLink(gdcord::LinkCallback cb) {
    if (auto as = gdcord::AuthState::get()) return as->startLink(std::move(cb));
    return cb(Err("Auth state not found"));
};