#include "iop-hal/wifi.hpp"
#include "iop-hal/panic.hpp"
#include "iop-hal/thread.hpp"

#include "esp/esp32/generated/certificates.hpp"

#include "WiFi.h"

#ifdef IOP_SSL
#include "WiFiClientSecure.h"
using NetworkClient = WiFiClientSecure;
#else
using NetworkClient = WiFiClient;
#endif

namespace iop_hal {
Wifi::Wifi() noexcept: client(new (std::nothrow) NetworkClient) {
    iop_assert(client, IOP_STR("OOM"));
}

Wifi::~Wifi() noexcept {
    delete static_cast<NetworkClient*>(this->client);
}

static std::function<void()> onConnectCallbackStorage;
void onConnectCallback(arduino_event_id_t event) {
    (void) event;
    if (onConnectCallbackStorage) {
        onConnectCallbackStorage();
    }
}

void Wifi::onConnect(std::function<void()> f) noexcept {
  onConnectCallbackStorage = std::move(f);
  ::WiFi.onEvent(onConnectCallback, ARDUINO_EVENT_ETH_GOT_IP);
}

Wifi::Wifi(Wifi &&other) noexcept: client(other.client) {
    other.client = nullptr;
}

auto Wifi::operator=(Wifi &&other) noexcept -> Wifi & {
    this->client = other.client;
    other.client = nullptr;
    return *this;
}

StationStatus Wifi::status() const noexcept {
    const auto s = ::WiFi.status();
    switch (s) {
        case WL_IDLE_STATUS:
        case WL_SCAN_COMPLETED:
            return StationStatus::IDLE;
        case WL_NO_SSID_AVAIL:
            return StationStatus::NO_AP_FOUND;
        case WL_CONNECT_FAILED:
        case WL_CONNECTION_LOST:
        case WL_DISCONNECTED:
            return StationStatus::CONNECT_FAIL;
        case WL_CONNECTED:
            return StationStatus::GOT_IP;
        case WL_NO_SHIELD:
            return StationStatus::IDLE;
    }
    iop_panic(IOP_STR("Unreachable status: ").toString() + std::to_string(static_cast<uint8_t>(s)));
}

std::string Wifi::ourAccessPointIp() const noexcept {
    return ::WiFi.softAPIP().toString().c_str();
}

void Wifi::setMode(WiFiMode mode) const noexcept {
    switch (mode) {
        case WiFiMode::OFF:
            ::WiFi.mode(WIFI_OFF);
            return;
        case WiFiMode::STATION:
            ::WiFi.mode(WIFI_STA);
            return;
        case WiFiMode::ACCESS_POINT:
            ::WiFi.mode(WIFI_AP);
            return;
        case WiFiMode::ACCESS_POINT_AND_STATION:
            ::WiFi.mode(WIFI_AP_STA);
            return;
    }
    iop_panic(IOP_STR("Unreachable"));
}

void Wifi::disconnectFromAccessPoint() const noexcept {
    IOP_TRACE()
    portDISABLE_INTERRUPTS();
    ::WiFi.disconnect();
    portENABLE_INTERRUPTS();
}

std::pair<iop::NetworkName, iop::NetworkPassword> Wifi::credentials() const noexcept {
    IOP_TRACE()

    auto ssid = iop::NetworkName();
    ssid.fill('\0');
    std::strncpy(ssid.data(), ::WiFi.SSID().c_str(), sizeof(ssid));

    auto psk = iop::NetworkPassword();
    psk.fill('\0');
    std::strncpy(psk.data(), ::WiFi.psk().c_str(), sizeof(psk));

    return std::make_pair(ssid, psk);
}

void Wifi::setup() noexcept {
  iop_assert(this->client, IOP_STR("Wifi has been moved out, client is nullptr"));

  #ifdef IOP_SSL
  iop_assert(generated::certs_bundle, IOP_STR("Cert Bundle is null, but SSL is enabled"));
  static_cast<NetworkClient*>(this->client)->setCACertBundle(generated::certs_bundle);
  #endif

  ::WiFi.persistent(false);
  ::WiFi.setAutoReconnect(false);
  ::WiFi.setAutoConnect(false);

  this->disconnectFromAccessPoint();
  ::WiFi.mode(WIFI_STA);
  iop_hal::thisThread.sleep(1);
}

void Wifi::enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
    ::WiFi.mode(WIFI_AP_STA);
    iop_hal::thisThread.sleep(1);

    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto staticIp = IPAddress(192, 168, 1, 1);
    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto mask = IPAddress(255, 255, 255, 0);
    ::WiFi.softAPConfig(staticIp, staticIp, mask);

    String ssidStr;
    ssidStr.concat(ssid.begin(), std::min(ssid.length(), static_cast<size_t>(31)));
    String pskStr;
    pskStr.concat(psk.begin(), std::min(psk.length(), static_cast<size_t>(63)));
    ::WiFi.softAP(ssidStr, pskStr);
}

auto Wifi::disableOurAccessPoint() const noexcept -> void {
    ::WiFi.softAPdisconnect();
    ::WiFi.disconnect();
    ::WiFi.mode(WIFI_STA);
    iop_hal::thisThread.sleep(1);
}

bool Wifi::connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
    IOP_TRACE();
    ::WiFi.begin(std::string(ssid).c_str(), std::string(psk).c_str());

    return ::WiFi.waitForConnectResult() != -1;
}
}