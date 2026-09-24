#include "Button.h"

namespace uullrich::playground
{

Button::Button(uint16_t pin, std::chrono::milliseconds debounce)
    : m_pin{pin},
      m_debounce{debounce}
{
}

void Button::handleExti(uint16_t triggeredPin)
{
    if (triggeredPin != m_pin)
        return;

    const auto now = SysTickClock::now();
    if (now - m_lastPress < m_debounce)
        return;
    m_lastPress = now;

    m_pressed.store(true, std::memory_order_release);
}

bool Button::consumePress()
{
    return m_pressed.exchange(false, std::memory_order_acquire);
}

}
