#include "iop/upgrade.hpp"
#include "iop/network.hpp"

namespace driver {
auto Upgrade::run(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> driver::UpgradeStatus {
    (void) network;
    (void) path;
    (void) authorization_header;
    return driver::UpgradeStatus::NO_UPGRADE;
}
} // namespace driver