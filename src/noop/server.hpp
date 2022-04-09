#include "iop/server.hpp"
#include <optional>

namespace driver {
auto HttpConnection::arg(iop::StaticString arg) const noexcept -> std::optional<std::string> { (void) arg; return std::nullopt; }
void HttpConnection::sendHeader(iop::StaticString name, iop::StaticString value) noexcept { (void) name; (void) value; }
void HttpConnection::send(uint16_t code, iop::StaticString type, iop::StaticString data) const noexcept { (void) code; (void) type, (void) data; }
void HttpConnection::sendData(iop::StaticString data) const noexcept { (void) data; }
void HttpConnection::setContentLength(size_t length) noexcept { (void) length; }
void HttpConnection::reset() noexcept {}
HttpServer::HttpServer(uint32_t port) noexcept { (void) port; }
void HttpServer::begin() noexcept {}
void HttpServer::close() noexcept {}
void HttpServer::handleClient() noexcept {}
void HttpServer::on(iop::StaticString uri, Callback handler) noexcept { (void) uri; (void) handler; }
void HttpServer::onNotFound(Callback handler) noexcept { (void) handler; }
CaptivePortal::CaptivePortal(CaptivePortal &&other) noexcept { (void) other; }
auto CaptivePortal::operator=(CaptivePortal &&other) noexcept -> CaptivePortal & { (void) other; return *this; }
CaptivePortal::CaptivePortal() noexcept {}
CaptivePortal::~CaptivePortal() noexcept {}
void CaptivePortal::start() noexcept {}
void CaptivePortal::close() noexcept {}
void CaptivePortal::handleClient() const noexcept {}
}