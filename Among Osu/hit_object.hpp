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

    hit_object_type object_type_{};
    std::pair<int, int> time_info_{};

public:
    hit_object() = default;

    explicit hit_object(const uintptr_t base) : base_(base)
    {
        auto type = read<int>(base_ + 0x18);
        type &= ~hit_object_type::combo_offset;
        type &= ~hit_object_type::new_combo;
        object_type_ = static_cast<hit_object_type>(type);

        time_info_ = std::make_pair<int, int>(read<int>(base_ + 0x10), read<int>(base_ + 0x14));
    }

	bool is_valid() const
    {
        return base_;
    }

    hit_object_type get_object_type() const
    {
        return object_type_;
    }

    std::pair<int, int> get_time_info() const
    {
        return time_info_;
    }

    std::pair<float, float> get_position(const float current_time = 0) const
    {
        if (object_type_ == hit_object_type::circle)
			return std::make_pair<float, float>(read<float>(base_ + 0x38), read<float>(base_ + 0x3C));
        else
        {
            if (current_time < time_info_.first || current_time > time_info_.second)
                return std::make_pair<float, float>(read<float>(base_ + 0x38), read<float>(base_ + 0x3C));
        	
            const auto slider_curve_smooth_lines_ptr = read<uint32_t>(base_ + 0xC4);
            const auto slider_cumulative_lengths_ptr = read<uint32_t>(base_ + 0xC8);

            const auto slider_curve_smooth_lines_count = read<uint32_t>(slider_curve_smooth_lines_ptr + 0xC);

        	if (!slider_curve_smooth_lines_count)
                return std::make_pair<float, float>(read<float>(base_ + 0x38), read<float>(base_ + 0x3C));
        	
            const auto slider_cumulative_lengths_count = read<uint32_t>(slider_cumulative_lengths_ptr + 0xC);
        	
            if (!slider_cumulative_lengths_count)
                return std::make_pair<float, float>(read<float>(base_ + 0x38), read<float>(base_ + 0x3C));
            //std::cout << "position - " << read<float>(base_ + 0x38) << ", " << read<float>(base_ + 0x3C) << std::endl;

            const auto repeat_count = read<int>(base_ + 0x20);
            const auto pixel_length = read<double>(base_ + 0x8);
            auto pos = (current_time - static_cast<float>(time_info_.first)) / (static_cast<float>(time_info_.second - time_info_.first) / repeat_count);
            //std::cout << "current_time - " << current_time << std::endl;
            //std::cout << "time_info_.first - " << time_info_.first << std::endl;
            //std::cout << "(static_cast<float>(time_info_.second - time_info_.first) - " << static_cast<float>(time_info_.second - time_info_.first) << std::endl;
            //std::cout << "pos - " << pos << std::endl;

            if (fmodf(pos, 2.f) > 1.f)
                pos = 1 - fmodf(pos, 1.f);
            else
                pos = fmodf(pos, 1.f);
            //std::cout << "pos2 - " << pos << std::endl;

            const auto length_required = pixel_length * pos;
            //std::cout << "length_required - " << length_required << std::endl;

            const auto slider_curve_smooth_lines_list = read<uint32_t>(slider_curve_smooth_lines_ptr + 0x4);
            const auto slider_cumulative_lengths_list = read<uint32_t>(slider_cumulative_lengths_ptr + 0x4);

            const auto end = read<double>(slider_cumulative_lengths_list + 0x8 + (slider_cumulative_lengths_count - 1) * 0x8);
            //std::cout << "end - " << end << std::endl;
            if (length_required >= end)
            {
                const auto slider_curve_last_line = read<uint32_t>(slider_curve_smooth_lines_list + 0x8 + slider_curve_smooth_lines_count * 0x4);
                return std::make_pair<float, float>(read<float>(slider_curve_last_line + 0x10), read<float>(slider_curve_last_line + 0x14));
            }
            
            auto similar_length = 999.0;
            auto similar_idx = 0;
            for (auto i = 0; i < slider_cumulative_lengths_count; i++)
            {
                const auto slider_cumulative_length = read<double>(slider_cumulative_lengths_list + 0x8 + i * 0x8);
                if (similar_length > abs(slider_cumulative_length - length_required))
                {
                    similar_length = abs(slider_cumulative_length - length_required);
                    similar_idx = i;
                }
            }

            const auto slider_curve_line = read<uint32_t>(slider_curve_smooth_lines_list + 0x8 + similar_idx * 0x4);

            const auto length_next = read<double>(slider_cumulative_lengths_list + 0x8 + similar_idx * 0x8);
            const auto length_previous = similar_idx ? read<double>(slider_cumulative_lengths_list + 0x8 + similar_idx - 1 * 0x8) : 0.0;
            const auto length_adjustment = (length_required - length_previous) / (length_next - length_previous);
            auto final_pos = std::make_pair<float, float>(read<float>(slider_curve_line + 0x8), read<float>(slider_curve_line + 0xC));
            const auto final_pos2 = std::make_pair<float, float>(read<float>(slider_curve_line + 0x10), read<float>(slider_curve_line + 0x14));
            /*final_pos.first += (final_pos2.first - final_pos.first) * length_adjustment;
            final_pos.second += (final_pos2.second - final_pos.second) * length_adjustment;*/
            final_pos.first += (final_pos2.first - final_pos.first) * length_adjustment;
            final_pos.second += (final_pos2.second - final_pos.second) * length_adjustment;
            return final_pos;
        }
    }

	bool get_hit_state() const
    {
        if (object_type_ == hit_object_type::slider)
            return read<bool>(read<uint32_t>(base_ + 0xCC) + 0x84);
		return read<bool>(base_ + 0x84);
    }

	static float get_radius(const std::pair<float, float> pf_size, const float circle_size)
    {
        return (pf_size.first / 8 * (1 - 0.7 * ((circle_size - 5) / 5))) / 2 / (pf_size.second / 384) * 1.00041f;
    }
};
