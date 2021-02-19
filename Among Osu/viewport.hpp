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
        const float width = read<int>(base_ + 0xC);
        const float height = read<int>(base_ + 0x10);

        const auto window_ratio = height / 480;
        const auto playfield_ratio = (384 * window_ratio) / 384;

        return std::make_pair<float, float>(
            (playfield_coord.first * playfield_ratio) + ((width - (512 * window_ratio)) / 2),
            (playfield_coord.second * playfield_ratio) + ((height - (384 * window_ratio)) / 4 * 3));
    }
};
