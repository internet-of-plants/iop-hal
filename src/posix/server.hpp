#include "iop-hal/server.hpp"
#include "iop-hal/log.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/panic.hpp"

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <errno.h>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

namespace iop_hal {
iop::Log & logger() noexcept {
  static iop::Log logger_(IOP_STR("HTTP Server"));
  return logger_;
}
}

static ssize_t send(int fd, const char * msg, const size_t len) noexcept {
  if (iop_hal::logger().isTracing()) iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
  ssize_t sent = 0;
  uint64_t count = 0;
  while ((sent = write(fd, msg, len)) < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    iop_assert(count++ < 1000000, IOP_STR("Waited too long"));
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
  }
  iop_assert(sent > 0, std::string("Sent failed (") + std::to_string(sent) + ") [" + std::to_string(errno) + "] " +  strerror(errno) + ": " + msg);
  return sent;
}

static std::string httpCodeToString(const int code) {
  if (code == 200) {
    return "OK";
  } else if (code == 302) {
    return "Found";
  } else if (code == 404) {
    return "Not Found";
  } else {
    iop_panic(std::string("Http code not known: ") + std::to_string(code));
  }
}

namespace iop_hal {
HttpServer::HttpServer(const uint32_t port) noexcept: port(3000) {
  this->notFoundHandler = [](HttpConnection &conn, iop::Log const &logger) {
    conn.send(404, IOP_STR("text/plain"), IOP_STR("Not Found"));
    (void) logger;
  };
}
// We keep the server open because linux doesn't deal well with reuse and we close the server between iop credentials tests because some platforms depend on it
void HttpServer::begin() noexcept {
  IOP_TRACE();
  this->close();

  // Linux boilerplate
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(this->port));
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  int fd = 0;
  if (!this->maybeFD) {
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
      logger().errorln(IOP_STR("Unable to open socket"));
      return;
    }

    const char reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
      logger().errorln(IOP_STR("Unable to set SO_REUSEPORT to socket"));
      //return;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      logger().error(IOP_STR("fnctl get failed: "));
      logger().errorln(static_cast<uint64_t>(flags));
      return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
      logger().errorln(IOP_STR("fnctl set failed"));
      return;
    }
    this->maybeFD = fd;
    
    // Is this UB? (posix depends on casting struct into another one, but that's technically not allowed in C++)
    if (bind(fd, (struct sockaddr* )&addr, sizeof(addr)) < 0) {
      iop_panic(std::string("Unable to bind socket (") + std::to_string(errno) + "): " + strerror(errno));
      return;
    }
    if (listen(fd, 100) < 0) {
      logger().errorln(IOP_STR("Unable to listen socket"));
      return;
    }
  }

  logger().info(IOP_STR("Listening to port "));
  logger().infoln(static_cast<uint64_t>(this->port));

  this->address = new (std::nothrow) sockaddr_in(addr);
  iop_assert(this->address, IOP_STR("OOM"));
}

void HttpServer::handleClient() noexcept {
  IOP_TRACE();
  if (!this->address)
    return;

  iop_assert(!this->isHandlingRequest, IOP_STR("Already handling a request"));
  this->isHandlingRequest = true;

  iop_assert(this->maybeFD, IOP_STR("FD not found"));
  int fd = *this->maybeFD;

  auto addr = *this->address;
  socklen_t addr_len = sizeof(addr);

  HttpConnection conn;
  auto client = 0;
  if ((client = accept(fd, (sockaddr *)&addr, &addr_len)) <= 0) {
    if (client == 0) {
      logger().errorln(IOP_STR("Client fd is zero"));
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      logger().error(IOP_STR("Error accepting connection ("));
      logger().error(static_cast<uint64_t>(errno));
      logger().error(IOP_STR("): "));
      logger().errorln(std::string_view(strerror(errno)));
    }
    this->isHandlingRequest = false;
    return;
  }
  logger().debug(IOP_STR("Accepted connection: "));
  logger().debugln(static_cast<uint64_t>(client));
  conn.currentClient = client;

  auto firstLine = true;
  auto isPayload = false;

  auto buffer = HttpConnection::Buffer({0});
  ssize_t signedLen = 0;
  size_t len = 0;
  while (true) {
    logger().debug(IOP_STR("Try read: "));
    logger().debugln(static_cast<uint64_t>(strnlen(buffer.begin(), buffer.max_size())));

    if (len < buffer.max_size() &&
        (signedLen = read(client, buffer.data() + len, buffer.max_size() - len)) < 0) {
      logger().error(IOP_STR("Read error: "));
      logger().errorln(static_cast<uint64_t>(len));
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        iop_hal::thisThread.sleep(50);
        continue;
      } else {
        logger().error(IOP_STR("Error reading from socket ("));
        logger().error(static_cast<uint64_t>(signedLen));
        logger().error(IOP_STR("): "));
        logger().error(static_cast<uint64_t>(errno));
        logger().error(IOP_STR(" - "));
        logger().errorln(std::string_view(strerror(errno)));
        conn.reset();
        this->isHandlingRequest = false;
        return;
      }
    }
    len += static_cast<size_t>(signedLen);
    logger().debug(IOP_STR("Len: "));
    logger().debugln(static_cast<uint64_t>(len));

    if (firstLine && len == 0) {
      logger().errorln(IOP_STR("Empty request"));
      conn.reset();
      this->isHandlingRequest = false;
      return;
    }
    logger().debug(IOP_STR("Read: ("));
    logger().debug(static_cast<uint64_t>(len));
    logger().debugln(IOP_STR(")"));

    if (len == 0) {
      logger().debug(IOP_STR("EOF: "));
      logger().debugln(isPayload);
      break;
    }

    auto buff = std::string_view(buffer.begin(), len);
    if (len > 0 && firstLine) {
      if (buff.find("POST") != buff.npos) {
        const auto space = buff.substr(5).find(" ");
        conn.currentRoute = buff.substr(5, space);
        logger().debug(IOP_STR("POST: "));
        logger().debugln(conn.currentRoute);
      } else if (buff.find("GET") != buff.npos) {
        const auto space = buff.substr(4).find(" ");
        conn.currentRoute = buff.substr(4, space);
        logger().debug(IOP_STR("GET: "));
        logger().debugln(conn.currentRoute);
      } else if (buff.find("OPTIONS") != buff.npos) {
        const auto space = buff.substr(8).find(" ");
        conn.currentRoute = buff.substr(8, space);
        logger().debug(IOP_STR("OPTIONS: "));
        logger().debugln(conn.currentRoute);
      } else {
        logger().error(IOP_STR("HTTP Method not found: "));
        logger().errorln(buff);
        conn.reset();
        this->isHandlingRequest = false;
        return;
      }
      firstLine = false;

      iop_assert(buff.find("\r\n") != buff.npos, IOP_STR("First: ").toString() + std::to_string(buff.length()) + IOP_STR(" bytes don't contain newline, the path is too long\n").toString());
      logger().debugln(IOP_STR("Found first line"));
      const auto newlineIndex = buff.find("\r\n");
      len -= newlineIndex + 2;
      memmove(buffer.data(), buffer.begin() + newlineIndex + 2, len);
      buff = std::string_view(buffer.begin(), len);
    }
    logger().debug(IOP_STR("Headers + Payload: "));
    logger().debugln(buff);

    while (len > 0 && !isPayload) {
      const auto newlineIndex = buff.find("\r\n");
      // TODO: if empty line is split into two reads (because of buff len) we are screwed
      if (newlineIndex == buff.npos) {
        iop_panic(IOP_STR("Bad code"));
      } else if (newlineIndex != buff.npos && newlineIndex == buff.find("\r\n\r\n")) {
        isPayload = true;
        len -= newlineIndex + 4;
        memmove(buffer.data(), buffer.begin() + newlineIndex + 4, len);
        buff = std::string_view(buffer.begin(), len);
      } else {
        len -= newlineIndex + 2;
        memmove(buffer.data(), buffer.begin() + newlineIndex + 2, len);
        buff = std::string_view(buffer.begin(), len);
      }
    }

    logger().debug(IOP_STR("Payload ("));
    logger().debug(static_cast<uint64_t>(len));
    logger().debugln(IOP_STR(")"));

    conn.currentPayload += buff;

    logger().debug(IOP_STR("Route: "));
    logger().debugln(conn.currentRoute);
    iop::Log::shouldFlush(false);
    if (this->router.count(conn.currentRoute) != 0) {
      this->router.at(conn.currentRoute)(conn, logger());
    } else {
      logger().debugln(IOP_STR("Route not found"));
      this->notFoundHandler(conn, logger());
    }
    iop::Log::shouldFlush(true);
    logger().flush();
    break;
  }

  logger().debugln(IOP_STR("Close connection"));
  conn.reset();
  this->isHandlingRequest = false;
}
void HttpServer::close() noexcept {
  IOP_TRACE();
  if (this->address) {
    delete this->address;
    this->address = nullptr;
  }

  //if (this->maybeFD) ::close(*this->maybeFD);
  //this->maybeFD = std::nullopt;
}

void HttpServer::on(iop::StaticString uri, HttpServer::Callback handler) noexcept {
  this->router.emplace(uri.toString(), handler);
}
//called when handler is not assigned
void HttpServer::onNotFound(HttpServer::Callback fn) noexcept {
  this->notFoundHandler = fn;
}

static auto percentDecode(const std::string_view input) noexcept -> std::optional<std::string> {
  logger().debug(IOP_STR("Decode: "));
  logger().debugln(input);
  static const char tbl[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
  };
  std::string out;
  out.reserve(input.length());
  char c = '\0';
  const char *in = input.begin();
  while((c = *in++) && static_cast<size_t>(in - input.begin()) <= input.length()) {
    if(c == '%') {
      const auto v1 = tbl[(unsigned char)*in++];
      const auto v2 = tbl[(unsigned char)*in++];
      if(v1 < 0 || v2 < 0)
        return std::nullopt;
      c = static_cast<char>((v1 << 4) | v2);
    }
    out.push_back(c);
  }
  return out;
}

void HttpConnection::reset() noexcept {
  this->currentHeaders = "";
  this->currentPayload = "";
  this->currentContentLength.reset();
  if (this->currentClient) ::close(*this->currentClient);
  this->currentClient = std::nullopt;
}
auto HttpConnection::arg(const iop::StaticString name) const noexcept -> std::optional<std::string> {
  IOP_TRACE();
  std::string_view view(this->currentPayload);

  auto argEncodingLen = 1 + name.length(); // len(name) + len('=')
  auto index = view.find(name.toString() + "=");
  if (index != 0) index = view.find(std::string("&") + name.asCharPtr() + "=");
  if (index == view.npos) return std::nullopt;
  if (index != 0) argEncodingLen++; // + len('&')
  view = view.substr(index + argEncodingLen);

  const auto end = view.find("&");

  if (end == view.npos) {
    const auto decoded = percentDecode(view);
    logger().debugln(decoded.value_or("No value"));
    return decoded;
  }

  const auto msg = view.substr(0, end);
  const auto decoded = percentDecode(msg);
  logger().debugln(decoded.value_or("No value"));
  return decoded;
}

void HttpConnection::send(uint16_t code, iop::StaticString contentType, iop::StaticString content) const noexcept {
  IOP_TRACE();
  iop_assert(this->currentClient, IOP_STR("No active client"));
  const auto fd = *this->currentClient;

  const auto codeStr = std::to_string(code);
  const auto codeText = httpCodeToString(code);
  if (logger().isTracing())
    iop::Log::print(IOP_STR(""), iop::LogLevel::TRACE, iop::LogType::START);
  ::send(fd, "HTTP/1.0 ", 9);
  ::send(fd, codeStr.c_str(), codeStr.length());
  ::send(fd, " ", 1);
  ::send(fd, codeText.c_str(), codeText.length());
  ::send(fd, "\r\n", 2);
  ::send(fd, "Content-Type: ", 14);
  ::send(fd, contentType.asCharPtr(), contentType.length());
  ::send(fd, "; charset=ISO-8859-5\r\n", 22);
  if (this->currentHeaders.length() > 0) ::send(fd, this->currentHeaders.c_str(), this->currentHeaders.length());
  if (this->currentContentLength) {
    const auto contentLength = std::to_string(*this->currentContentLength);
    ::send(fd, "Content-Length: ", 16);
    ::send(fd, contentLength.c_str(), contentLength.length());
    ::send(fd, "\r\n", 2);
  }
  ::send(fd, "\r\n", 2);
  if (content.length() > 0) ::send(fd, content.asCharPtr(), content.length());

  if (logger().isTracing()) iop::Log::print(IOP_STR(""), iop::LogLevel::TRACE, iop::LogType::END);
}

void HttpConnection::setContentLength(const size_t contentLength) noexcept {
  IOP_TRACE();
  this->currentContentLength = contentLength;
}
void HttpConnection::sendHeader(const iop::StaticString name, const iop::StaticString value) noexcept{
  IOP_TRACE();
  this->currentHeaders += name.toString() + ": " + value.toString() + "\r\n";
}
void HttpConnection::sendData(iop::StaticString content) const noexcept {
  IOP_TRACE();
  logger().debug(IOP_STR("Send Content ("));
  logger().debug(content.length());
  logger().debug(IOP_STR("): "));
  logger().debugln(content);
  iop_assert(this->currentClient, IOP_STR("No active client"));

  if (logger().isTracing()) iop::Log::print(IOP_STR(""), iop::LogLevel::TRACE, iop::LogType::START);
  ::send(*this->currentClient, content.asCharPtr(), content.length());
  if (logger().isTracing()) iop::Log::print(IOP_STR(""), iop::LogLevel::TRACE, iop::LogType::END);
}

// NOOP
CaptivePortal::CaptivePortal(CaptivePortal &&other) noexcept { (void) other; }
auto CaptivePortal::operator=(CaptivePortal &&other) noexcept -> CaptivePortal & { (void) other; return *this; }
CaptivePortal::CaptivePortal() noexcept {}
CaptivePortal::~CaptivePortal() noexcept {}
void CaptivePortal::start() noexcept {}
void CaptivePortal::close() noexcept {}
void CaptivePortal::handleClient() const noexcept {}
}
