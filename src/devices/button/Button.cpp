#include "Button.h"

#include <utility>

namespace uullrich::playground
{

Button::Button(uint16_t pin, uint32_t debounceMs, PressHandler onPress)
    : m_pin{pin},
      m_debounceMs{debounceMs},
      m_onPress{std::move(onPress)}
{
}

void Button::handleExti(uint16_t triggeredPin)
{
    if (triggeredPin != m_pin)
        return;

    const uint32_t now = HAL_GetTick();
    if ((now - m_lastPressTick) < m_debounceMs)
        return;
    m_lastPressTick = now;

    if (m_onPress)
        m_onPress();
}

}
