#pragma once
#include <utility>

#include "utils/memory.hpp"

class beatmap
{
private:
    uint32_t base_{};

public:
    beatmap() = default;

    explicit beatmap(const uint32_t base) : base_(base) {}

	float get_circle_size()
    {
        return read<float>(base_ + 0x30);
    }
};
