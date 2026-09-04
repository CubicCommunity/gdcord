#pragma once

#include <argon/argon.hpp>

#include <matjson.hpp>

#include <Geode/Result.hpp>

namespace gdcord {
    struct DiscordLink final {
        std::string id;
        std::string username;
        std::string avatar;
    };

    using LinkCallback = geode::Function<void(geode::Result<DiscordLink>)>;
    void startLink(LinkCallback callback);

    bool isLinked() noexcept;

    geode::Result<DiscordLink> getDiscordLink();
};

template <>
struct matjson::Serialize<gdcord::DiscordLink> final {
    static geode::Result<gdcord::DiscordLink> fromJson(matjson::Value const& value);
    static matjson::Value toJson(gdcord::DiscordLink const& value);
};