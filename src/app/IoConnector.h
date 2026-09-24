#pragma once

#include "IIoRepository.h"
#include "IIoWriteListener.h"
#include "IoRepository.h"
#include "VirtualDigitalOutput.h"
#include "VirtualPwmOutput.h"
#include "VirtualAdcInput.h"
#include "IDigitalOutput.h"
#include "IPwmOutput.h"
#include "IAdcInput.h"

namespace uullrich::playground
{

class IoConnector final : public IIoWriteListener
{
  public:
    explicit IoConnector(IDigitalOutput& ld2, IDigitalOutput& ld3, IDigitalOutput& d6,
                         IPwmOutput& ld1, IAdcInput& adcPoti, IAdcInput& adcAnode);

    IoConnector(const IoConnector&) = delete;
    IoConnector& operator=(const IoConnector&) = delete;

    [[nodiscard]] IIoRepository& repository();

    void onWritten(const IVirtualIo& io) override;
    [[nodiscard]] bool consumeAnimatedOutputOverride();

  private:
    VirtualDigitalOutput m_ld2;
    VirtualDigitalOutput m_ld3;
    VirtualDigitalOutput m_d6;
    VirtualPwmOutput m_ld1;
    VirtualAdcInput m_adcPoti;
    VirtualAdcInput m_adcAnode;
    IoRepository m_repository{};
    bool m_animatedOutputOverridden{false};
};

}
