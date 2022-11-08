
#include <iop-hal/runtime.hpp>
#include <iop-hal/string.hpp>
#include <iop-hal/storage.hpp>
#include <optional>

static iop::Wifi wifi;
static std::optional<std::pair<iop::HttpServer, iop::CaptivePortal>> server;
static iop::Storage storage;

static bool gotWifiCreds = false;

static uint8_t wifiCredsWrittenToFlag = 128;
static std::optional<std::array<char, 64>> ssid;
static std::optional<std::array<char, 32>> psk;
static uint8_t wifiCredsSizeInStorage = 1 + 64 + 32;

static bool tokenWrittenToFlag = 127;
static std::optional<std::array<char, 64>> authToken;

namespace iop {
    auto setup() noexcept -> void {
        wifi.setup();
        if (storage.get(0) == wifiCredsWrittenToFlag) {
            ssid = storage.read<64>(1);
            iop_assert(ssid, IOP_STR("Invalid address when accessing SSID from storage"));
            psk = storage.read<32>(65); // flag + ssid
            iop_assert(psk, IOP_STR("Invalid address when accessing PSK from storage"));
        }
        if (storage.get(wifiCredsSizeInStorage) == tokenWrittenToFlag) {
            authToken = storage.read<64>(wifiCredsSizeInStorage + 1);
            iop_assert(authToken, IOP_STR("Invalid address when accessing authToken from storage"));
        }

        wifi.onConnect([] { gotWifiCreds = true; });

        if (!ssid || !psk) {
            auto http = std::make_option(HttpServer());
            http->begin();
            auto captivePortal = std::make_option(CaptivePortal());
            captivePortal->start();

            server = std::make_optional(std::make_pair(http, captivePortal));
        } else {
            wifi.connectToAccessPoint(iop::to_view(*ssid), iop::to_view(*psk));
        }
    }

    auto loop() noexcept -> void {
        if (gotWifiCreds) {
            gotWifiCreds = false;
            const auto [name, password] = wifi.credentials();
            ssid = std::make_option(name);
            psk = std::make_option(password);

            // Avoids writing to flash if not needed
            if (storage.get(0) == wifiCredsWrittenToFlag && ssid == storage.read<64>(1) && psk == storage.read<32>(65)) {
                return;
            }

            storage.set(0, wifiCredsWrittenToFlag);
            storage.write(1, *ssid);
            storage.write(65, *psk);
            storage.commit();
        }

        if (ssid && psk && authToken) {
            server = std::nullopt;
        }

        if (server) {
            const auto& [http, captivePortal] = *server;
            server->handleClient();
            captivePortal->handleClient();
            return;
        }

    }
}