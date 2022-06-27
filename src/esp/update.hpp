#include "iop-hal/update.hpp"
#include "iop-hal/client.hpp"
#include "iop-hal/panic.hpp"
#include "iop-hal/network.hpp"

#include <memory>

#ifdef IOP_ESP8266
#include "ESP8266WiFi.h"
#ifdef IOP_SSL
using NetworkClient = BearSSL::WiFiClientSecure;
#else
using NetworkClient = WiFiClient;
#endif
#include "ESP8266httpUpdate.h"
using HTTPUpdate = ESP8266HTTPUpdate;
#elif defined(IOP_ESP32)
#include "HTTPUpdate.h"
using NetworkClient = WiFiClient;
#else
#error "Non supported arduino based update device"
#endif


namespace iop_hal {
auto Update::run(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop_hal::UpdateStatus {
  auto *client = static_cast<NetworkClient*>(iop::wifi.client);
  iop_assert(client, IOP_STR("Wifi has been moved out, client is nullptr"));

  auto route = String();
  route.concat(network.uri().toString().c_str(), network.uri().length());
  route.concat(path.toString().c_str(), path.length());

  auto ESPhttpUpdate = std::unique_ptr<HTTPUpdate>(new (std::nothrow) HTTPUpdate());
  iop_assert(ESPhttpUpdate, IOP_STR("Unable to allocate HTTPUpdate"));
  ESPhttpUpdate->rebootOnUpdate(true);

#ifdef IOP_ESP8266
  //ESPhttpUpdate->setLedPin(LED_BUILTIN);
  ESPhttpUpdate->setAuthorization(std::string(authorization_header).c_str());
  ESPhttpUpdate->closeConnectionsOnUpdate(true);
  const auto result = ESPhttpUpdate->update(*client, route, "");
#elif defined(IOP_ESP32)
  ::HTTPClient http;
  if (!http.begin(*client, route)) {
    return iop_hal::UpdateStatus::IO_ERROR;
  }
  http.setAuthorization(std::string(authorization_header).c_str());
  const auto result = ESPhttpUpdate->update(http, "");
#else
  #error "Non supported arduino based update device"
#endif

switch (result) {
  case HTTP_UPDATE_NO_UPDATES:
  case HTTP_UPDATE_OK:
    return iop_hal::UpdateStatus::NO_UPGRADE;

  case HTTP_UPDATE_FAILED:
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    network.logger().error(IOP_STR("Update failed: "),
                       std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
    return iop_hal::UpdateStatus::BROKEN_SERVER;
}

// TODO(pc): properly handle ESPhttpUpdate.getLastError()
network.logger().error(IOP_STR("Update failed (UNKNOWN): "),
                    std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
return iop_hal::UpdateStatus::BROKEN_SERVER;
}
} // namespace iop_hal