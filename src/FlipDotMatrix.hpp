//
// Created by patrick on 8/22/22.
//
#pragma once
#include "BinaryUtility.hpp"
#include "HWConfig.hpp"
#include "remote_fmt/remote_fmt.hpp"

#include <array>
#include <cstdint>

template<typename Pins>
struct PixelAddress {
    std::uint8_t address : 5;
    PixelAddress(std::uint8_t address_) : address{address_} {}
    std::uint8_t getA0() { return address & 0x1; }
    std::uint8_t getA1() { return address & 0x2; }
    std::uint8_t getA2() { return address & 0x4; }
    std::uint8_t getB0() { return address & 0x8; }
    std::uint8_t getB1() { return address & 0x10; }
    void         setHWAddress() {
                if(getA0()) {
                    apply(set(Pins::A0));
        } else {
                    apply(clear(Pins::A0));
        }

                if(getA1()) {
                    apply(set(Pins::A1));
        } else {
                    apply(clear(Pins::A1));
        }

                if(getA2()) {
                    apply(set(Pins::A2));
        } else {
                    apply(clear(Pins::A2));
        }

                if(getB0()) {
                    apply(set(Pins::B0));
        } else {
                    apply(clear(Pins::B0));
        }

                if(getB1()) {
                    apply(set(Pins::B1));
        } else {
                    apply(clear(Pins::B1));
        }
    }

    PixelAddress& operator++() {
        ++address;
        if(address % 8 == 0) {
            ++address;
        }
        return *this;
    }
};

template<std::size_t columns, std::size_t pixPerRow>
struct FlipDotMatrix {
    static constexpr auto arraySize{(columns * pixPerRow) / (sizeof(std::uint8_t) * 8)};
    using Clock = HW::SystickClock;

    std::array<std::uint8_t, arraySize> matrix{};

private:
    std::array<std::uint8_t, arraySize> oldMatrix{};
    std::array<std::uint8_t, arraySize> diffMatrix{};

public:
    explicit FlipDotMatrix() {
        diffMatrix.fill(0xFF);
        fill(false);
        flush();
        flush();
    }

    void fill(bool state) {
        for(auto& m : matrix) {
            for(std::uint8_t i{0}; i < 8; ++i) {
                if(state) {
                    binUtil::setBit(m, i);
                } else {
                    binUtil::clearBit(m, i);
                }
            }
        }
    }

    struct PinsMuxData {
        static constexpr auto A0{HW::Pin::A0MuxData{}};
        static constexpr auto A1{HW::Pin::A1MuxData{}};
        static constexpr auto A2{HW::Pin::A2MuxData{}};
        static constexpr auto B0{HW::Pin::B0MuxData{}};
        static constexpr auto B1{HW::Pin::B1MuxData{}};
    };

    struct PinsMuxHiLo {
        static constexpr auto A0{HW::Pin::A0MuxHiLo{}};
        static constexpr auto A1{HW::Pin::A1MuxHiLo{}};
        static constexpr auto A2{HW::Pin::A2MuxHiLo{}};
        static constexpr auto B0{HW::Pin::B0MuxHiLo{}};
        static constexpr auto B1{HW::Pin::B1MuxHiLo{}};
    };

    void setPixelOutput(bool pixelState) {
        if(pixelState) {
            apply(
              set(HW::Pin::DataMuxData{}),
              set(HW::Pin::EnableMuxData{}),
              set(HW::Pin::EnableMuxLo{}),
              clear(HW::Pin::EnableMuxHi{}));
        } else {
            apply(
              clear(HW::Pin::DataMuxData{}),
              set(HW::Pin::EnableMuxData{}),
              clear(HW::Pin::EnableMuxLo{}),
              set(HW::Pin::EnableMuxHi{}));
        }
        Clock::delay<std::chrono::microseconds, 250>();

        apply(
          clear(HW::Pin::EnableMuxHi{}),
          clear(HW::Pin::EnableMuxLo{}),
          clear(HW::Pin::EnableMuxData{}));

        Clock::delay<std::chrono::microseconds, 200>();
    }

    void compareMatrix() {
        diffMatrix.fill(0);
        for(std::size_t i{0}; i < matrix.size(); ++i) {
            auto& current = matrix[i];
            auto& diff    = diffMatrix[i];
            auto& old     = oldMatrix[i];
            for(std::size_t j{0}; j < 8; ++j) {
                auto currentBit = binUtil::checkBit(current, j);
                auto oldBit     = binUtil::checkBit(old, j);
                if(currentBit != oldBit) {
                    binUtil::setBit(diff, j);
                }
            }
            old = current;
        }
    }

    void writeMatrix() {
        compareMatrix();
        flush();
        flush();
    }

    void flush() {
        PixelAddress<PinsMuxHiLo> rowAddress{1};
        PixelAddress<PinsMuxData> pixelAddress{1};
        std::size_t               counter{0};
        for(std::size_t i{0}; i < matrix.size(); ++i) {
            auto& current = matrix[i];
            auto& diff    = diffMatrix[i];
            for(std::size_t j{0}; j < 8; ++j) {
                if(binUtil::checkBit(diff, j)) {
                    pixelAddress.setHWAddress();
                    rowAddress.setHWAddress();
                    auto output = binUtil::checkBit(current, j);
                    setPixelOutput(output);
                }
                ++pixelAddress;
                ++counter;
                if(counter % 28 == 0) {
                    ++rowAddress;
                }
            }
        }
    }

    enum class State { reset, init };

    void handler() { writeMatrix(); }
};
