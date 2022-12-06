#pragma once
#include "HWConfig.hpp"
///

#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"

using Clock          = HW::SystickClock;
using StackProtector = Kvasir::StackProtector<>;
using FaultHandler   = Kvasir::Fault::Handler<HW::Fault_CleanUpAction>;

struct DMAConfig {
    static constexpr auto numberOfChannels     = 6;
    static constexpr auto callbackFunctionSize = 64;
};

using DMA = Kvasir::DMA::DmaBase<DMAConfig>;

using Uart
  = Kvasir::UART::UartBehavior<HW::UartConfig, DMA, DMA::Channel::ch0, DMA::Priority::low, 2048>;


using Startup = Kvasir::Startup::Startup<
  HW::ClockSettings,
  Clock,
  HW::ComBackend,
  FaultHandler,
  StackProtector,
  HW::PinConfig,
  DMA,
  Uart
  >;
