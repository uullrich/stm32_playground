#pragma once

#include "IDimmableLed.h"
#include "IPwmOutput.h"

#include <cstdint>

namespace uullrich::playground
{

class DimmableLed : public IDimmableLed
{
  public:
    explicit DimmableLed(IPwmOutput& output);

    DimmableLed(const DimmableLed&) = delete;
    DimmableLed& operator=(const DimmableLed&) = delete;

    void on() override;
    void off() override;
    void toggle() override;
    void setBrightnessPercent(uint8_t percent) override;

  private:
    IPwmOutput& m_output;
    bool m_isOn{false};
};

}
