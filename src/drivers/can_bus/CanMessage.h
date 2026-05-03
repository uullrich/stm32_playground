#pragma once

#include <array>
#include <cstdint>

namespace uullrich::playground
{

struct CanMessage
{
    static constexpr std::size_t MAX_LEN = 8;

    std::uint32_t id{0};
    std::array<std::uint8_t, MAX_LEN> data{};
    std::uint8_t length{0};
    bool extended{false};
    bool remote{false};
};

}
