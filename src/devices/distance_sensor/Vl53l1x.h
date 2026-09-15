#pragma once

#include "IDistanceSensor.h"
#include "Vl53l1xPlatform.h"

namespace uullrich::playground
{

class Vl53l1x final : public IDistanceSensor
{
  public:
    explicit Vl53l1x(II2cBus& bus);
    [[nodiscard]] Status init() override;
    [[nodiscard]] Status poll(Measurement& measurement) override;

  private:
    [[nodiscard]] Status driverStatus(int8_t result) const;
    [[nodiscard]] Status disable(Status status);
    [[nodiscard]] Status scheduleRecovery(Status status);
    [[nodiscard]] Status recover();

    enum class RecoveryState { None, Backoff, WaitingForStop };

    static constexpr uint16_t ADDRESS = 0x29;
    static constexpr uint32_t INIT_TIMEOUT_MS = 1000;
    static constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500;
    static constexpr uint32_t POLL_TIMEOUT_MS = 40;
    static constexpr uint32_t RECOVERY_BACKOFF_MS = 1000;
    static constexpr uint32_t STOP_SETTLE_MS = 100;

    Vl53l1xPlatform m_platform;
    bool m_active{false};
    uint32_t m_lastMeasurementMs{0};
    RecoveryState m_recoveryState{RecoveryState::None};
    uint32_t m_recoveryTick{0};
};

}
