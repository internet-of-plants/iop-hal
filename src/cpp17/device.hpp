#include "iop-hal/device.hpp"
#include "iop-hal/panic.hpp"
#include "cpp17/runtime_metadata.hpp"
#include "cpp17/md5.hpp"

#include <thread>
#include <filesystem>
#include <fstream>

namespace iop_hal {
auto Device::availableStorage() const noexcept -> uintmax_t {
  // TODO: handle errors
  std::error_code code;
  auto available = std::filesystem::space(std::filesystem::current_path(), code).available;
  if (code) return 0;
  return available;
}

void Device::deepSleep(uintmax_t seconds) const noexcept {
  if (seconds == 0) seconds = UINTMAX_MAX;
  // Note: this only sleeps our current thread, but we currently don't use multiple threads on posix anyway
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

iop::MD5Hash & Device::firmwareMD5() const noexcept {
  static iop::MD5Hash hash;
  static bool cached = false;
  if (cached)
    return hash;
  hash.fill('\0');
  
  const auto filename = std::filesystem::current_path().append(iop_hal::execution_path());
  std::ifstream file(filename);
  iop_assert(file.is_open(), IOP_STR("Unable to open firmware file"));

  const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  iop_assert(!file.fail(), IOP_STR("Unable to read from firmware file"));

  file.close();
  iop_assert(!file.fail(), IOP_STR("Unable to close firmware file"));

  std::array<uint8_t, 16> buff;
  MD5_CTX md5;
  MD5_Init(&md5);
  MD5_Update(&md5, &data.front(), data.size());
  MD5_Final(buff.data(), &md5);
  for (uint8_t i = 0; i < 16; i++){
    sprintf(hash.data() + (i * 2), "%02X", buff[i]);
  }

  cached = true;
  return hash;
}
}