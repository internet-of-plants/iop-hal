#ifndef IOP_DRIVER_SERVER_HPP
#define IOP_DRIVER_SERVER_HPP

#include "iop-hal/log.hpp"
#include <functional>
#include <unordered_map>
#include <memory>

#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
class sockaddr_in;
#elif defined(IOP_ESP8266)
class DNSServer;
#elif defined(IOP_NOOP) ||  defined(IOP_ESP32)
#else
#error "Target not supported"
#endif

namespace iop_hal {
class HttpConnection {
// TODO: make variables private with setters/getters available to friend classes (make HttpServer a friend)
public:
#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
  std::optional<int32_t> currentClient;
  std::string currentHeaders;
  std::string currentPayload;
  std::optional<size_t> currentContentLength;
  std::string currentRoute;
  
  using Buffer = std::array<char, 1024>;
#elif defined(IOP_ESP8266) || defined(IOP_ESP32)
private:
  void *server; // ESP8266WebServer
public:
  ~HttpConnection() noexcept;
  HttpConnection(void *parent) noexcept;
  
  HttpConnection(HttpConnection &other) noexcept = delete;
  HttpConnection(HttpConnection &&other) noexcept;
  auto operator=(HttpConnection &other) noexcept -> HttpConnection & = delete;
  auto operator=(HttpConnection &&other) noexcept -> HttpConnection &;
#elif defined(IOP_NOOP)
#else
#error "Target not supported"
#endif

  auto arg(iop::StaticString arg) const noexcept -> std::optional<std::string>;
  void sendHeader(iop::StaticString name, iop::StaticString value) noexcept;

  // TODO: how can we represent that this moves the value out?
  // UNSAFE
  void send(uint16_t code, iop::StaticString type, iop::StaticString data) const noexcept;

  void sendData(iop::StaticString data) const noexcept;
  void setContentLength(size_t length) noexcept;
  void reset() noexcept;
};

class HttpServer {
  // TODO: currently we only support one client at a time, this is not thread safe tho
  bool isHandlingRequest = false;
public:
  using Callback = std::function<void(HttpConnection&, iop::Log const &)>;
#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
private:
  std::unordered_map<std::string, Callback> router;
  Callback notFoundHandler;
  uint32_t port;

  std::optional<int> maybeFD;
  sockaddr_in *address;
#elif defined(IOP_ESP8266) || defined(IOP_ESP32)
  void *server; // ESP8266WebServer
public:
  HttpServer(HttpServer &other) noexcept = delete;
  HttpServer(HttpServer &&other) noexcept;
  auto operator=(HttpServer &other) noexcept -> HttpServer & = delete;
  auto operator=(HttpServer &&other) noexcept -> HttpServer &;
  ~HttpServer() noexcept;
#elif defined(IOP_NOOP)
#else
#error "Target not supported"
#endif
public:
  HttpServer(uint32_t port = 80) noexcept;

  void begin() noexcept;
  void close() noexcept;
  void handleClient() noexcept;
  void on(iop::StaticString uri, Callback handler) noexcept;
  void onNotFound(Callback fn) noexcept;
};

class CaptivePortal {
  void *server;
public:
  CaptivePortal() noexcept;
  ~CaptivePortal() noexcept;

  CaptivePortal(CaptivePortal &other) noexcept = delete;
  CaptivePortal(CaptivePortal &&other) noexcept;
  auto operator=(CaptivePortal &other) noexcept -> CaptivePortal & = delete;
  auto operator=(CaptivePortal &&other) noexcept -> CaptivePortal &;

  void start() noexcept;
  void close() noexcept;  
  void handleClient() const noexcept;
};
}
#endif
