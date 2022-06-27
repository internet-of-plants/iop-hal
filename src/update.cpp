#ifdef IOP_POSIX_MOCK
#include "cpp17/update.hpp"
#elif defined(IOP_ESP8266)
#include "esp/update.hpp"
#elif defined(IOP_ESP32)
#include "esp/update.hpp"
#elif defined(IOP_NOOP)
#include "noop/update.hpp"
#else
#error "Target not supported"
#endif