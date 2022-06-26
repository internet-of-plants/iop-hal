# iop-hal - IoP's Hardware Abstraction Layer

A low level modern C++17 framework that provides opaque and unified access to different hardwares, without exposing their inner workings.

If you want a higher level framework, safer, but more opinionated, check: https://github.com/internet-of-plants/iop

## Targets supported

- ESP8266 (all boards supported by [esp8266/Arduino](https://github.com/esp8266/Arduino))
- ESP32 (all boards supported by [espessif/arduino-esp32](https://github.com/espressif/arduino-esp32/))
- Posix (all targets that support POSIX)

*Note: Some functionalities in the posix target are NOOP, most will be implemented to support Raspberry Pis and other boards that support posix. But for now this is mostly used for testing*

## Functionalities

Provides the following functionalities:
- [User defined `iop_hal::setup` and `iop_hal::loop`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/runtime.hpp): Entrypoints of the framework, from `#include <iop-hal/runtime.hpp>`
- [`iop_hal::WiFi`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/wifi.hpp): Access Point + Station management, use `iop::wifi` from `#include <iop-hal/network.hpp>`
- [`iop::Upgrade`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/upgrade.hpp): Make over-the-air firmware upgrades, from `#include <iop-hal/upgrade.hpp>`
- [`iop::Thread`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/thread.hpp): Thread management, use `iop::thisThread` from `#include <iop-hal/thread.hpp>`
- [`iop::StaticString`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/string.hpp): Disk stored strings, use it with `IOP_STR(str)` macro, from `#include <iop-hal/string.h>`
- [`iop::CowString`, `iop::to_view` and string handling functions](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/string.hpp): Borrow strings to handle them in a unified way, from `#include<iop-hal/string.h>`
  - Also includes fixed size char arrays `iop::MD5Hash`, `iop::MacAddress`, `iop::NetworkName`, `iop::NetworkPassword` that can be accessed as strings with `iop::to_view`
- [`iop::Storage`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/storage.hpp): Low level access to persistent storage as continuous memory, from `#include <iop-hal/storage.hpp>`
- [`iop::HttpServer`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/server.hpp): HTTP server hosting in the device, from `#include <iop-hal/server.hpp>`
- [`iop::CaptivePortal`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/server.hpp): Turns access point into a captive portal, redirecting all queries to itself, - [`iop_panic` and `iop_assert` macros](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/panic.hpp): Panic hook API (exceptions aren't supported, fatal errors should use iop_hal's panic), from `#include <iop-hal/panic.hpp>`
- [`iop::HttpClient`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/client.hpp): HTTP(s) client, from `#include <iop-hal/client.hpp>`
- [`iop::Network`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/network.hpp): Higher level HTTP(s) client with authentication + JSON requests + upgrade hooks, from `#include <iop-hal/network.hpp>`
- [`iop::Log`](- [`iop::HttpClient`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/client.hpp): String log system with variadic arguments + levels + hooks for extension, from `#include <iop-hal/log.hpp>`
  - Supports `iop::StaticString` or `std::string_view`, the former can be created with the `IOP_STR(str)` macro, the latest can be created with `iop::to_view` (non string data has to be casted with `std::to_string` or similar)
- [`iop::Device`](- [`iop::HttpClient`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/device.hpp): Unified hardware management, from `#include <iop-hal/device.hpp>`

## Example

```cpp
#include <iop-hal/runtime.hpp>
#include <iop-hal/string.hpp>
#include <iop-hal/storage.hpp>
#include <optional>

static iop::Wifi wifi;
static uint8_t wifiCredsWrittenToFlag = 128;
static std::optional<std::pair<std::array<char, 64>, std::array<char, 32>>> wifiCredentials;
static bool gotWifiCreds = false;

constexpr static iop::time::milliseconds twoHours = 2 * 3600 * 1000;
static iop::time::milliseconds waitUntilUseHardcodedCredentials = iop::thisThread.timeRunning() + twoHours;

namespace iop {
    auto setup() noexcept -> void {
        wifi.setup();
        // We shouldn't make expensive operations here, set a flag to handle later
        // TODO: iop_hal should do this for the user, allowing for a safer Wifi::onConnect
        // (that also passes the credentials as parameter)
        wifi.onConnect([] { gotWifiCreds = true; });

        // Raw storage access, storage is just a huge array backed by HDD/SSD/Flash, not RAM
        if (storage.get(0) == wifiCredsWrittenToFlag) {
            const auto ssid = storage.read<64>(1);
            iop_assert(ssid, IOP_STR("Invalid address when accessing SSID from storage"));
            const auto psk = storage.read<32>(65); // flag + ssid
            iop_assert(psk, IOP_STR("Invalid address when accessing PSK from storage"));
            wifiCredentials = std::make_pair(*ssid, *psk);

            wifi.connectToAccessPoint(iop::to_view(*ssid), iop::to_view(*psk));
        }
    }

    auto loop() noexcept -> void {
        if (gotWifiCreds) {
            gotWifiCreds = false;
            const auto [name, password] = wifi.credentials();
            wifiCredentials = std::make_pair(name, password);

            // Avoids writing to flash if not needed
            if (storage.get(0) == wifiCredsWrittenToFlag && name == storage.read<64>(1) && password == storage.read<32>(65)) {
                return;
            }

            // Store credentials for simpler reuse
            storage.set(0, wifiCredsWrittenToFlag);
            storage.write(1, name);
            storage.write(65, password);
        }

        // Just to show more about wifi, you should not hardcode creds
        // Check the captive_portal.cpp example for a better way
        if (!wifiCredentials && iop::thisThread.timeRunning() > waitUntilUseHardcodedCredetials) {
            wifi.connectToAccessPoint("my-ssid", "my-super-secret-psk");
        }
    }
}
```