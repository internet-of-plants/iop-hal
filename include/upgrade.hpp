#ifndef IOP_DRIVER_UPGRADE_HPP
#define IOP_DRIVER_UPGRADE_HPP

#include "driver/string.hpp"

namespace iop {
    class Network;
}

namespace driver {
/// Higher level error reporting. Lower level is handled by core
enum class UpgradeStatus {
  IO_ERROR,
  BROKEN_SERVER,
  BROKEN_CLIENT,

  NO_UPGRADE,
  FORBIDDEN,
};
    
// Handles upgrade procedure, reseting the board on success.
//
// Downloads firmware from behind the monitor server's authentication
struct Upgrade {
    static auto run(const iop::Network &network, iop::StaticString path, std::string_view authorization_header) noexcept -> driver::UpgradeStatus;
};
}

#endif