#pragma once

#include <fmt/core.h>

#include <argon/argon.hpp>

#include <matjson.hpp>

#include <Geode/Result.hpp>

namespace gdc {
    struct DiscordLink final {
        std::string id;
        std::string username;
        std::string avatar;
    };

    using LinkResult = geode::Result<DiscordLink>;

    using LinkFuture = arc::Future<LinkResult>;
    LinkFuture getLink();
    LinkFuture startLink();

    using LinkCallback = geode::CopyableFunction<void(LinkResult)>;
    void getLinkAsync(LinkCallback&& callback);
    void startLinkAsync(LinkCallback&& callback);

    bool isLinkOngoing() noexcept;
    bool isLinked() noexcept;

    LinkResult getDiscordLink();
};

template <>
struct matjson::Serialize<gdc::DiscordLink> final {
    static geode::Result<gdc::DiscordLink> fromJson(matjson::Value const& value);
    static matjson::Value toJson(gdc::DiscordLink const& value);
};

namespace gdcord = gdc;