#pragma once

#include "ILogger.h"

#include <string>
#include <string_view>

namespace uullrich::playground::test
{

class CapturingLogger final : public ILogger
{
  public:
    void write(std::string_view text) override
    {
        m_captured.append(text);
        ++m_writeCount;
    }

    [[nodiscard]] const std::string& captured() const { return m_captured; }
    [[nodiscard]] int writeCount() const { return m_writeCount; }

  private:
    std::string m_captured;
    int m_writeCount{0};
};

}
