#pragma once
#include <utility>


#include "beatmap.hpp"
#include "hit_object.hpp"
#include "utils/memory.hpp"

class player
{
private:
    uint32_t base_{};
    
    uint32_t hit_object_mgr_{};
    int hit_object_current_idx{};
    int hit_object_count_{};
    uint32_t hit_object_list_{};

public:
    player() = default;

    explicit player(const uint32_t base) : base_(base)
    {
        hit_object_mgr_ = read<uint32_t>(base_ + 0x40);
        hit_object_count_ = read<int>(hit_object_mgr_ + 0x90);
        hit_object_list_ = read<uint32_t>(read<uint32_t>(hit_object_mgr_ + 0x48) + 0x4);
    }

	bool is_loaded() const
    {
        return read<bool>(base_ + 0x182);
    }

	beatmap get_beatmap() const
    {
        return beatmap(read<uint32_t>(base_ + 0xD4));
    }

	hit_object get_hit_object(const int index) const
    {
        return hit_object(read<uint32_t>(hit_object_list_ + 0x8 + index * 4));
    }

	//
	// tbd: need to optimized
	//
	hit_object get_current_hit_object(const int time) const
    {
        const auto current_idx = read<int>(hit_object_mgr_ + 0x8C);
    	for (auto i = max(current_idx - 5, 0); i < min(current_idx + 5, hit_object_count_); i++)
    	{
            auto hit_object_ = get_hit_object(i);
            if (hit_object_.get_hit_state() && hit_object_.get_object_type() == hit_object_type::circle)
                continue;
            const auto time_info = hit_object_.get_time_info();
            if (time_info.first - 1000 < time && time_info.second + (hit_object_.get_object_type() == hit_object_type::circle ? 200 : 0) > time)
                return hit_object_;
    	}

        return hit_object{};
    }
};
