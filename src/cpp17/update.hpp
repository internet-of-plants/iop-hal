#include "iop-hal/update.hpp"
#include "iop-hal/network.hpp"
#include "cpp17/runtime_metadata.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace iop_hal {
auto Update::run(iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop_hal::UpdateStatus {
    auto response = network.httpGet(path, authorization_header, "");
    const auto status = response.status();
    if (status && *status == iop::NetworkStatus::OK) {
        const auto binary = response.await().payload;
        if (binary.size() == 0) {
            network.logger().errorln(IOP_STR("Update failed, no firmware returned"));
            return iop_hal::UpdateStatus::BROKEN_SERVER;
        }

        // Possible race here, but it's the best cross platform solution
        const std::string tmpFile = std::tmpnam(nullptr);
        network.logger().info(IOP_STR("Upgrading binary file: "));
        network.logger().infoln(tmpFile);

        std::ofstream file(tmpFile);
        if (!file.is_open()) {
            network.logger().errorln(IOP_STR("Unable to open firmware file"));
            return iop_hal::UpdateStatus::IO_ERROR;
        }

        file.write(reinterpret_cast<const char*>(&binary.front()), static_cast<std::streamsize>(binary.size()));
        if (file.fail()) {
            network.logger().errorln(IOP_STR("Unable to write to firmware file"));
            return iop_hal::UpdateStatus::IO_ERROR;
        }

        file.close();
        if (file.fail()) {
            network.logger().errorln(IOP_STR("Unable to close firmware file"));
            return iop_hal::UpdateStatus::IO_ERROR;
        }

        std::error_code code;
        // TODO: improve this perm
        std::filesystem::permissions(tmpFile, std::filesystem::perms::owner_all | std::filesystem::perms::others_exec | std::filesystem::perms::others_read | std::filesystem::perms::group_exec | std::filesystem::perms::group_read, code);
        if (code) {
            network.logger().error(IOP_STR("Unable to set file as executable: "));
            network.logger().errorln(code.value());
            return iop_hal::UpdateStatus::IO_ERROR;
        }

        // TODO FIXME: Race here
        const auto filename = std::filesystem::current_path().append(iop_hal::execution_path());
        const auto renamedBinary = std::filesystem::current_path().append(std::string(iop_hal::execution_path()).append(std::string_view(".old")));
        std::filesystem::rename(filename, renamedBinary);
        std::filesystem::rename(tmpFile, filename);
        std::filesystem::remove(renamedBinary);

        network.logger().infoln(IOP_STR("Upgrading runtime"));
        exit(system(filename.string().c_str()));
    } else {
        network.logger().error(IOP_STR("Invalid status returned by the server on update: "));
        network.logger().errorln(response.code());
    }

    return iop_hal::UpdateStatus::BROKEN_SERVER;
}
} // namespace iop_hal