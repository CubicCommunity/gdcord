#pragma once

#include <argon/argon.hpp>

#include <matjson.hpp>

#include <Geode/Result.hpp>

namespace gdc {
    struct DiscordLink final {
        std::string id;
        std::string username;
        std::string avatar;
    };

    using LinkCallback = geode::Function<void(geode::Result<DiscordLink>)>;
    void getLink(LinkCallback callback);
    void startLink(LinkCallback callback);

    bool isLinkOngoing() noexcept;
    bool isLinked() noexcept;

    geode::Result<DiscordLink> getDiscordLink();
};

template <>
struct matjson::Serialize<gdc::DiscordLink> final {
    static geode::Result<gdc::DiscordLink> fromJson(matjson::Value const& value);
    static matjson::Value toJson(gdc::DiscordLink const& value);
};

namespace gdcord = gdc;