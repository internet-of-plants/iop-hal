#ifdef IOP_POSIX_MOCK
#include "cpp17/upgrade.hpp"
#elif defined(IOP_ESP8266)
#include "esp/upgrade.hpp"
#elif defined(IOP_ESP32)
#include "esp/upgrade.hpp"
#elif defined(IOP_NOOP)
#include "noop/upgrade.hpp"
#else
#error "Target not supported"
#endif