#pragma once

#include "IDistanceSensor.h"
#include "Vl53l1xPlatform.h"
#include "SysTickClock.h"

#include <chrono>

namespace uullrich::playground
{

class Vl53l1x final : public IDistanceSensor
{
  public:
    explicit Vl53l1x(II2cBus& bus);

    Vl53l1x(const Vl53l1x&) = delete;
    Vl53l1x& operator=(const Vl53l1x&) = delete;

    [[nodiscard]] Status init() override;
    [[nodiscard]] PollResult poll() override;

  private:
    [[nodiscard]] Status driverStatus(int8_t result) const;
    [[nodiscard]] Status disable(Status status);
    [[nodiscard]] Status scheduleRecovery(Status status);
    [[nodiscard]] Status recover();
    [[nodiscard]] Status configure();

    enum class RecoveryState { None, Backoff, WaitingForStop };

    static constexpr std::chrono::milliseconds INIT_TIMEOUT{1000};
    static constexpr std::chrono::milliseconds MEASUREMENT_TIMEOUT{500};
    static constexpr std::chrono::milliseconds POLL_TIMEOUT{40};
    static constexpr std::chrono::milliseconds RECOVERY_BACKOFF{1000};
    static constexpr std::chrono::milliseconds STOP_SETTLE{100};

    Vl53l1xPlatform m_platform;
    bool m_active{false};
    SysTickClock::time_point m_lastMeasurement{};
    RecoveryState m_recoveryState{RecoveryState::None};
    SysTickClock::time_point m_recoveryStart{};
};

}
