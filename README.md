# iop-hal - IoP's Hardware Abstraction Layer

A C++17 framework that provides transparent/unified access to different hardwares, without exposing their inner workings.

Note: Posix target turns some functionalities into No-Ops, we intend to improve this, but some are naturally impossible to implement for this target. If some is needed you should make a speci fic target for that platform, as it surely will depend on things outside of the C++17 + posix API. The Posix target is mostly for ease of testing.

Provides the following functionalities:
- WiFi's Access Point and Station management
  - `#include <iop/wifi.hpp>`
  - `iop::Wifi`
- Firmware upgrade functionality
  - `#include <iop/upgrade.hpp>`
  - `iop::Upgrade`
- Thread management
  - `#include <iop/thread.hpp>`
  - `iop::Thread`
- Type-safe string abstractions and operations
  - `#include <iop/string.hpp>`
  - `iop::StaticString`
- Persistent storage access
  - `#include <iop/storage.hpp>`
  - `iop::Storage`
- HTTP server hosting
  - `#include <iop/server.hpp>`
  - `iop::HttpServer`
- Runtime hook API
  - `#include <iop/runtime.hpp>`
  - User defines `iop::setup` and `iop::login`
- Panic hook API (no exceptions supported, but we have our panic system)
  - `#include <iop/panic.hpp>`
  - `iop_panic` macro
- HTTPs client
  - `#include <iop/client.hpp>`
  - `iop::HttpClient`
- Higher level client to interface with IoP server, JSON + authentication + firmware upgrade management
  - `#include <iop/network.hpp>`
  - `iop::Network`
- Log system
  - `#include <iop/log.hpp>`
  - `iop::Log`
- Device primitives management
  - `#include <iop/device.hpp>`
  - `iop::Device`