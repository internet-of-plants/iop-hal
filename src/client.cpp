#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "openssl/client.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/esp/client.hpp"
#elif defined(IOP_ESP32)
#include "arduino/esp/client.hpp"
#elif defined(IOP_NOOP)
#include "noop/client.hpp"
#else
#error "Target not supported"
#endif