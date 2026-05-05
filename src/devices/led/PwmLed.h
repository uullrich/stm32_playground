#pragma once

#include "IDimmableLed.h"
#include "IPwmOutput.h"

#include <cstdint>

namespace uullrich::playground
{

class PwmLed : public IDimmableLed
{
  public:
    explicit PwmLed(IPwmOutput& output);

    PwmLed(const PwmLed&) = delete;
    PwmLed& operator=(const PwmLed&) = delete;

    void on() override;
    void off() override;
    void toggle() override;
    void setBrightnessPercent(std::uint8_t percent) override;

  private:
    IPwmOutput& m_output;
    bool m_isOn{false};
};

}
