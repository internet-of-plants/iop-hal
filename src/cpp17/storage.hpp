#include "iop-hal/storage.hpp"
#include "iop-hal/panic.hpp"

#include <fstream>

namespace iop_hal {
auto Storage::setup(uintmax_t size) noexcept -> bool {
    // TODO: properly log errors
    iop_assert(size > 0, IOP_STR("Storage size is zero"));

    if (!this->buffer) {
        this->buffer = new (std::nothrow) uint8_t[size];
        if (!this->buffer) return false;
        this->size = size;
        std::memset(this->buffer, '\0', size);
    }

    std::ifstream file("eeprom.dat");
    std::cout << file.is_open() << std::endl;
    if (!file.is_open()) return false;

    std::cout << size << std::endl;
    file.read((char*) buffer, static_cast<std::streamsize>(size));
    if (file.fail()) return false;

    file.close();
    return !file.fail();
}
auto Storage::commit() noexcept -> bool {
    // TODO: properly log errors
    iop_assert(this->buffer, IOP_STR("Unable to allocate storage"));
    std::ofstream file("eeprom.dat");
    if (!file.is_open()) return false;

    file.write((char*) buffer, static_cast<std::streamsize>(size));
    if (file.fail()) return false;

    file.close();
    return !file.fail();
}
auto Storage::asRef() const noexcept -> uint8_t const * {
    iop_assert(this->buffer, IOP_STR("Allocation failed"));
    return this->buffer;
}
auto Storage::asMut() noexcept -> uint8_t * {
    iop_assert(this->buffer, IOP_STR("Allocation failed"));
    return this->buffer;
}
}