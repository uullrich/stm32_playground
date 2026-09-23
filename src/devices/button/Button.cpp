#include "Button.h"

namespace uullrich::playground
{

Button::Button(uint16_t pin, uint32_t debounceMs)
    : m_pin{pin},
      m_debounceMs{debounceMs}
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

    m_pressed.store(true);
}

bool Button::consumePress()
{
    return m_pressed.exchange(false);
}

}
