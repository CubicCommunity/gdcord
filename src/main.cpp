#include <gdcord/gdc.h>

#include "LinkState.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

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