# gdcord
A low-level [Argon](https://github.com/GlobedGD/argon)-based library for Geode mods to verify linkage between Geometry Dash and Discord accounts.

## Usage 
You can use gdcord from the client or your server to fetch and verify Discord account information for your players.

### Client
First, be sure to include gdcord as a static dependency for your mod in your **`CMakeLists.txt`**, *after* the `setup_geode_mod` step.
```cmake
CPMAddPackage("gh:CubicCommunity/gdcord@1.0.0")
target_link_libraries(${PROJECT_NAME} gdcord)
```

The only header you'll really need to include is **`gdc.h`**, where everything you'll need is provided.
```cpp
#include <gdcord/gdc.h>
```

It's always important that you first check if the user's account is already linked to begin with. You can do this by using the **`gdc::getLink`** function.
```cpp
#include <gdcord/gdc.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    async::spawn(
        gdc::getLink(),
        [](gdc::LinkResult res) {
            if (res.isErr()) {
                log::warn("Failed to get linked Discord account: {}", res.unwrapErr());
                return;
            };

            auto user = std::move(res).unwrap();
            log::info("Received linked Discord user: @{}", user.username);
        });
};
```

For more direct syntax, you can use our async wrapper functions.
```cpp
gdc::getLinkAsync([](gdc::LinkResult res) {
    // handle result here
});
```

#### When to use `gdc::startLink`?
Since linking involves opening a new web page with the player being required to manually authorize their Discord account, calling **`gdc::startLink`** is *only recommended* when their account information is crucial for authorization or validation for your service.

Granted, the authorization flow in `gdc::startLink` will not occur if the player already linked their account before, and the `gdc::LinkResult` object provided in the callback will contain that same previously saved data.

```cpp
async::spawn(
    gdc::startLink(),
    [](gdc::LinkResult res) {
        // handle result here
    });
```
```cpp
gdc::startLinkAsync([](gdc::LinkResult res) {
    // handle result here
});
```

#### Server
To check a Geometry Dash user's linked Discord account, you only need one **`api.cubicstudios.xyz`** endpoint!
> ### GET `/breakeode/v1/discord`
> #### Query Parameters
> - **`id`**: Account ID of the GD player
> 
> #### Response
> - JSON object
>   - **`id`**: Discord account ID snowflake
>   - **`username`**: Discord username
>   - **`avatar`**: Discord avatar URL (in `WEBP`)

Authorization flows can only be started on the client side, so this endpoint is for your server to verify that the information being sent from the client is accurate to what is actually stored on our platform.