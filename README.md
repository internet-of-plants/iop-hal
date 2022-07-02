# Internet of Plants - Hardware Abstraction Layer

A low level modern C++17 framework that provides opaque and unified access to different hardwares, without exposing their inner workings.

If you want a higher level framework, safer, but more opinionated, check: https://github.com/internet-of-plants/iop

## Targets supported

- IOP_ESP8266 (all boards supported by [esp8266/Arduino](https://github.com/esp8266/Arduino))
- IOP_ESP32 (all boards supported by [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32/))
- IOP_LINUX_MOCK (all targets that support Linux with mocked GPIOs)
- IOP_LINUX (all targets that support Linux)

*Note: Some functionalities in the linux target are NOOP, most will be implemented to support Raspberry Pis and other boards that support linux. But for now this is mostly used for testing*

## Functionalities

Provides the following functionalities:
- [`iop_hal::setup`, `iop_hal::loop`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/runtime.hpp): User defined `iop_hal` entrypoints, from `#include <iop-hal/runtime.hpp>`
- [`iop_hal::WiFi`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/wifi.hpp): Access Point + Station management, use `iop::wifi` from `#include <iop-hal/network.hpp>`
- [`iop::Update`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/update.hpp): Make over-the-air firmware updates, from `#include <iop-hal/update.hpp>`
- [`iop::Thread`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/thread.hpp): Thread management, use `iop::thisThread` from `#include <iop-hal/thread.hpp>`
- [`iop::StaticString`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/string.hpp): Disk stored strings, use it with `IOP_STR(str)` macro, from `#include <iop-hal/string.h>`
- [`iop::CowString`, `iop::to_view`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/string.hpp): Unifies borrowed strings handling, from `#include<iop-hal/string.h>`
  - With string utils functions, we use `std::string` for dynamically allocated strings
  - With fixed sized (non zero terminated) char arrays castable with `iop::to_view`
  - Like `iop::MD5Hash`, `iop::MacAddress`, `iop::NetworkName`, `iop::NetworkPassword`
- [`iop::Storage`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/storage.hpp): Low level access to persistent flat storage, from `#include <iop-hal/storage.hpp>`
- [`iop::HttpServer`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/server.hpp): HTTP server hosting in the device, from `#include <iop-hal/server.hpp>`
- [`iop::CaptivePortal`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/server.hpp): Turns Access Point into a captive portal, from `#include <iop-hal/server.hpp>`
  - Hijacks DNS to redirect all TCP requests in its own AP to some port
- [`iop_panic` and `iop_assert` macros](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/panic.hpp): Fatal error handler hook API, from `#include <iop-hal/panic.hpp>`
  - Panic hooks should never return, either halt/wait for a interaction, or reboot the process
  - Exceptions aren't supported, fatal errors should use iop_hal's panic
- [`iop::HttpClient`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/client.hpp): HTTP(s) client, from `#include <iop-hal/client.hpp>`
- [`iop::Network`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/network.hpp): Higher level HTTP(s) client, from `#include <iop-hal/network.hpp>`
  -  With authentication + JSON requests + update hook
- [`iop::Log`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/client.hpp): String log system, from `#include <iop-hal/log.hpp>`
  - With variadic arguments + levels + extension hooks, for `iop::StaticString` and `std::string_view`
  - One comes from the `IOP_STR(str)` macro, the other from `iop::to_view` + `std::to_string`
- [`iop::Device`](https://github.com/internet-of-plants/iop-hal/blob/main/include/iop-hal/device.hpp): Unified hardware management, from `#include <iop-hal/device.hpp>`

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

## License

[GNU Affero General Public License version 3 or later (AGPL-3.0+)](https://github.com/internet-of-plants/iop-hal/blob/main/LICENSE.md)