#pragma once

struct DbgPres {
    bool operator()() { return true; }
};

#include "uc_log/DefaultRttComBackend.hpp"

namespace uc_log {
template<>
struct ComBackend<uc_log::Tag::User>
  : public uc_log::DefaultRttComBackend<DbgPres, 32768, 32768, rtt::BufferMode::skip> {};
}   // namespace uc_log
// need to be included first

#include "chip/Interrupt.hpp"
#include "core/core.hpp"
#include "uc_log/uc_log.hpp"

namespace HW {
static constexpr auto ClockSpeed   = 133'000'000;
static constexpr auto CrystalSpeed = 12'000'000;

struct SystickClockConfig {
    static constexpr auto clockBase = Kvasir::Systick::useProcessorClock;

    static constexpr auto clockSpeed     = ClockSpeed;
    static constexpr auto minOverrunTime = std::chrono::hours(24);
};

using SystickClock = Kvasir::Systick::SystickClockBase<SystickClockConfig>;
}   // namespace HW

namespace uc_log {
template<>
struct LogClock<uc_log::Tag::User> : public HW::SystickClock {};
}   // namespace uc_log

#include "chip/chip.hpp"

namespace HW {
using ComBackend = uc_log::ComBackend<uc_log::Tag::User>;

    namespace Pin {
        using EnableMuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin12));
        using EnableMuxHi = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin1));
        using EnableMuxLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin0));

        using DataMuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin13));

        using A0MuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin2));
        using A1MuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin3));
        using A2MuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin4));
        using B0MuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin5));
        using B1MuxData = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin6));

        using A0MuxHiLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin7));
        using A1MuxHiLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin8));
        using A2MuxHiLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin9));
        using B0MuxHiLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin10));
        using B1MuxHiLo = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin11));

        using uart0_tx = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin16));
        using uart0_rx = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin17));
        using Led = decltype(makePinLocation(Kvasir::Io::port0, Kvasir::Io::pin25));
    }   // namespace Pin

struct ClockSettings {
    static void coreClockInit() {
        using Kvasir::Register::value;
        static constexpr std::uint32_t fbdiv{133};
        static constexpr std::uint32_t pd1{6};
        static constexpr std::uint32_t pd2{2};
        static constexpr std::uint32_t refdiv{1};

        static constexpr std::uint32_t usb_fbdiv{120};
        static constexpr std::uint32_t usb_pd1{6};
        static constexpr std::uint32_t usb_pd2{5};
        static constexpr std::uint32_t usb_refdiv{1};

        static_assert(
          ClockSpeed == (CrystalSpeed / refdiv) * fbdiv / (pd1 * pd2),
          "bad clock config");

        using PERI_CLOCK = Kvasir::Peripheral::CLOCKS::Registers<>::CLK_PERI_CTRL;
        using SYS_CLOCK  = Kvasir::Peripheral::CLOCKS::Registers<>::CLK_SYS_CTRL;
        using REF_CLOCK  = Kvasir::Peripheral::CLOCKS::Registers<>::CLK_REF_CTRL;
        using USB_CLOCK  = Kvasir::Peripheral::CLOCKS::Registers<>::CLK_USB_CTRL;
        using XOSC       = Kvasir::Peripheral::XOSC::Registers<>;
        using RST        = Kvasir::Peripheral::RESETS::Registers<0>;
        using PLL        = Kvasir::Peripheral::PLL::Registers<0>;
        using USBPLL     = Kvasir::Peripheral::PLL::Registers<1>;

        // disable periphery clocks
        apply(PERI_CLOCK::overrideDefaults(clear(PERI_CLOCK::enable)));

        // set ref clock to default
        apply(REF_CLOCK::overrideDefaults(write(REF_CLOCK::SRCValC::rosc_clksrc_ph)));

        // set sysclock to default
        apply(SYS_CLOCK::overrideDefaults(write(SYS_CLOCK::SRCValC::clk_ref)));

        apply(write(XOSC::CTRL::ENABLEValC::en), write(XOSC::CTRL::FREQ_RANGEValC::_1_15mhz));
        // wait for XOSC stable
        while(!apply(read(XOSC::STATUS::stable))) {
        }
        {   //sys pll
            // reset pll
            apply(set(RST::RESET::pll_sys));
            apply(clear(RST::RESET::pll_sys));
            while(!apply(read(RST::RESET_DONE::pll_sys))) {
            }

            apply(PLL::CS::overrideDefaults(
              clear(PLL::CS::bypass),
              write(PLL::CS::refdiv, value<refdiv>())));

            apply(write(PLL::FBDIV_INT::fbdiv_int, value<fbdiv>()));

            apply(PLL::PWR::overrideDefaults(
              clear(PLL::PWR::vcopd),
              clear(PLL::PWR::pd),
              set(PLL::PWR::postdivpd)));

            // wait for PLL lock
            while(!apply(read(PLL::CS::lock))) {
            }

            apply(PLL::PRIM::overrideDefaults(
              write(PLL::PRIM::postdiv1, value<pd1>()),
              write(PLL::PRIM::postdiv2, value<pd2>())));

            apply(PLL::PWR::overrideDefaults(
              clear(PLL::PWR::vcopd),
              clear(PLL::PWR::pd),
              clear(PLL::PWR::postdivpd)));

            // set sysclock to pll
            apply(SYS_CLOCK::overrideDefaults(
              write(SYS_CLOCK::SRCValC::clksrc_clk_sys_aux),
              write(SYS_CLOCK::AUXSRCValC::clksrc_pll_sys)));

            // set ref clock to xosc
            apply(REF_CLOCK::overrideDefaults(write(REF_CLOCK::SRCValC::xosc_clksrc)));

            // enable periphery clock
            apply(PERI_CLOCK::overrideDefaults(set(PERI_CLOCK::enable)));
        }

        {   //usb pll
            // reset pll
            apply(set(RST::RESET::pll_usb));
            apply(clear(RST::RESET::pll_usb));
            while(!apply(read(RST::RESET_DONE::pll_usb))) {
            }

            apply(USBPLL::CS::overrideDefaults(
              clear(USBPLL::CS::bypass),
              write(USBPLL::CS::refdiv, value<usb_refdiv>())));

            apply(write(USBPLL::FBDIV_INT::fbdiv_int, value<usb_fbdiv>()));

            apply(USBPLL::PWR::overrideDefaults(
              clear(USBPLL::PWR::vcopd),
              clear(USBPLL::PWR::pd),
              set(USBPLL::PWR::postdivpd)));

            // wait for PLL lock
            while(!apply(read(USBPLL::CS::lock))) {
            }

            apply(USBPLL::PRIM::overrideDefaults(
              write(USBPLL::PRIM::postdiv1, value<usb_pd1>()),
              write(USBPLL::PRIM::postdiv2, value<usb_pd2>())));

            apply(USBPLL::PWR::overrideDefaults(
              clear(USBPLL::PWR::vcopd),
              clear(USBPLL::PWR::pd),
              clear(USBPLL::PWR::postdivpd)));

            // enable periphery clock
            apply(USB_CLOCK::overrideDefaults(
              set(USB_CLOCK::enable),
              write(USB_CLOCK::AUXSRCValC::clksrc_pll_usb)));
        }
    }

    static void peripheryClockInit() {}
};

struct UartConfig {
    static constexpr auto clockSpeed = ClockSpeed;

    static constexpr auto instance      = 0;
    static constexpr auto rxPinLocation = Pin::uart0_rx{};
    static constexpr auto txPinLocation = Pin::uart0_tx{};
    static constexpr auto baudRate      = 115'200;
    static constexpr auto dataBits      = Kvasir::UART::DataBits::_8;
    static constexpr auto parity        = Kvasir::UART::Parity::none;
    static constexpr auto stopBits      = Kvasir::UART::StopBits::_1;
    static constexpr auto isrPriority   = 3;
};

struct Fault_CleanUpAction {
    void operator()() {
    }
};

struct PinConfig {
    static constexpr auto powerClockEnable = list(
      clear(Kvasir::Peripheral::RESETS::Registers<>::RESET::io_bank0),
      clear(Kvasir::Peripheral::RESETS::Registers<>::RESET::pads_bank0));

    static constexpr auto initStepPinConfig = list(
      makeOutput(HW::Pin::Led{}),
      makeOutput(HW::Pin::EnableMuxData{}),
      makeOutput(HW::Pin::EnableMuxHi{}),
      makeOutput(HW::Pin::EnableMuxLo{}),
      makeOutput(HW::Pin::DataMuxData{}),
      makeOutput(HW::Pin::A0MuxData{}),
      makeOutput(HW::Pin::A1MuxData{}),
      makeOutput(HW::Pin::A2MuxData{}),
      makeOutput(HW::Pin::B0MuxData{}),
      makeOutput(HW::Pin::B1MuxData{}),
      makeOutput(HW::Pin::A0MuxHiLo{}),
      makeOutput(HW::Pin::A1MuxHiLo{}),
      makeOutput(HW::Pin::A2MuxHiLo{}),
      makeOutput(HW::Pin::B0MuxHiLo{}),
      makeOutput(HW::Pin::B1MuxHiLo{})
      );
};

}   // namespace HW
