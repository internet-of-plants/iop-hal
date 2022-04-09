#include "iop/client.hpp"
#include "iop/panic.hpp"
#include "iop/network.hpp"

#include <charconv>
#include <system_error>

#ifdef IOP_ESP8266
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#ifdef IOP_SSL
using NetworkClient = BearSSL::WiFiClientSecure;
#else
using NetworkClient = WiFiClient;
#endif
#elif defined(IOP_ESP32)
#include "HTTPClient.h"
using NetworkClient = WiFiClient;
#else
#error "Non supported arduino based client device"
#endif

namespace iop {
auto Network::codeToString(const int code) const noexcept -> std::string {
  IOP_TRACE();
  switch (code) {
#ifdef IOP_ESP8266
  case HTTPC_ERROR_CONNECTION_FAILED:
    return IOP_STR("CONNECTION_FAILED").toString();
#elif defined(IOP_ESP32)
  case HTTPC_ERROR_CONNECTION_REFUSED:
    return IOP_STR("CONNECTION_FAILED").toString();
#endif
  case HTTPC_ERROR_TOO_LESS_RAM:
    return IOP_STR("LOW_RAM").toString();
  case HTTPC_ERROR_NOT_CONNECTED:
    return IOP_STR("NOT_CONNECTED").toString();
  case HTTPC_ERROR_CONNECTION_LOST:
    return IOP_STR("CONNECTION_LOST").toString();
  case HTTPC_ERROR_SEND_HEADER_FAILED:
    return IOP_STR("SEND_HEADER_FAILED").toString();
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
    return IOP_STR("SEND_PAYLOAD_FAILED").toString();
  case HTTPC_ERROR_NO_STREAM:
    return IOP_STR("NO_STREAM").toString();
  case HTTPC_ERROR_NO_HTTP_SERVER:
    return IOP_STR("NO_HTTP_SERVER").toString();
  case HTTPC_ERROR_ENCODING:
    return IOP_STR("UNSUPPORTED_ENCODING").toString();
  case HTTPC_ERROR_STREAM_WRITE:
    return IOP_STR("WRITE_ERROR").toString();
  case HTTPC_ERROR_READ_TIMEOUT:
    return IOP_STR("READ_TIMEOUT").toString();
  case 200:
    return IOP_STR("OK").toString();
  case 500:
    return IOP_STR("SERVER_ERROR").toString();
  case 403:
    return IOP_STR("FORBIDDEN").toString();
  }
  return std::to_string(code);
}
}

namespace driver{
class SessionContext {
public:
  HTTPClient & http;
  std::unordered_map<std::string, std::string> headers;
  std::string_view uri;

  SessionContext(HTTPClient &http, std::string_view uri) noexcept: http(http), headers({}), uri(uri) {}

  SessionContext(SessionContext &&other) noexcept = delete;
  SessionContext(const SessionContext &other) noexcept = delete;
  auto operator==(SessionContext &&other) noexcept -> SessionContext & = delete;
  auto operator==(const SessionContext &other) noexcept -> SessionContext & = delete;
  ~SessionContext() noexcept = default;
};

auto networkStatus(const int code) noexcept -> std::optional<iop::NetworkStatus> {
  IOP_TRACE();
  switch (code) {
  case 200:
    return iop::NetworkStatus::OK;
  case 500:
  case HTTPC_ERROR_NO_HTTP_SERVER:
  // Unsupported Transfer-Encoding header, if set it must be "chunked"
  case HTTPC_ERROR_ENCODING:
    return iop::NetworkStatus::BROKEN_SERVER;
  case 403:
    return iop::NetworkStatus::FORBIDDEN;
  case HTTPC_ERROR_TOO_LESS_RAM:
    return iop::NetworkStatus::BROKEN_CLIENT;


#ifdef IOP_ESP8266
  case HTTPC_ERROR_CONNECTION_FAILED:
#elif defined(IOP_ESP32)
  case HTTPC_ERROR_CONNECTION_REFUSED:
#endif
  case HTTPC_ERROR_SEND_HEADER_FAILED:
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
  case HTTPC_ERROR_NOT_CONNECTED:
  case HTTPC_ERROR_CONNECTION_LOST:
  case HTTPC_ERROR_NO_STREAM:
  case HTTPC_ERROR_STREAM_WRITE:
  case HTTPC_ERROR_READ_TIMEOUT:
    return iop::NetworkStatus::IO_ERROR;
  }
  return std::nullopt;
}

void HTTPClient::headersToCollect(std::vector<std::string> headers) noexcept {
  iop_assert(this->http, IOP_STR("HTTP client is nullptr"));
  std::vector<const char*> normalized;
  normalized.reserve(headers.size());
  for (const auto &str: headers) {
    normalized.push_back(str.c_str());
  }
  this->http->collectHeaders(normalized.data(), headers.size());
}
auto Response::header(iop::StaticString key) const noexcept -> std::optional<std::string> {
  const auto value = this->headers_.find(key.toString());
  if (value == this->headers_.end()) return std::nullopt;
  return std::move(value->second);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  this->ctx.http.http->addHeader(String(key.get()), String(value.get()));
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());
  this->ctx.http.http->addHeader(String(key.get()), val);
}
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  String header;
  header.concat(key.begin(), key.length());
  this->ctx.http.http->addHeader(header, String(value.get()));
}
void Session::addHeader(std::string_view key, std::string_view value) noexcept {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());

  String header;
  header.concat(key.begin(), key.length());
  this->ctx.http.http->addHeader(header, val);
}
void Session::setAuthorization(std::string auth) noexcept {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  this->ctx.http.http->setAuthorization(auth.c_str());
}
auto Session::sendRequest(const std::string method, const std::string_view data) noexcept -> Response {
  iop_assert(this->ctx.http.http, IOP_STR("Session has been moved out"));
  // ESP32 gets a uint8_t* but immediately converts it to immutable, so const cast here is needed
  const auto code = this->ctx.http.http->sendRequest(method.c_str(), reinterpret_cast<uint8_t*>(const_cast<char*>(data.begin())), data.length());
  if (code < 0) {
    return Response(code);
  }

  std::unordered_map<std::string, std::string> headers;
  headers.reserve(this->ctx.http.http->headers());
  for (int index = 0; index < this->ctx.http.http->headers(); ++index) {
    const auto key = std::string(this->ctx.http.http->headerName(index).c_str());
    const auto value = std::string(this->ctx.http.http->header(index).c_str());
    headers[key] = value;
  }

  const auto httpString = this->ctx.http.http->getString();
  auto storage = std::vector<uint8_t>();
  storage.insert(storage.end(), httpString.c_str(), httpString.c_str() + httpString.length());
  const auto payload = Payload(storage);
  const auto response = Response(headers, payload, code);
  return response;
}

HTTPClient::HTTPClient() noexcept: http(new (std::nothrow) ::HTTPClient()) {
  iop_assert(http, IOP_STR("OOM"));
}
HTTPClient::~HTTPClient() noexcept {
  delete this->http;
}

auto HTTPClient::begin(const std::string_view uri, std::function<Response(Session &)> func) noexcept -> Response {
  IOP_TRACE(); 

  iop_assert(this->http, IOP_STR("driver::HTTPClient* not allocated"));
  this->http->setTimeout(UINT16_MAX);
  this->http->setAuthorization("");

  // Parse URI
  auto index = uri.find("://");
  if (index == uri.npos) {
    index = 0;
  } else {
    index += 3;
  }

  auto host = std::string_view(uri).substr(index);
  host = host.substr(0, host.find('/'));

  auto portStr = std::string_view();
  index = host.find(':');

  if (index != host.npos) {
    portStr = host.substr(index + 1);
    host = host.substr(0, index);
  } else if (uri.find("http://") != uri.npos) {
    portStr = "80";
  } else if (uri.find("https://") != uri.npos) {
    portStr = "443";
  } else {
    iop_panic(IOP_STR("Protocol missing inside HttpClient::begin"));
  }

  uint16_t port;
  auto result = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
  if (result.ec != std::errc()) {
    iop_panic(IOP_STR("Unable to confert port to uint16_t: ").toString() + portStr.begin() + IOP_STR(" ").toString() + std::error_condition(result.ec).message());
  }

  iop_assert(iop::wifi.client, IOP_STR("Wifi has been moved out, client is nullptr"));
  iop_assert(this->http, IOP_STR("HTTP client is nullptr"));

  auto uriArduino = String();
  uriArduino.concat(uri.begin(), uri.length());
  if (this->http->begin(*static_cast<NetworkClient*>(iop::wifi.client), uriArduino)) {
    auto ctx = SessionContext(*this, uri);
    auto session = Session(ctx);
    const auto ret = func(session);
    this->http->end();
    return ret;
  }

  return driver::Response(iop::NetworkStatus::IO_ERROR);
}
HTTPClient::HTTPClient(HTTPClient &&other) noexcept: http(other.http) {
  other.http = nullptr;
}
auto HTTPClient::operator==(HTTPClient &&other) noexcept -> HTTPClient & {
  delete this->http;
  this->http = other.http;
  other.http = nullptr;
  return *this;
}
}