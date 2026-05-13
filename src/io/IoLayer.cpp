#include "IoLayer.h"
#include "IoRepositoryBuilder.h"

namespace uullrich::playground
{

IoLayer::IoLayer(IDigitalOutput& ld2, IDigitalOutput& ld3, IDigitalOutput& d6, IPwmOutput& ld1,
                 IAdcInput& adcPoti, IAdcInput& adcAnode)
    : m_ld2{ld2, 0},
      m_ld3{ld3, 1},
      m_d6{d6, 2},
      m_ld1{ld1, 0},
      m_adcPoti{adcPoti, 0},
      m_adcAnode{adcAnode, 1}
{
    IoRepositoryBuilder{m_repository}.add(m_ld2).add(m_ld3).add(m_d6).add(m_ld1).add(m_adcPoti).add(
        m_adcAnode);
}

IIoRepository& IoLayer::repository()
{
    return m_repository;
}

}
