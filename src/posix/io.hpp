#include "iop-hal/io.hpp"
#include "iop-hal/panic.hpp"

#include <thread>
#include <optional>
#include <string>
#include <memory>
#include <chrono>

#include <sstream>
#include <fstream>

namespace iop_hal {
namespace io {
auto GPIO::setMode(const PinRaw pin, const Mode mode) const noexcept -> void {
    const auto pinStr = std::to_string(pin);

    std::ofstream exported("/sys/class/gpio/export");
    iop_assert(exported, std::string("Unable to export GPIO: ") + pinStr);

    exported.write(pinStr.c_str(), pinStr.length());
    exported.close();

    std::stringstream directionPath;
    directionPath << "/sys/class/gpio/gpio" << pin << "/direction";
    std::ofstream direction(directionPath.str().c_str());
    iop_assert(direction, std::string("Unable to set direction of GPIO: ") + pinStr);

    direction << (mode == Mode::INPUT ? "in" : "out");
    direction.close();
}

auto GPIO::digitalRead(const PinRaw pin) const noexcept -> Data {
    std::stringstream path;
    path << "/sys/class/gpio/gpio" << pin << "/value";

    std::ifstream file(path.str().c_str());
    iop_assert(file, std::string("Unable to read from GPIO: ") + std::to_string(pin));

    std::string tmp;
    file >> tmp;
    file.close();

    return tmp == "0" ? Data::LOW : Data::HIGH;
}

auto GPIO::setInterruptCallback(const PinRaw pin, const InterruptState state, void (*func)()) const noexcept -> void {
    auto gpio = std::make_shared<GPIO>(*this);
    // TODO: fix this, this is horrible
    std::thread([pin, state, func, gpio]() {
        auto lastValue = gpio->digitalRead(pin);
        while (true) {
            auto value = gpio->digitalRead(pin);
            switch (state) {
                case InterruptState::CHANGE:
                    if (lastValue == Data::LOW && value == Data::HIGH) func();
                    if (lastValue == Data::HIGH && value == Data::LOW) func();
                    break;
                case InterruptState::RISING:
                    if (lastValue == Data::LOW && value == Data::HIGH) func();
                    break;
                case InterruptState::FALLING:
                    if (lastValue == Data::HIGH && value == Data::LOW) func();
                    break;
            }
            lastValue = value;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }).detach();
}
}  // namespace io
} // namespace iop_hal