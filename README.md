# iop-hal - IoP's Hardware Abstraction Layer

A C++17 framework that provides transparent/unified access to different hardwares, without exposing their inner workings.

Note: Posix target turns some functionalities into No-Ops, we intend to improve this, but some are naturally impossible to implement for this target. If some is needed you should make a speci fic target for that platform, as it surely will depend on things outside of the C++17 + posix API. The Posix target is mostly for ease of testing.

Provides the following functionalities:
- WiFi's Access Point and Station management
  - `#include <iop-hal/wifi.hpp>`
  - `iop::Wifi`
- Firmware upgrade functionality
  - `#include <iop-hal/upgrade.hpp>`
  - `iop::Upgrade`
- Thread management
  - `#include <iop-hal/thread.hpp>`
  - `iop::Thread`
- Type-safe string abstractions and operations
  - `#include <iop-hal/string.hpp>`
  - `iop::StaticString`, `iop::CowString`
- Persistent storage access
  - `#include <iop-hal/storage.hpp>`
  - `iop::Storage`
- HTTP server hosting
  - `#include <iop-hal/server.hpp>`
  - `iop::HttpServer`
- Runtime hook API
  - `#include <iop-hal/runtime.hpp>`
  - User defines `iop::setup` and `iop::login`
- Panic hook API (no exceptions supported, but we have our panic system)
  - `#include <iop-hal/panic.hpp>`
  - `iop_panic` macro
- HTTPs client
  - `#include <iop-hal/client.hpp>`
  - `iop::HttpClient`
- Higher level client to interface with IoP server, JSON + authentication + firmware upgrade management
  - `#include <iop-hal/network.hpp>`
  - `iop::Network`
- Log system
  - `#include <iop-hal/log.hpp>`
  - `iop::Log`
- Device primitives management
  - `#include <iop-hal/device.hpp>`
  - `iop::Device`