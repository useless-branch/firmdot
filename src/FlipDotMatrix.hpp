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

template<std::size_t columns, std::size_t pixPerRow, typename Clock>
struct FlipDotMatrix {
    static constexpr auto arraySize{(columns * pixPerRow) / (sizeof(std::uint8_t) * 8)};
    std::size_t flushCounter{0};
    std::array<std::uint8_t, arraySize> matrix{};

private:
    std::array<std::uint8_t, arraySize> oldMatrix{};
    std::array<std::uint8_t, arraySize> diffMatrix{};
    PixelAddress<PinsMuxHiLo>           rowAddress{1};
    PixelAddress<PinsMuxData>           pixelAddress{1};
    std::size_t                         currentByte{0};
    std::size_t                         currentBit{0};
    bool                                currentPixelValue{false};

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
        //Clock::delay<std::chrono::microseconds, 200>();

        apply(
          clear(HW::Pin::EnableMuxHi{}),
          clear(HW::Pin::EnableMuxLo{}),
          clear(HW::Pin::EnableMuxData{}));

        //Clock::delay<std::chrono::microseconds, 150>();
    }

    void compareMatrix() {
        diffMatrix.fill(0x00);
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

    void resetWriting()
    {
        currentByte          = 0;
        currentBit           = 0;
        counter              = 0;
        pixelAddress.address = 1;
        rowAddress.address   = 1;
        st = State::idle;
    }

    void writeFullPicture(){

    }

    void writeMatrix() {
        if(frameCounter > 3){
            frameCounter = 0;
            diffMatrix.fill(0xFF);
            flush();
        }
        else{
            compareMatrix();
            flush();
        }

    }

    void flush() { flushCounter += 3; }

    enum class State { idle, flush, set, wait2reset, reset, wait2idle, incrementPixel };
    State st{State::idle};
    using tp = typename Clock::time_point;
    tp                    setTime;
    tp                    resetTime;
    static constexpr auto set_time{std::chrono::microseconds(100)};
    static constexpr auto reset_time{std::chrono::microseconds(50)};
    std::size_t           counter{0};
    std::size_t frameCounter{0};

    void handler() {
        auto const currentTime = Clock::now();
        switch(st) {
        case State::idle:
            {
                if(flushCounter > 0){
                    ++frameCounter;
                    st = State::flush;
                }
            }
            break;
        case State::flush:
            {
                if(currentByte > matrix.size()) {
                    resetWriting();
                    UC_LOG_D("WriteCounter {}", flushCounter);
                    --flushCounter;
                    st                   = State::idle;
                    break;
                }
                if(currentBit > 7) {
                    currentBit = 0;
                    ++currentByte;
                }
                auto& current = matrix[currentByte];
                auto& diff    = diffMatrix[currentByte];

                if(binUtil::checkBit(diff, currentBit)) {
                    pixelAddress.setHWAddress();
                    rowAddress.setHWAddress();
                    currentPixelValue = binUtil::checkBit(current, currentBit);
                    st                = State::set;
                } else {
                    st = State::incrementPixel;
                }
            }
            break;
        case State::set:
            {
                //UC_LOG_D("Setting Pixel ({},{}) to {}", currentByte, currentBit, currentPixelValue);
                if(currentPixelValue) {
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
                st      = State::wait2reset;
                setTime = currentTime + set_time;
            }
            break;
        case State::wait2reset:
            {
                if(currentTime > setTime) {
                    st = State::reset;
                }
            }
            break;
        case State::reset:
            {
                apply(
                  clear(HW::Pin::EnableMuxHi{}),
                  clear(HW::Pin::EnableMuxLo{}),
                  clear(HW::Pin::EnableMuxData{}));
                resetTime = currentTime + reset_time;
                st        = State::wait2idle;
            }
            break;
        case State::wait2idle:
            {
                if(currentTime > resetTime) {
                    st = State::incrementPixel;
                }
            }
            break;
        case State::incrementPixel:
            {
                ++pixelAddress;
                ++counter;
                if(counter % 28 == 0) {
                    ++rowAddress;
                }
                ++currentBit;
                st = State::flush;
            }
            break;
        }
    }
};
