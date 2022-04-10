# iop-hal - IoP's Hardware Abstraction Layer

A moden C++17, low level, framework that provides transparent/unified access to different hardwares, without exposing their inner workings.

If you want a higher level framework, safer, but more opinionated, check: https://github.com/internet-of-plants/iop

Note: Posix target turns some functionalities into No-Ops, we intend to improve this, but some are naturally impossible to implement for this target. If some is needed you should make a specific target for that platform, as it surely will depend on things outside of the C++17 + posix API. The Posix target is mostly for ease of testing.

# Example

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
        if (storage.read(0) == wifiCredsWrittenToFlag) {
            const ssid = storage.read<64>(1);
            iop_assert(ssid, IOP_STR("Invalid address when accessing SSID from storage"));
            const psk = storage.read<32>(65); // flag + ssid
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
            if (storage.read(0) == wifiCredsWrittenToFlag && name == storage.read<64>(1) && password == storage.read<32>(65)) {
                return;
            }

            // Store credentials for simpler reuse
            storage.write(0, wifiCredsWrittenToFlag);
            storage.write(1, *ssid);
            storage.write(65, *psk);
        }

        // Just to show more about wifi, you should not hardcode creds
        // Check the captive_portal.cpp example for a better way
        if (!wifiCredentials && iop::thisThread.timeRunning() > waitUntilUseHardcodedCredetials) {
            wifi.connectToAccessPoint("my-ssid", "my-super-secret-psk");
        }
    }
}
```

# Details

Provides the following functionalities:
- WiFi's Access Point and Station management
  - `#include <iop-hal/wifi.hpp>`
  - `iop::Wifi`
- Firmware upgrade functionality
  - `#include <iop-hal/upgrade.hpp>`
  - `iop::Upgrade`
- Thread management
  - `#include <iop-hal/thread.hpp>`
  - `iop::Thread`
- Type-safe string abstractions and operations
  - `#include <iop-hal/string.hpp>`
  - `iop::StaticString`, `iop::CowString`, `iop::to_view`
- Persistent storage access
  - `#include <iop-hal/storage.hpp>`
  - `iop::Storage`
- HTTP server hosting
  - `#include <iop-hal/server.hpp>`
  - `iop::HttpServer`
- Runtime hook API
  - `#include <iop-hal/runtime.hpp>`
  - User defines `iop_hal::setup` and `iop_hal::loop`
- Panic hook API (no exceptions supported, but we have our panic system)
  - `#include <iop-hal/panic.hpp>`
  - `iop_panic` macro
- HTTPs client
  - `#include <iop-hal/client.hpp>`
  - `iop::HttpClient`
- Higher level client to interface with IoP server, JSON + authentication + firmware upgrade management
  - `#include <iop-hal/network.hpp>`
  - `iop::Network`
- Log system
  - `#include <iop-hal/log.hpp>`
  - `iop::Log`
- Device primitives management
  - `#include <iop-hal/device.hpp>`
  - `iop::Device`