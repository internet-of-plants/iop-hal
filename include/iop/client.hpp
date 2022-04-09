#ifndef IOP_DRIVER_CLIENT_HPP
#define IOP_DRIVER_CLIENT_HPP

#include "iop/string.hpp"
#include "iop/response.hpp"
#include <functional>
#include <string>
#include <optional>
#include <memory>

class HTTPClient;

namespace driver {
class HTTPClient;

class SessionContext;

// References HTTPClient, should never outlive it
class Session {
  SessionContext &ctx;
  Session(SessionContext &context) noexcept: ctx(context) {}

  friend HTTPClient;

public:
  void addHeader(iop::StaticString key, iop::StaticString value) noexcept;
  void addHeader(iop::StaticString key, std::string_view value) noexcept;
  void addHeader(std::string_view key, iop::StaticString value) noexcept;
  void addHeader(std::string_view key, std::string_view value) noexcept;
  void setAuthorization(std::string auth) noexcept;
  // How to represent that this moves the server out
  auto sendRequest(std::string method, std::string_view data) noexcept -> Response;
  Session(Session &&other) noexcept = delete;
  Session(const Session &other) noexcept = delete;
  auto operator==(Session &&other) noexcept -> Session & = delete;
  auto operator==(const Session &other) noexcept -> Session & = delete;
  ~Session() noexcept = default;
};

class HTTPClient {
#ifdef IOP_POSIX
  std::vector<std::string> headersToCollect_;
#elif defined(IOP_ESP8266) || defined(IOP_ESP32)
  ::HTTPClient * http;
#elif defined(IOP_NOOP)
#else
#error "Target not valid"
#endif
public:
  HTTPClient() noexcept;
  ~HTTPClient() noexcept;
  auto begin(std::string_view uri, std::function<Response(Session &)> func) noexcept -> Response;
  auto headersToCollect(std::vector<std::string> headers) noexcept -> void;

  HTTPClient(HTTPClient &&other) noexcept;
  HTTPClient(const HTTPClient &other) noexcept = delete;
  auto operator==(HTTPClient &&other) noexcept -> HTTPClient &;
  auto operator==(const HTTPClient &other) noexcept -> HTTPClient & = delete;
  friend Session;
};
}

#endif