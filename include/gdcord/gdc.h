#pragma once

#include <fmt/core.h>

#include <argon/argon.hpp>

#include <matjson.hpp>

#include <Geode/Result.hpp>

#include <Geode/utils/string.hpp>

#include <Geode/cocos/platform/CCImage.h>

namespace gdc {
    enum class DiscordImgFmt : uint8_t {
        WEBP = 0,
        PNG = 1,
        JPEG = 2,
        GIF = 3,
    };

    struct DiscordLink final {
        std::string id;        // Discord user ID snowflake
        std::string username;  // Discord user handle
        std::string avatar;    // Discord avatar URL in `WEBP` format

        // Get this user's Discord avatar URL in a different image format, animated or not
        std::string getAvatarInFormat(DiscordImgFmt format, bool animated = false) const;
    };

    using LinkResult = geode::Result<DiscordLink>;
    using LinkFuture = arc::Future<LinkResult>;

    /// Get previously saved Discord account data, if any
    LinkFuture getLink();
    /// Start the Discord authorization flow for the user
    /// @note Make sure you checked by calling `gdc::getLink` prior!
    LinkFuture startLink();

    using LinkCallback = geode::CopyableFunction<void(LinkResult)>;

    /// Get previously saved Discord account data, if any
    void getLinkAsync(LinkCallback&& callback);
    /// Start the Discord authorization flow for the user
    /// @note Make sure you checked by calling `gdc::getLinkAsync` prior!
    void startLinkAsync(LinkCallback&& callback);

    /// Check if there's an ongoing link attempt
    /// @warning Call only on main thread
    bool isLinkOngoing() noexcept;
    /// Check if the player already linked their account
    /// @warning Call only on main thread
    bool isLinked() noexcept;

    /// Get the Discord account linked to the player's Geometry Dash account, if any
    /// @warning Call only on main thread
    LinkResult getDiscordLink();

    using UnlinkResult = geode::Result<>;
    using UnlinkFuture = arc::Future<UnlinkResult>;

    /// Remove the Discord link from the current user's account
    UnlinkFuture unlink();

    using UnlinkCallback = geode::CopyableFunction<void(UnlinkResult)>;

    /// Remove the Discord link from the current user's account
    void unlinkAsync(UnlinkCallback&& callback);
};

template <>
struct matjson::Serialize<gdc::DiscordLink> final {
    static geode::Result<gdc::DiscordLink> fromJson(matjson::Value const& value);
    static matjson::Value toJson(gdc::DiscordLink const& value);
};

namespace gdcord = gdc;