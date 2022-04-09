#include "iop-hal/upgrade.hpp"
#include "iop-hal/network.hpp"

namespace iop_hal {
auto Upgrade::run(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop_hal::UpgradeStatus {
    (void) network;
    (void) path;
    (void) authorization_header;
    return iop_hal::UpgradeStatus::NO_UPGRADE;
}
} // namespace iop_hal