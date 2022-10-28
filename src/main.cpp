#include "ApplicationConfig.hpp"
///
#include "BinaryUtility.hpp"
#include "FlipDotMatrix.hpp"
#include "chip/DMA.hpp"
#include "chip/UART.hpp"
#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"
#include "remote_fmt/remote_fmt.hpp"
//#include "cup.h"

//#include "cmake_git_version/version.hpp"

namespace uc_log {
    inline void log_assert(int line, std::string_view file) { UC_LOG_C("assert in {}:{}", file, line); }
}   // namespace uc_log

int main() {
    UC_LOG_D("{}", "Test");
    std::array<std::uint8_t, 84> matrix;
    matrix.fill(0);

    FlipDotMatrix<24, 28> FDMatrix;
    FDMatrix.fill(false);

    auto next        = Clock::time_point{} + std::chrono::milliseconds(400);
    auto everySecond = Clock::time_point{} + std::chrono::seconds(1);
    
    std::array<std::byte, 2> test;
    test.fill(std::byte{0xAB});

    while(true) {
        auto now = Clock::now();
        if(now > next) {
            UC_LOG_D("I am alive");
            if(Uart::operationState() == Uart::OperationState::succeeded){
                UC_LOG_D("Sending...");
                Uart::send_nocopy(test);
            }
            if(Uart::operationState() == Uart::OperationState::failed){
                UC_LOG_E("Failed to send!");
            }
            next = now + std::chrono::seconds(1);
        }

        if(!Uart::rxbuffer_.empty()){
            UC_LOG_D("Got messages!");
            Uart::rxbuffer_.pop();
            apply(toggle(HW::Pin::Led{}));
        }



        if(now > everySecond) {

            everySecond = now + std::chrono::milliseconds(400);
        }

        std::memcpy(&FDMatrix.matrix, &matrix, sizeof(matrix));
        FDMatrix.handler();
        StackProtector::handler();
    }
}

KVASIR_START(Startup)
