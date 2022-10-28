#include "HWConfig.hpp"
///
#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"
#include "remote_fmt/remote_fmt.hpp"
#include "FlipDotMatrix.hpp"
#include "BinaryUtility.hpp"
//#include "cup.h"


//#include "cmake_git_version/version.hpp"
using Clock          = HW::SystickClock;
using StackProtector = Kvasir::StackProtector<>;
using FaultHandler   = Kvasir::Fault::Handler<HW::Fault_CleanUpAction>;

using Startup = Kvasir::Startup::
  Startup<HW::ClockSettings, Clock, HW::ComBackend, FaultHandler, StackProtector, HW::PinConfig>;

int main() {
    UC_LOG_D("{}", "Test");
    std::array<std::uint8_t, 84> matrix;
    matrix.fill(0);
    auto next = Clock::time_point{} + std::chrono::milliseconds(400);
    auto everySecond = Clock::time_point{} + std::chrono::seconds(1);
    FlipDotMatrix<24,28> FDMatrix;
    FDMatrix.fill(false);
    bool Babel = false;
    while(true) {
        auto now = Clock::now();
        if(now > next){
            apply(toggle(HW::Pin::Led{}));
            next = now + std::chrono::milliseconds(200);
        }

        if(now > everySecond){

            everySecond = now + std::chrono::milliseconds(400);
        }

        std::memcpy(&FDMatrix.matrix, &matrix, sizeof(matrix));
        FDMatrix.handler();
        StackProtector::handler();
    }
}

KVASIR_START(Startup)
