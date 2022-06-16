#ifdef IOP_POSIX_MOCK
#include "posix/client.hpp"
#elif defined(IOP_ESP8266)
#include "esp/client.hpp"
#elif defined(IOP_ESP32)
#include "esp/client.hpp"
#elif defined(IOP_NOOP)
#include "noop/client.hpp"
#else
#error "Target not supported"
#endif