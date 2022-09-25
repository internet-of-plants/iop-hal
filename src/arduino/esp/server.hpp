#include "iop-hal/server.hpp"
#include "iop-hal/panic.hpp"

#ifdef IOP_ESP8266
#include "ESP8266WebServer.h"
#include <user_interface.h>
using WebServer = ESP8266WebServer;
#elif defined(IOP_ESP32)
#include "WebServer.h"
#else
#error "Non supported arduino based server device"
#endif
#include "DNSServer.h"
#include <optional>

namespace iop_hal {
iop::Log & logger() noexcept {
  static iop::Log logger_(IOP_STR("HTTP Server"));
  return logger_;
}

auto to_server(void *ptr) noexcept -> WebServer & {
  return *reinterpret_cast<WebServer *>(ptr);
}

HttpConnection::HttpConnection(void * parent) noexcept: server(parent) {}
auto HttpConnection::arg(iop::StaticString arg) const noexcept -> std::optional<std::string> {
  if (!to_server(this->server).hasArg(arg.get())) return std::nullopt;
  return std::string(to_server(this->server).arg(arg.get()).c_str());
}
void HttpConnection::sendHeader(iop::StaticString name, iop::StaticString value) noexcept {
  to_server(this->server).sendHeader(String(name.get()), String(value.get()));
}
void HttpConnection::send(uint16_t code, iop::StaticString type, iop::StaticString data) const noexcept {
  to_server(this->server).send_P(code, type.asCharPtr(), data.asCharPtr());
}
void HttpConnection::sendData(iop::StaticString data) const noexcept {
  to_server(this->server).sendContent_P(data.asCharPtr());
}
void HttpConnection::setContentLength(size_t length) noexcept {
  to_server(this->server).setContentLength(length);
}
void HttpConnection::reset() noexcept {}

static uint32_t serverPort = 0;
HttpServer::HttpServer(uint32_t port) noexcept { IOP_TRACE(); serverPort = port; }

auto validateServer(void **ptr) noexcept -> WebServer & {
  if (!*ptr) {
    iop_assert(serverPort != 0, IOP_STR("Server port is not defined"));
    WebServer **s = reinterpret_cast<WebServer **>(ptr);
    *s = new (std::nothrow) WebServer(serverPort);
  }
  iop_assert(*ptr, IOP_STR("Unable to allocate WebServer"));
  return to_server(*ptr);
}
HttpConnection::HttpConnection(HttpConnection &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto HttpConnection::operator=(HttpConnection &&other) noexcept -> HttpConnection & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}
HttpConnection::~HttpConnection() noexcept {
  //delete reinterpret_cast<WebServer *>(this->server);
}
HttpServer::HttpServer(HttpServer &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto HttpServer::operator=(HttpServer &&other) noexcept -> HttpServer & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}
HttpServer::~HttpServer() noexcept {
  delete reinterpret_cast<WebServer *>(this->server);
}
void HttpServer::begin() noexcept { IOP_TRACE(); validateServer(&this->server).begin(); }
void HttpServer::close() noexcept { IOP_TRACE(); validateServer(&this->server).close(); }
void HttpServer::handleClient() noexcept {
  IOP_TRACE();
  iop_assert(!this->isHandlingRequest, IOP_STR("Already handling a client"));
  this->isHandlingRequest = true;
  validateServer(&this->server).handleClient();
  this->isHandlingRequest = false;
}
void HttpServer::on(iop::StaticString uri, Callback handler) noexcept {
  IOP_TRACE();
  auto *_server = &iop_hal::validateServer(&this->server);
  validateServer(&this->server).on(uri.get(), [handler, _server]() { HttpConnection conn(_server); handler(conn, logger()); });
}
void HttpServer::onNotFound(Callback handler) noexcept {
  IOP_TRACE();
  auto *_server = &iop_hal::validateServer(&this->server);
  validateServer(&this->server).onNotFound([handler, _server]() { HttpConnection conn(_server); handler(conn, logger()); });
}
CaptivePortal::CaptivePortal(CaptivePortal &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto CaptivePortal::operator=(CaptivePortal &&other) noexcept -> CaptivePortal & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}
CaptivePortal::CaptivePortal() noexcept: server(nullptr) {}
CaptivePortal::~CaptivePortal() noexcept {
  delete static_cast<DNSServer*>(this->server);
}
void CaptivePortal::start() noexcept {
  const uint16_t port = 53;
  this->server = new (std::nothrow) DNSServer();
  iop_assert(this->server, IOP_STR("Unable to allocate DNSServer"));
  static_cast<DNSServer*>(this->server)->setErrorReplyCode(DNSReplyCode::NoError);
  static_cast<DNSServer*>(this->server)->start(port, IOP_STR("*").get(), ::WiFi.softAPIP());
}
void CaptivePortal::close() noexcept {
  iop_assert(this->server, IOP_STR("Must initialize DNSServer first"));
  static_cast<DNSServer*>(this->server)->stop();
  this->server = nullptr;
}
void CaptivePortal::handleClient() const noexcept {
  IOP_TRACE();
  iop_assert(this->server, IOP_STR("Must initialize DNSServer first"));
  static_cast<DNSServer*>(this->server)->processNextRequest();
}
}
