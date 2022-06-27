#include "iop-hal/update.hpp"
#include "iop-hal/network.hpp"

namespace iop_hal {
auto Update::run(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop_hal::UpdateStatus {
    (void) network;
    (void) path;
    (void) authorization_header;
    return iop_hal::UpdateStatus::NO_UPGRADE;
}
} // namespace iop_hal