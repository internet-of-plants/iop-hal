#ifdef IOP_POSIX
#include "iop/posix/client.hpp"
#elif defined(IOP_ESP8266)
#include "iop/esp/client.hpp"
#elif defined(IOP_ESP32)
#include "iop/esp/client.hpp"
#elif defined(IOP_NOOP)
#include "iop/noop/client.hpp"
#else
#error "Target not supported"
#endif