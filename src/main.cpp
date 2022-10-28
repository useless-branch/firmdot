#include "ApplicationConfig.hpp"
///
#include "BinaryUtility.hpp"
#include "FlipDotMatrix.hpp"
#include "package.hpp"
#include "aglio/packager.hpp"
#include "kvasir/Util/StaticVector.hpp"
#include "aglio/remote_fmt.hpp"
//#include "cmake_git_version/version.hpp"

namespace uc_log {
    inline void log_assert(int line, std::string_view file) { UC_LOG_C("assert in {}:{}", file, line); }
}   // namespace uc_log

struct Crc {
    using type = std::uint16_t;
    static type calc(std::span<std::byte const> data) {
        return Kvasir::CRC::calcCrc<Kvasir::CRC::CRC_Type::crc16, DMA, DMA::Channel::ch5>(data);
    }
};

using packager = aglio::Packager<aglio::CrcConfig<Crc>>;

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

    Kvasir::StaticVector<std::byte,256> rxBuffer;

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
            std::optional<std::byte> sna;
            Uart::rxbuffer_.pop_into(sna);
            if(sna){
                rxBuffer.push_back(*sna);
                while(!rxBuffer.empty()){
                    auto optionalPack = packager::unpack<package::Package>(rxBuffer);
                    if(optionalPack)
                    {
                        UC_LOG_D("{}", *optionalPack);
                        apply(set(HW::Pin::Led{}));
                    }
                    else{
                        break;
                    }
                }
            }
        }
        else
        {
            apply(clear(HW::Pin::Led{}));
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
