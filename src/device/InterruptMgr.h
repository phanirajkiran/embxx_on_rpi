//
// Copyright 2013 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstddef>
#include <type_traits>
#include <array>
#include <algorithm>

#include "embxx/util/Assert.h"
#include "embxx/util/StaticFunction.h"

namespace device
{

namespace interrupt
{

inline
void enable()
{
    __asm volatile("cpsie i");
}

inline
void disable()
{
    __asm volatile("cpsid i");
}

}  // namespace interrupt

template <typename THandler = embxx::util::StaticFunction<void ()> >
class InterruptMgr
{
public:
    typedef THandler HandlerFunc;
    enum IrqId {
        IrqId_Timer,
        IrqId_AuxInt,
        IrqId_Gpio1,
        IrqId_Gpio2,
        IrqId_Gpio3,
        IrqId_Gpio4,
        IrqId_I2C,
        IrqId_SPI,
        IrqId_NumOfIds // Must be last
    };

    InterruptMgr();

    template <typename TFunc>
    void registerHandler(IrqId id, TFunc&& handler);

    void enableInterrupt(IrqId id);

    void disableInterrupt(IrqId id);

    void handleInterrupt();

private:

    typedef std::uint32_t EntryType;

    struct IrqInfo {
        IrqInfo();

        HandlerFunc handler_;
        volatile EntryType* pendingPtr_;
        EntryType pendingMask_;
        volatile EntryType* enablePtr_;
        volatile EntryType* disablePtr_;
        EntryType enDisMask_;
    };

    typedef std::array<IrqInfo, IrqId_NumOfIds> IrqsArray;

    IrqsArray irqs_;

    static constexpr volatile EntryType* IrqBasicPending =
        reinterpret_cast<EntryType*>(0x2000B200);

    static constexpr volatile EntryType* IrqPending1 =
        reinterpret_cast<EntryType*>(0x2000B204);

    static constexpr volatile EntryType* IrqPending2 =
        reinterpret_cast<EntryType*>(0x2000B208);

    static constexpr volatile EntryType* IrqEnableBasic =
        reinterpret_cast<EntryType*>(0x2000B218);

    static constexpr volatile EntryType* IrqEnable1 =
        reinterpret_cast<EntryType*>(0x2000B210);

    static constexpr volatile EntryType* IrqEnable2 =
        reinterpret_cast<EntryType*>(0x2000B214);

    static constexpr volatile EntryType* IrqDisableBasic =
        reinterpret_cast<EntryType*>(0x2000B224);

    static constexpr volatile EntryType* IrqDisable1 =
        reinterpret_cast<EntryType*>(0x2000B21C);

    static constexpr volatile EntryType* IrqDisable2 =
        reinterpret_cast<EntryType*>(0x2000B220);

    static const EntryType MaskPending1 = 0x00000100;
    static const EntryType MaskPending2 = 0x00000200;

};

// Implementation

template <typename THandler>
InterruptMgr<THandler>::InterruptMgr()
{
    {
        auto& timerIrq = irqs_[IrqId_Timer];
        timerIrq.pendingPtr_ = IrqBasicPending;
        timerIrq.pendingMask_ = static_cast<EntryType>(1) << 0;
        timerIrq.enablePtr_ = IrqEnableBasic;
        timerIrq.disablePtr_ = IrqDisableBasic;
        timerIrq.enDisMask_ = timerIrq.pendingMask_;
        static_cast<void>(timerIrq);
    }

    {
        auto& auxIrq = irqs_[IrqId_AuxInt];
        auxIrq.pendingPtr_ = IrqPending1;
        auxIrq.pendingMask_ = static_cast<EntryType>(1) << 29;
        auxIrq.enablePtr_ = IrqEnable1;
        auxIrq.disablePtr_ = IrqDisable1;
        auxIrq.enDisMask_ = auxIrq.pendingMask_;
        static_cast<void>(auxIrq);
    }

    for (int i = 0; i <= (IrqId_Gpio4 - IrqId_Gpio1); ++i) {
        auto& gpioIrq = irqs_[IrqId_Gpio1 + i];
        gpioIrq.pendingPtr_ = IrqPending2;
        gpioIrq.pendingMask_ = static_cast<EntryType>(1) << ((49 - 32) + i);
        gpioIrq.enablePtr_ = IrqEnable2;
        gpioIrq.disablePtr_ = IrqDisable2;
        gpioIrq.enDisMask_ = gpioIrq.pendingMask_;
        static_cast<void>(gpioIrq);
    }

    {
        auto& i2cIrq = irqs_[IrqId_I2C];
        i2cIrq.pendingPtr_ = IrqBasicPending;
        i2cIrq.pendingMask_ = static_cast<EntryType>(1) << 15;
        i2cIrq.enablePtr_ = IrqEnable2;
        i2cIrq.disablePtr_ = IrqDisable2;
        i2cIrq.enDisMask_ = static_cast<EntryType>(1) << (53 - 32);
        static_cast<void>(i2cIrq);
    }

    {
        auto& spiIrq = irqs_[IrqId_SPI];
        spiIrq.pendingPtr_ = IrqBasicPending;
        spiIrq.pendingMask_ = static_cast<EntryType>(1) << 16;
        spiIrq.enablePtr_ = IrqEnable2;
        spiIrq.disablePtr_ = IrqDisable2;
        spiIrq.enDisMask_ = static_cast<EntryType>(1) << (54 - 32);
        static_cast<void>(spiIrq);
    }
}

template <typename THandler>
template <typename TFunc>
void InterruptMgr<THandler>::registerHandler(
    IrqId id,
    TFunc&& handler)
{
    GASSERT(id < IrqId_NumOfIds);
    irqs_[id].handler_ = std::forward<TFunc>(handler);
}

template <typename THandler>
void InterruptMgr<THandler>::enableInterrupt(IrqId id)
{
    GASSERT(id < IrqId_NumOfIds);
    auto& info = irqs_[id];
    *info.enablePtr_ = info.enDisMask_;
}

template <typename THandler>
void InterruptMgr<THandler>::disableInterrupt(IrqId id)
{
    GASSERT(id < IrqId_NumOfIds);
    auto& info = irqs_[id];
    *info.disablePtr_ = info.enDisMask_;
}

template <typename THandler>
void InterruptMgr<THandler>::handleInterrupt()
{
    auto irqsBasic = *IrqBasicPending;
    auto irqsPending1 = *IrqPending1;
    auto irqsPending2 = *IrqPending2;

    std::for_each(irqs_.begin(), irqs_.end(),
        [this, irqsBasic, irqsPending1, irqsPending2](IrqInfo& info)
        {
            bool invoke = false;
            do {
                if (info.pendingPtr_ == IrqBasicPending) {
                    if ((info.pendingMask_ & irqsBasic) != 0) {
                        invoke = true;
                    }
                    break;
                }

                if (info.pendingPtr_ == IrqPending1) {
                    if (((irqsBasic & MaskPending1) != 0) &&
                        ((info.pendingMask_ & irqsPending1) != 0)) {
                        invoke = true;
                    }
                    break;
                }

                if (info.pendingPtr_ == IrqPending2) {
                    if (((irqsBasic & MaskPending2) != 0) &&
                        ((info.pendingMask_ & irqsPending2) != 0)) {
                        invoke = true;
                    }
                    break;
                }
            } while (false);

            if (invoke) {
                if (info.handler_) {
                    info.handler_();
                }
            }
        });
}

template <typename THandler>
InterruptMgr<THandler>::IrqInfo::IrqInfo()
    : pendingPtr_(0),
      pendingMask_(0),
      enablePtr_(0),
      disablePtr_(0),
      enDisMask_(0)
{
}

}  // namespace device


