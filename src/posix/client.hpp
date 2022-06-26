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
#include <netdb.h>

#ifdef IOP_SSL
#include <openssl/ssl.h>
#else
class SSL;
#endif

constexpr size_t bufferSize = 8192;

static iop::Log clientDriverLogger(iop::LogLevel::TRACE, IOP_STR("HTTP Client"));

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
  case 401:
    return IOP_STR("UNAUTHORIZED").toString();
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
  SSL *ssl;

  SessionContext(int fd, std::vector<std::string> headersToCollect, std::string_view uri, SSL *ssl) noexcept: fd(fd), headersToCollect(headersToCollect), headers({}), uri(uri), ssl(ssl) {}

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
  case 401:
    return iop::NetworkStatus::UNAUTHORIZED;
  }
  return std::nullopt;
}
HTTPClient::HTTPClient() noexcept: headersToCollect_() {}
HTTPClient::~HTTPClient() noexcept {}

static ssize_t send(const SessionContext &ctx, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing()) iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  #ifdef IOP_SSL
  if (ctx.ssl) {
    return SSL_write(ctx.ssl, msg, len);
  }
  #endif
  return write(ctx.fd, msg, len);
}
static ssize_t recv(const SessionContext &ctx, char *buffer, const size_t len) noexcept {
  #ifdef IOP_SSL
  if (ctx.ssl) {
    return SSL_read(ctx.ssl, buffer, len);
  }
  #endif
  return read(ctx.fd, buffer, len);
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
    send(this->ctx, method.c_str(), method.length());
    send(this->ctx, " ", 1);
    send(this->ctx, path.begin(), path.length());
    send(this->ctx, " HTTP/1.0\r\n", 11);
    send(this->ctx, "Content-Length: ", 16);
    const auto dataLengthStr = std::to_string(len);
    send(this->ctx, dataLengthStr.c_str(), dataLengthStr.length());
    send(this->ctx, "\r\n", 2);
    for (const auto& [key, value]: this->ctx.headers) {
      send(this->ctx, key.c_str(), key.length());
      send(this->ctx, ": ", 2);
      send(this->ctx, value.c_str(), value.length());
      send(this->ctx, "\r\n", 2);
    }
    send(this->ctx, "\r\n", 2);
    send(this->ctx, data.begin(), len);
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
          (signedSize = recv(this->ctx, buffer.get() + size, bufferSize - size)) < 0) {
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
  memset(&serv_addr, 0, sizeof(serv_addr));

  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(IOP_STR("Unable to open socket"));
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }

  const auto useTLS = uri.find("https://") == 0;
  if (useTLS) {
    #ifdef IOP_TLS
    clientDriverLogger.error(IOP_STR("Tried o make TLS connection but IOP_SSL is not defined"));
    return iop_hal::Response(iop::NetworkStatus::BROKEN_CLIENT);
    #else
    uri = uri.substr(8);
    #endif
  } else if (uri.find("http://") == 0) {
    uri = uri.substr(7);
  }

  const auto portIndex = uri.find(IOP_STR(":").toString());
  uint16_t port = 443;
  if (!useTLS) port = 80;

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

  // TODO: handle DNS resolution

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);

  struct hostent *he = gethostbyname(host.c_str());
  if (!he) {
    clientDriverLogger.error(IOP_STR("Unable to obtain hostent from host string: "), host);
    return iop_hal::Response(iop::NetworkStatus::BROKEN_CLIENT);
  }

  serv_addr.sin_addr= *(struct in_addr *) he->h_addr_list[0];
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  
#ifdef IOP_SSL
  SSL_CTX* context = nullptr;
  SSL* ssl = nullptr;
  if (useTLS) {
    context = SSL_CTX_new(TLS_client_method());
    if (!context) {
      clientDriverLogger.error(IOP_STR("Unable to allocate SSL context: "));
      return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
    }
  
    ssl = SSL_new(context);
    if (!ssl) {
      clientDriverLogger.error(IOP_STR("Unable to allocate SSL object: "));
      SSL_CTX_free(context);
      return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
    }
  }
#endif

  auto connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(IOP_STR("Unable to connect: "), std::to_string(connection));
    #ifdef IOP_SSL
    SSL_free(ssl);
    SSL_CTX_free(context);
    #endif
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }

#ifdef IOP_SSL
  if (useTLS && !SSL_set_fd(ssl, fd)) {
    clientDriverLogger.error(IOP_STR("Unable to set ssl fd"));
    SSL_free(ssl);
    SSL_CTX_free(context);
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }

  if (useTLS && !SSL_connect(ssl)) {
    clientDriverLogger.error(IOP_STR("Unable to connect SSL"));
    SSL_free(ssl);
    SSL_CTX_free(context);
    return iop_hal::Response(iop::NetworkStatus::IO_ERROR);
  }
  
  auto *cert_names = SSL_load_client_CA_file("./lib/iop-hal/build/ca-bundle.crt");
  iop_assert(cert_names != nullptr, IOP_STR("NULL"));
  SSL_CTX_set_client_CA_list(context, cert_names);

  SSL_set_tlsext_host_name(ssl, host.c_str());

  const auto verify = SSL_get_verify_result(ssl);
  if (verify != X509_V_OK) {
    clientDriverLogger.error(IOP_STR("Invalid SSL cert: "), std::to_string(verify));
    SSL_free(ssl);
    SSL_CTX_free(context);
    return iop_hal::Response(iop::NetworkStatus::BROKEN_SERVER);
  }

  auto *peer = SSL_get_peer_certificate(ssl);
  std::array<char, 256> peer_CN;
  peer_CN.fill('0');
  X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN.begin(), 256);
  if (host != iop::to_view(peer_CN)) {
    clientDriverLogger.error(IOP_STR("Invalid SSL cert host"));
    SSL_free(ssl);
    SSL_CTX_free(context);
    return iop_hal::Response(iop::NetworkStatus::BROKEN_SERVER);
  }
#endif

  clientDriverLogger.debug(IOP_STR("Began connection: "), uri);
  auto ctx = SessionContext(fd, this->headersToCollect_, uri, ssl);
  auto session = Session(ctx);
  auto result = func(session);

  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(context);
  close(fd);
  return result;
}
HTTPClient::HTTPClient(HTTPClient &&other) noexcept: headersToCollect_(std::move(other.headersToCollect_)) {}
auto HTTPClient::operator==(HTTPClient &&other) noexcept -> HTTPClient & {
  this->headersToCollect_ = std::move(other.headersToCollect_);
  return *this;
}
}