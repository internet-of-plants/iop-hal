#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#ifndef UNIT_TEST
#include "posix/runtime.hpp"
#endif
#elif defined(IOP_ESP8266)
#include "arduino/runtime.hpp"
#elif defined(IOP_ESP32)
#include "arduino/runtime.hpp"
#elif defined(IOP_NOOP)
#ifdef ARDUINO
#include "arduino/runtime.hpp"
#else
#ifndef UNIT_TEST
#include "cpp17/runtime.hpp"
#endif
#endif
#else
#error "Target not supported"
#endif