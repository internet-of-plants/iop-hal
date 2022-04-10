#include "iop-hal/network.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/upgrade.hpp"
#include "iop-hal/panic.hpp"

constexpr static iop::UpgradeHook defaultHook(iop::UpgradeHook::defaultHook);

static iop::UpgradeHook hook(defaultHook);

namespace iop {
iop_hal::Wifi wifi;

auto Network::logger() const noexcept -> const Log & {
  return this->logger_;
}
auto Network::upgrade(StaticString path, std::string_view token) const noexcept -> iop_hal::UpgradeStatus {
  return iop_hal::Upgrade::run(*this, path, token);
}
void UpgradeHook::defaultHook() noexcept { IOP_TRACE(); }
void Network::setUpgradeHook(UpgradeHook scheduler) noexcept {
  hook = std::move(scheduler);
}
auto Network::takeUpgradeHook() noexcept -> UpgradeHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}

auto Network::isConnected() noexcept -> bool {
  return iop::wifi.status() == iop_hal::StationStatus::GOT_IP;
}

void Network::disconnect() noexcept {
  IOP_TRACE();
  return iop::wifi.disconnectFromAccessPoint();
}

auto Network::httpPost(std::string_view token, const StaticString path, std::string_view data) const noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::POST, token, path, data);
}

auto Network::httpPost(StaticString path, std::string_view data) const noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::POST, std::nullopt, path, data);
}

auto Network::httpGet(StaticString path, std::string_view token, std::string_view data) const noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::GET, token, path, data);
}

Network::Network(StaticString uri, const LogLevel &logLevel) noexcept
  : logger_(logLevel, IOP_STR("NETWORK")), uri_(uri) {
  IOP_TRACE();
}
}

#if defined(IOP_NOOP)
#include "noop/network.hpp"
#else

#include "iop-hal/wifi.hpp"
#include "iop-hal/device.hpp"
#include "iop-hal/client.hpp"
#include "iop-hal/panic.hpp"
#include "string.h"

static iop_hal::HTTPClient http;

namespace iop {
static auto methodToString(const HttpMethod &method) noexcept -> StaticString;

// Returns Response if it can understand what the server sent, int is the raw
// response given by ESP8266HTTPClient
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<std::string_view> &token, StaticString path,
                          const std::optional<std::string_view> &data) const noexcept
    -> iop_hal::Response {
  IOP_TRACE();
  Network::setup();

  const auto uri = this->uri().toString() + path.toString();
  const auto method = methodToString(method_);

  const auto data_ = data.value_or(std::string_view());

  this->logger().debug(method, IOP_STR(" to "), this->uri(), path, IOP_STR(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at debug because of that
  if (data)
    this->logger().debug(*data);
  
  this->logger().debug(IOP_STR("Begin"));
  const auto func = [this, token, data, data_, method](iop_hal::Session & session) {
    this->logger().trace(IOP_STR("Began HTTP connection"));

    if (token) {
      session.setAuthorization(std::string(*token));
    }

    // Currently only JSON is supported
    if (data)
      session.addHeader(IOP_STR("Content-Type"), IOP_STR("application/json"));

    // Authentication headers, identifies device and detects updates, perf
    // monitoring
    {
      auto str = iop::to_view(iop_hal::device.firmwareMD5());
      session.addHeader(IOP_STR("VERSION"), str);
      session.addHeader(IOP_STR("x-ESP8266-sketch-md5"), str);

      str = iop::to_view(iop_hal::device.macAddress());
      session.addHeader(IOP_STR("MAC_ADDRESS"), str);
    }
    
    {
      // Could this cause memory fragmentation?
      auto memory = iop_hal::thisThread.availableMemory();
      
      session.addHeader(IOP_STR("FREE_STACK"), std::to_string(memory.availableStack));

      for (const auto & item: memory.availableHeap) {
        session.addHeader(IOP_STR("FREE_").toString().append(item.first), std::to_string(item.second));
      }
      for (const auto & item: memory.biggestHeapBlock) {
        session.addHeader(IOP_STR("BIGGEST_BLOCK_").toString().append(item.first), std::to_string(item.second));
      }
    }
    session.addHeader(IOP_STR("VCC"), std::to_string(iop_hal::device.vcc()));
    session.addHeader(IOP_STR("TIME_RUNNING"), std::to_string(iop_hal::thisThread.timeRunning()));
    session.addHeader(IOP_STR("ORIGIN"), this->uri());
    session.addHeader(IOP_STR("DRIVER"), iop_hal::device.platform());

    this->logger().debug(IOP_STR("Making HTTP request"));

    auto response = session.sendRequest(method.toString(), data_);

    const auto status = response.status();
    if (!status) {
      return iop_hal::Response(response.code());
    }
  
    this->logger().debug(IOP_STR("Made HTTP request")); 

    // Handle system upgrade request
    const auto upgrade = response.header(IOP_STR("LATEST_VERSION"));
    if (upgrade && upgrade != iop::to_view(iop_hal::device.firmwareMD5())) {
      this->logger().info(IOP_STR("Scheduled upgrade"));
      hook.schedule();
    }
  
    this->logger().debug(IOP_STR("Response code: "), iop::to_view(this->codeToString(response.code())));

    // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
    /*
    constexpr const int32_t maxPayloadSizeAcceptable = 2048;
    if (globalData.http().getSize() > maxPayloadSizeAcceptable) {
      globalData.http().end();
      const auto lengthStr = std::to_string(response.payload.length());
      this->logger().error(IOP_STR("Payload from server was too big: "), lengthStr);
      globalData.response() = iop_hal::Response(NetworkStatus::BROKEN_SERVER);
      return globalData.response();
    }
    */

    // We have to simplify the errors reported by this API (but they are logged)
    if (*status == iop::NetworkStatus::OK) {
      // The payload is always downloaded, since we check for its size and the
      // origin is trusted. If it's there it's supposed to be there.
      const auto payload = std::move(response.await().payload);
      this->logger().debug(IOP_STR("Payload (") , iop::to_view(std::to_string(payload.size())), IOP_STR("): "), iop::to_view(iop::scapeNonPrintable(iop::to_view(payload).substr(0, payload.size() > 30 ? 30 : payload.size()))));
      return iop_hal::Response(iop_hal::Payload(payload), *status);
    }
    return iop_hal::Response(response.code());
  };
  return http.begin(uri, func);
}

void Network::setup() const noexcept {
  IOP_TRACE();
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  http.headersToCollect({"LATEST_VERSION"});

  iop::wifi.setup();
}

static auto methodToString(const HttpMethod &method) noexcept -> StaticString {
  IOP_TRACE();
  switch (method) {
  case HttpMethod::GET:
    return IOP_STR("GET");

  case HttpMethod::HEAD:
    return IOP_STR("HEAD");

  case HttpMethod::POST:
    return IOP_STR("POST");

  case HttpMethod::PUT:
    return IOP_STR("PUT");

  case HttpMethod::PATCH:
    return IOP_STR("PATCH");

  case HttpMethod::DELETE:
    return IOP_STR("DELETE");

  case HttpMethod::CONNECT:
    return IOP_STR("CONNECT");
    
  case HttpMethod::OPTIONS:
    return IOP_STR("OPTIONS");
  }
  iop_panic(IOP_STR("HTTP Method not found"));
}
} // namespace iop
#endif