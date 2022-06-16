#ifdef IOP_POSIX_MOCK
#include "posix/server.hpp"
#elif defined(IOP_ESP8266)
#include "esp/server.hpp"
#elif defined(IOP_ESP32)
#include "esp/server.hpp"
#elif defined(IOP_NOOP)
#include "noop/server.hpp"
#else
echo "Target not supported"
#endif