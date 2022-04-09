#include "iop-hal/client.hpp"
#include "iop-hal/network.hpp"
#include "iop-hal/panic.hpp"
#include "iop-hal/wifi.hpp"

#include <system_error>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>
#include <stdio.h>
#include <stdlib.h>

// POSIX
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef IOP_SSL
#error "SSL not supported for desktop right now"
#endif

constexpr size_t bufferSize = 8192;

static iop::Log clientDriverLogger(iop::LogLevel::DEBUG, IOP_STR("HTTP Client"));

auto shiftChars(char* ptr, const size_t start, const size_t end) noexcept {
  //clientDriverLogger.debug(std::string_view(ptr).substr(start, end), std::to_string(start), " ", std::to_string(end));
  memmove(ptr, ptr + start, end);
  memset(ptr + end, '\0', bufferSize - end);
}

namespace iop {
auto Network::codeToString(const int code) const noexcept -> std::string {
  IOP_TRACE();
  switch (code) {
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

namespace iop_hal {
class SessionContext {
public:
  int fd;
  std::vector<std::string> headersToCollect;
  std::unordered_map<std::string, std::string> headers;
  std::string_view uri;

  SessionContext(int fd, std::vector<std::string> headersToCollect, std::string_view uri) noexcept: fd(fd), headersToCollect(headersToCollect), headers({}), uri(uri) {}

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
    return iop::NetworkStatus::BROKEN_SERVER;
  case 403:
    return iop::NetworkStatus::FORBIDDEN;
  }
  return std::nullopt;
}
HTTPClient::HTTPClient() noexcept: headersToCollect_() {}
HTTPClient::~HTTPClient() noexcept {}

static ssize_t send(int fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing()) iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  return write(fd, msg, len);
}

void HTTPClient::headersToCollect(std::vector<std::string> headers) noexcept {
  for (auto & key: headers) {
    // Headers can't be UTF8 so we cool
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
  }
  this->headersToCollect_ = std::move(headers);
}
auto Response::header(iop::StaticString key) const noexcept -> std::optional<std::string> {
  auto keyString = key.toString();
  // Headers can't be UTF8 so we cool
  std::transform(keyString.begin(), keyString.end(), keyString.begin(), [](unsigned char c){ return std::tolower(c); });
  if (this->headers_.count(keyString) == 0) return std::nullopt;
  return this->headers_.at(keyString);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept  {
  this->ctx.headers.emplace(key.toString(), value.toString());
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept  {
  this->ctx.headers.emplace(key.toString(), std::string(value));
}
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept  {
  this->ctx.headers.emplace(std::string(key), value.toString());
}
void Session::addHeader(std::string_view key, std::string_view value) noexcept  {
  this->ctx.headers.emplace(std::string(key), std::string(value));
}
void Session::setAuthorization(std::string auth) noexcept  {
  if (auth.length() == 0) return;
  this->ctx.headers.emplace(std::string("Authorization"), std::string("Basic ") + auth);
}

auto Session::sendRequest(const std::string method, const std::string_view data) noexcept -> Response {
  const auto len = data.length();

  if (iop::wifi.status() != iop_hal::StationStatus::GOT_IP)
    return Response(iop::NetworkStatus::IO_ERROR);

  auto responseHeaders = std::unordered_map<std::string, std::string>();
  auto responsePayload = std::vector<uint8_t>();
  auto status = std::make_optional(1000);

  responseHeaders.reserve(this->ctx.headersToCollect.size());

  {
    const auto path = std::string_view(this->ctx.uri.begin() + this->ctx.uri.find("/", this->ctx.uri.find("://") + 3));
    clientDriverLogger.debug(IOP_STR("Send request to "), path);

    const auto fd = this->ctx.fd;
    iop_assert(fd != -1, IOP_STR("Invalid file descriptor"));
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(IOP_STR(""), iop::LogLevel::TRACE, iop::LogType::START);
    send(fd, method.c_str(), method.length());
    send(fd, " ", 1);
    send(fd, path.begin(), path.length());
    send(fd, " HTTP/1.0\r\n", 11);
    send(fd, "Content-Length: ", 16);
    const auto dataLengthStr = std::to_string(len);
    send(fd, dataLengthStr.c_str(), dataLengthStr.length());
    send(fd, "\r\n", 2);
    for (const auto& [key, value]: this->ctx.headers) {
      send(fd, key.c_str(), key.length());
      send(fd, ": ", 2);
      send(fd, value.c_str(), value.length());
      send(fd, "\r\n", 2);
    }
    send(fd, "\r\n", 2);
    send(fd, data.begin(), len);
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(IOP_STR("\n"), iop::LogLevel::TRACE, iop::LogType::END);
    clientDriverLogger.debug(IOP_STR("Sent data: "), data);
    
    auto buffer = std::unique_ptr<char[]>(new (std::nothrow) char[bufferSize]);
    iop_assert(buffer, IOP_STR("OOM"));
    memset(buffer.get(), '\0', bufferSize);
    
    ssize_t signedSize = 0;
    size_t size = 0;
    auto firstLine = true;
    auto isPayload = false;
    
    std::string_view buff;
    while (true) {
      if (!isPayload) {
        //clientDriverLogger.debug(IOP_STR("Try read: "), buff);
      }

      if (size < bufferSize &&
          (signedSize = read(fd, buffer.get() + size, bufferSize - size)) < 0) {
        clientDriverLogger.error(IOP_STR("Error reading from socket ("), std::to_string(signedSize), IOP_STR("): "), std::to_string(errno), IOP_STR(" - "), strerror(errno)); 
        return Response(iop::NetworkStatus::IO_ERROR);
      }
      
      size += static_cast<size_t>(signedSize);

      clientDriverLogger.debug(IOP_STR("Len: "), std::to_string(size));
      if (firstLine && size == 0) {
        clientDriverLogger.warn(IOP_STR("Empty request: "), std::to_string(fd), IOP_STR(" "), std::to_string(size));
        return Response(status.value_or(500));
      }

      if (size == 0) {
        clientDriverLogger.debug(IOP_STR("EOF: "), std::to_string(isPayload));
        break;
      }

      buff = std::string_view(buffer.get(), size);

      if (!firstLine && !isPayload && buff.find("\r\n") == 0) {
          // TODO: move this logic to inside Response::await to be lazy-evaluated
          clientDriverLogger.debug(IOP_STR("Found Payload"));
          isPayload = true;

          clientDriverLogger.debug(std::to_string(size));
          size -= 2;
          shiftChars(buffer.get(), 2, size);
          buff = std::string_view(buffer.get(), size);
      }
      
      if (!isPayload) {
        clientDriverLogger.debug(IOP_STR("Buffer ["), std::to_string(size), IOP_STR("]: "), buff.substr(0, buff.find("\r\n") == buff.npos ? (size > 30 ? 30 : size) : buff.find("\r\n")));
      }
      clientDriverLogger.debug(IOP_STR("Is Payload: "), std::to_string(isPayload));

      if (!isPayload && buff.find("\n") == buff.npos) {
        iop_panic(IOP_STR("Header line used more than 4kb, currently we don't support this: ").toString() + buffer.get());
      }

      if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
        clientDriverLogger.error(IOP_STR("Error reading first line: "), std::to_string(size));
        return Response(iop::NetworkStatus::IO_ERROR);
      }

      if (firstLine && size > 0) {
        clientDriverLogger.debug(IOP_STR("Found first line"));

        const std::string_view statusStr(buffer.get() + 9); // len("HTTP/1.1 ") = 9
        const auto codeEnd = statusStr.find(" ");
        if (codeEnd == statusStr.npos) {
          clientDriverLogger.error(IOP_STR("Bad server: "), statusStr, IOP_STR(" -- "), buff);
          return Response(iop::NetworkStatus::IO_ERROR);
        }
        status = atoi(std::string(statusStr.begin(), 0, codeEnd).c_str());
        clientDriverLogger.debug(IOP_STR("Status: "), std::to_string(status.value_or(500)));
        firstLine = false;

        const auto newlineIndex = buff.find("\n") + 1;
        size -= newlineIndex;
        shiftChars(buffer.get(), newlineIndex, size);
        buff = std::string_view(buffer.get(), size);
      }
      if (!status) {
        clientDriverLogger.error(IOP_STR("No status"));
        return Response(iop::NetworkStatus::IO_ERROR);
      }
      if (!isPayload) {
        //clientDriverLogger.debug(IOP_STR("Buffer: "), buff.substr(0, buff.find("\r\n")));
      //if (!buff.contains(IOP_STR("\r\n"))) continue;
        //clientDriverLogger.debug(IOP_STR("Headers + Payload: "), buff.substr(0, buff.find("\r\n") == buff.npos ? size : buff.find("\r\n")));
      }

      while (size > 0 && !isPayload) {
        if (buff.find("\r\n") == 0) {
          size -= 2;
          shiftChars(buffer.get(), 2, size);
          buff = std::string_view(buffer.get(), size);
          
          clientDriverLogger.debug(IOP_STR("Found Payload ("), std::to_string(buff.find("\r\n") == buff.npos ? size + 2 : buff.find("\r\n") + 2), IOP_STR(")"));
          isPayload = true;
        } else if (buff.find("\r\n") != buff.npos) {
          clientDriverLogger.debug(IOP_STR("Found headers (buffer length: "), std::to_string(size), IOP_STR(")"));
          for (const auto &key: this->ctx.headersToCollect) {
            if (size < key.length() + 2) continue; // "\r\n"
            std::string headerKey(buff.substr(0, key.length()));
            // Headers can't be UTF8 so we cool
            std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
              [](unsigned char c){ return std::tolower(c); });

            clientDriverLogger.debug(headerKey, IOP_STR(" == "), key);
            if (headerKey != key.c_str())
              continue;

            auto valueView = buff.substr(key.length());
            if (*valueView.begin() == ':') valueView = valueView.substr(1);
            while (*valueView.begin() == ' ') valueView = valueView.substr(1);

            std::string value(valueView, 0, valueView.find("\r\n"));
            clientDriverLogger.debug(IOP_STR("Found header "), key, IOP_STR(" = "), value, IOP_STR("\n"));
            responseHeaders.emplace(key.c_str(), value);
          }
          clientDriverLogger.debug(IOP_STR("Buffer: "), buff.substr(0, buff.find("\r\n") == buff.npos ? (size > 30 ? 30 : size) : buff.find("\r\n")));
          clientDriverLogger.debug(IOP_STR("Skipping header ("), std::to_string(buff.find("\r\n") == buff.npos ? size : buff.find("\r\n") + 2), IOP_STR(")"));
          const auto startIndex = buff.find("\r\n") + 2;
          size -= startIndex;
          shiftChars(buffer.get(), startIndex, size);
          buff = std::string_view(buffer.get(), size);
        } else {
          clientDriverLogger.debug(IOP_STR("Header line too big"));
          return Response(iop::NetworkStatus::IO_ERROR);
        }
      }

      if (isPayload && size > 0) {
        clientDriverLogger.debug(IOP_STR("Appending payload: "), std::to_string(size), IOP_STR(" "), std::to_string(responsePayload.size()));
        responsePayload.insert(responsePayload.end(), buffer.get(), buffer.get() + size);
        memset(buffer.get(), '\0', bufferSize);
        buff = "";
        size = 0;
      }
    }
  }

  clientDriverLogger.debug(IOP_STR("Close client: "), std::to_string(this->ctx.fd));
  clientDriverLogger.debug(IOP_STR("Status: "), std::to_string(status.value_or(500)));
  iop_assert(status, IOP_STR("Status not available"));

  return Response(responseHeaders, Payload(responsePayload), *status);
}

auto HTTPClient::begin(std::string_view uri, std::function<Response(Session&)> func) noexcept -> Response {
  struct sockaddr_in serv_addr;
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(IOP_STR("Unable to open socket"));
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }

  iop_assert(uri.find("http://") == 0, IOP_STR("Protocol must be http (no SSL)"));
  uri = uri.substr(7);

  const auto portIndex = uri.find(IOP_STR(":").toString());
  uint16_t port = 443;
  if (portIndex != uri.npos) {
    auto end = uri.substr(portIndex + 1).find("/");
    if (end == uri.npos) end = uri.length();
    port = static_cast<uint16_t>(strtoul(std::string(uri.begin(), portIndex + 1, end).c_str(), nullptr, 10));
    if (port == 0) {
      clientDriverLogger.error(IOP_STR("Unable to parse port, broken server: "), uri);
      return iop_hal::Response(iop::NetworkStatus::BROKEN_SERVER);
    }
  }
  clientDriverLogger.debug(IOP_STR("Port: "), std::to_string(port));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
    clientDriverLogger.error(IOP_STR("Address not supported: "), host);
    return iop_hal::Response(iop::NetworkStatus::BROKEN_CLIENT);
  }

  auto connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(IOP_STR("Unable to connect: "), std::to_string(connection));
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }
  clientDriverLogger.debug(IOP_STR("Began connection: "), uri);
  auto ctx = SessionContext(fd, this->headersToCollect_, uri);
  auto session = Session(ctx);
  return func(session);
}
HTTPClient::HTTPClient(HTTPClient &&other) noexcept: headersToCollect_(std::move(other.headersToCollect_)) {}
auto HTTPClient::operator==(HTTPClient &&other) noexcept -> HTTPClient & {
  this->headersToCollect_ = std::move(other.headersToCollect_);
  return *this;
}
}