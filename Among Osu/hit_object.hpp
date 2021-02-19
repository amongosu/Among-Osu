#pragma once
#include <utility>

#include "utils/memory.hpp"

enum hit_object_type
{
    circle = 1 << 0,
    slider = 1 << 1,
    new_combo = 1 << 2,
    spinner = 1 << 3,
    combo_offset = 1 << 4 | 1 << 5 | 1 << 6,
    hold = 1 << 7
};

class hit_object {
private:
    uint32_t base_{};
	
    std::pair<int, int> time_info_{};
    std::pair<float, float> position_{};

public:
    hit_object() = default;

    explicit hit_object(const uintptr_t base) : base_(base) {}

	bool is_valid() const
    {
        return base_;
    }

    hit_object_type get_object_type() const
    {
        auto type = read<int>(base_ + 0x18);
        type &= ~hit_object_type::combo_offset;
        type &= ~hit_object_type::new_combo;

        return static_cast<hit_object_type>(type);
    }

    std::pair<int, int> get_time_info() const
    {
        return std::make_pair<int, int>(read<int>(base_ + 0x10), read<int>(base_ + 0x14));
    }

    std::pair<float, float> get_position() const
    {
        return std::make_pair<float, float>(read<float>(base_ + 0x38), read<float>(base_ + 0x3C));
    }

	static float get_radius(const std::pair<float, float> pf_size, float circle_size)
    {
        const auto radius = (pf_size.first / 8 * (1 - 0.7 * ((circle_size - 5) / 5))) / 2 / (pf_size.second / 384) * 1.00041f;

        return radius;
    }
};
