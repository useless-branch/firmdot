#include "aglio/type_descriptor.hpp"
#include <array>

namespace package {
    struct Package {
        std::uint32_t panelID;
        std::array<std::uint8_t, 84> matrixData;
    };
}   // namespace Potentiometer

#include "TypeDescriptor_package.hpp"
