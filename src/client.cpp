#ifdef IOP_LINUX_MOCK
#include "openssl/client.hpp"
#elif defined(IOP_ESP8266)
#include "esp/client.hpp"
#elif defined(IOP_ESP32)
#include "esp/client.hpp"
#elif defined(IOP_NOOP)
#include "noop/client.hpp"
#else
#error "Target not supported"
#endif