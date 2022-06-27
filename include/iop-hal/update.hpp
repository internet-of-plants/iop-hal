#ifndef IOP_DRIVER_UPGRADE_HPP
#define IOP_DRIVER_UPGRADE_HPP

#include "iop-hal/string.hpp"

namespace iop {
    class Network;
}

namespace iop_hal {
/// Higher level error reporting. Lower level is handled by core
enum class UpdateStatus {
  IO_ERROR,
  BROKEN_SERVER,
  BROKEN_CLIENT,

  NO_UPGRADE,
  UNAUTHORIZED,
};
    
// Handles update procedure, reseting the board on success.
//
// Downloads firmware from behind the monitor server's authentication
struct Update {
    static auto run(const iop::Network &network, iop::StaticString path, std::string_view authorization_header) noexcept -> iop_hal::UpdateStatus;
};
}

#endif