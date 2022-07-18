#include "iop-hal/wifi.hpp"
#include "iop-hal/panic.hpp"
#include "iop-hal/thread.hpp"

#include "lwip/dns.h"
#include "ESP8266WiFi.h"
#include "arduino/esp/esp8266/generated/certificates.hpp"
#ifdef IOP_SSL
using NetworkClient = BearSSL::WiFiClientSecure;
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

void Wifi::onConnect(std::function<void()> f) noexcept {
  ::WiFi.onStationModeGotIP([f](const ::WiFiEventStationModeGotIP &ev) {
      (void) ev;
      f();
  });
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
    const auto s = wifi_station_get_connect_status();
    switch (static_cast<int>(s)) {
        case STATION_IDLE:
            return StationStatus::IDLE;
        case STATION_CONNECTING:
            return StationStatus::CONNECTING;
        case STATION_WRONG_PASSWORD:
            return StationStatus::WRONG_PASSWORD;
        case STATION_NO_AP_FOUND:
            return StationStatus::NO_AP_FOUND;
        case STATION_CONNECT_FAIL:
            return StationStatus::CONNECT_FAIL;
        case STATION_GOT_IP:
            return StationStatus::GOT_IP;
        case 255: // No idea what this is, but it's returned sometimes;
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
    ETS_UART_INTR_DISABLE(); // NOLINT hicpp-signed-bitwise
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE(); // NOLINT hicpp-signed-bitwise
}

std::pair<iop::NetworkName, iop::NetworkPassword> Wifi::credentials() const noexcept {
    IOP_TRACE()

    station_config config;
    memset(&config, '\0', sizeof(config));
    wifi_station_get_config(&config);

    auto ssid = iop::NetworkName();
    ssid.fill('\0');
    std::memcpy(ssid.data(), config.ssid, sizeof(config.ssid));

    auto psk = iop::NetworkPassword();
    psk.fill('\0');
    std::memcpy(psk.data(), config.password, sizeof(config.password));

    return std::make_pair(ssid, psk);
}

void Wifi::setup() noexcept {
  iop_assert(this->client, IOP_STR("Wifi has been moved out, client is nullptr"));

#ifdef IOP_SSL
  static iop_hal::CertStore certStore(generated::certList);
  static_cast<NetworkClient*>(this->client)->setCertStore(&certStore);
  #endif

  ::WiFi.persistent(false);
  ::WiFi.setAutoReconnect(false);
  ::WiFi.setAutoConnect(false);

  IPAddress dnsAddress { 1, 1, 1, 1 };
  dns_setserver(0, dnsAddress);

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
    ssidStr.concat(ssid.begin(), ssid.substr(0, 32).length());
    String pskStr;
    pskStr.concat(psk.begin(), psk.substr(0, 64).length());
    ::WiFi.softAP(ssidStr, pskStr);
}

auto Wifi::disableOurAccessPoint() const noexcept -> void {
    ::WiFi.softAPdisconnect();
    ::WiFi.disconnect();
    ::WiFi.mode(WIFI_STA);
    iop_hal::thisThread.sleep(1);
}

bool Wifi::connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
    if (wifi_station_get_connect_status() == STATION_CONNECTING) {
        this->disconnectFromAccessPoint();
    }

    String ssidStr;
    ssidStr.concat(ssid.begin(), ssid.length());
    String pskStr;
    pskStr.concat(psk.begin(), psk.length());
    ::WiFi.begin(ssidStr, pskStr);

    return ::WiFi.waitForConnectResult() != -1;
}
}
