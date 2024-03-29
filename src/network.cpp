#include "iop-hal/network.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/update.hpp"
#include "iop-hal/panic.hpp"

constexpr static iop::UpdateHook defaultHook(iop::UpdateHook::defaultHook);

static iop::UpdateHook hook(defaultHook);

namespace iop {
iop_hal::Wifi wifi;

auto Network::logger() noexcept -> Log & {
  return this->logger_;
}
auto Network::update(StaticString path, std::string_view token) noexcept -> iop_hal::UpdateStatus {
  return iop_hal::Update::run(*this, path, token);
}
void UpdateHook::defaultHook() noexcept { IOP_TRACE(); }
void Network::setUpdateHook(UpdateHook scheduler) noexcept {
  hook = std::move(scheduler);
}
auto Network::takeUpdateHook() noexcept -> UpdateHook {
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

auto Network::httpPost(std::string_view token, const StaticString path, std::string_view data) noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::POST, token, path, data);
}

auto Network::httpPost(StaticString path, std::string_view data) noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::POST, std::nullopt, path, data);
}

auto Network::httpGet(StaticString path, std::string_view token, std::string_view data) noexcept -> iop_hal::Response {
  return this->httpRequest(HttpMethod::GET, token, path, data);
}

Network::Network(StaticString uri) noexcept
  : logger_(IOP_STR("NETWORK")), uri_(uri) {
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

auto prepareSession(Network  &network, iop_hal::Session &session, const std::optional<std::string_view> &token, const std::optional<std::string_view> &data) noexcept -> void {
  network.logger().debugln(IOP_STR("Began HTTP connection"));

  if (token) {
    session.setAuthorization(std::string(*token));
  }

  // Currently only JSON is supported
  if (data) {
    session.addHeader(IOP_STR("Content-Type"), IOP_STR("application/json"));
  }

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
  session.addHeader(IOP_STR("ORIGIN"), network.uri());
  session.addHeader(IOP_STR("DRIVER"), iop_hal::device.platform());

  network.logger().debugln(IOP_STR("Making HTTP request"));
}

auto beforeConnect(Network & network, StaticString path, const std::optional<std::string_view> &token, const std::optional<std::string_view> &data, iop::StaticString method) noexcept -> void {
  Network::setup();

  if (token) {
    network.logger().debug(IOP_STR("Authenticated "));
  } else {
    network.logger().debug(IOP_STR("Unauthenticated "));
  }
  network.logger().debug(method);
  network.logger().debug(IOP_STR(" to "));
  network.logger().debug(network.uri());
  network.logger().debug(path);
  network.logger().debug(IOP_STR(", data length: "));
  network.logger().debugln(data.value_or(std::string_view()).length());

  // TODO: this may log sensitive information, network logging is currently
  // capped at info because of that
  if (data) {
    network.logger().debugln(*data);
  }

  network.logger().debugln(IOP_STR("Begin"));
}

auto processResponse(Network & network, iop_hal::Response & response) noexcept -> iop_hal::Response {
  const auto status = response.status();
  if (!status || *status == iop::NetworkStatus::IO_ERROR) {
    return iop_hal::Response(response.code());
  }

  network.logger().debugln(IOP_STR("Made HTTP request"));

  // Handle system update request
  const auto update = response.header(IOP_STR("LATEST_VERSION"));
  if (update && update != iop::to_view(iop_hal::device.firmwareMD5())) {
    network.logger().debugln(IOP_STR("Scheduled update"));
    hook.schedule();
  }

  auto code = network.codeToString(response.code());
  network.logger().debug(IOP_STR("Response code ("));
  network.logger().debug(code);
  network.logger().debug(IOP_STR("): "));
  network.logger().debugln(static_cast<uint64_t>(*status));

  // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
  /*
  constexpr const int32_t maxPayloadSizeAcceptable = 2048;
  if (globalData.http().getSize() > maxPayloadSizeAcceptable) {
    globalData.http().end();
    const auto lengthStr = std::to_string(response.payload.length());
    this->logger().error(IOP_STR("Payload from server was too big: "));
    this->logger().errorln(lengthStr);
    globalData.response() = iop_hal::Response(NetworkStatus::BROKEN_SERVER);
    return globalData.response();
  }
  */

  // We have to simplify the errors reported by this API (but they are logged)
  if (*status == iop::NetworkStatus::OK) {
    // The payload is always downloaded, since we check for its size and the
    // origin is trusted. If it's there it's supposed to be there.
    const auto payload = std::move(response.await().payload);
    network.logger().debug(IOP_STR("Payload ("));
    network.logger().debug(payload.size());
    network.logger().debug(IOP_STR("): "));
    network.logger().debugln(iop::scapeNonPrintable(iop::to_view(payload).substr(0, payload.size() > 30 ? 30 : payload.size())));
    return iop_hal::Response(iop_hal::Payload(payload), *status);
  }
  return iop_hal::Response(response.code());
}

auto generateRequestProcessor(Network * network, const std::optional<std::string_view> &token, const std::optional<std::string_view> &data, const StaticString method) noexcept -> std::function<iop_hal::Response (iop_hal::Session &)> {
  const auto func = [network, token, data, method](iop_hal::Session & session) {
    prepareSession(*network, session, token, data);
    auto response = session.sendRequest(method.toString(), data.value_or(std::string_view()));
    return processResponse(*network, response);
  };
  return func;
}

// Returns Response if it can understand what the server sent
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<std::string_view> &token, StaticString path,
                          const std::optional<std::string_view> &data) noexcept
    -> iop_hal::Response {
  IOP_TRACE();
  const auto method = methodToString(method_);
  beforeConnect(*this, path, token, data, method);
  const auto func = generateRequestProcessor(this, token, data, method);
  return http.begin(this->endpoint(path), func);
}

auto Network::setup() noexcept -> void {
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
