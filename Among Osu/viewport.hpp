#pragma once
#include <utility>

#include "utils/memory.hpp"

class viewport
{
private:
    uint32_t base_{};

public:
    viewport() = default;

    explicit viewport(const uint32_t base) : base_(base) {}

    std::pair<float, float> get_playfield_size() const
    {
        const float height = read<int>(base_ + 0x10);

        const auto window_ratio = height / 480;
        return std::make_pair<float, float>(
            512 * window_ratio,
            384 * window_ratio);
    }

	std::pair<float, float> screen_to_playfield(const std::pair<float, float> screen_coord) const
    {
        const float width = read<int>(base_ + 0xC);
        const float height = read<int>(base_ + 0x10);

        const auto window_ratio = height / 480;

        return std::make_pair<float, float>(
            screen_coord.first - ((width - (512 * window_ratio)) / 2),
            screen_coord.second - ((height - (384 * window_ratio)) / 4 * 3 + (-16 * window_ratio)));
    }

    std::pair<float, float> playfield_to_screen(const std::pair<float, float> playfield_coord) const
    {
        const float x = read<int>(base_ + 0x4);
        const float y = read<int>(base_ + 0x8);
        const float width = read<int>(base_ + 0xC);
        const float height = read<int>(base_ + 0x10);

        auto adjusted_width = width;
        auto adjusted_height = height;
    	
        if (width * 3 > height * 4)
            adjusted_width = height * 4 / 3;
        else
            adjusted_height = width * 3 / 4;

        const auto multiplier_x = adjusted_width / 640.f;
        const auto multiplier_y = adjusted_height / 480.f;

        const auto offset_x = static_cast<int>(width - 512.f * multiplier_x) / 2 + x + 1;
        const auto offset_y = static_cast<int>(height - 384.f * multiplier_y) / 2 + y + (8.f * multiplier_y);

        return std::make_pair<float, float>(
            (playfield_coord.first * multiplier_x) + offset_x,
            (playfield_coord.second * multiplier_y) + offset_y);
    }
};
