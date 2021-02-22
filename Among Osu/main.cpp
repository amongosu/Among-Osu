#include <random>
#include <thread>


#include "hit_object.hpp"
#include "player.hpp"
#include "viewport.hpp"
#include "utils/memory.hpp"
#include "library/interception.h"

float get_distance(const std::pair<float, float> p1, const std::pair<float, float> p2)
{
    return sqrtf(powf(p1.first - p2.first, 2) + powf(p1.second - p2.second, 2));
}

bool settings_bot = true;
const unsigned char sig_viewport[] = { 0x89, 0x45, 0xC8, 0x8B, 0x72, 0x0C, 0x8B, 0x15 }; //credit: https://github.com/mrflashstudio/osu-rx/blob/master/OsuManager/Memory/Signatures.cs
const unsigned char sig_time[] = { 0x7E, 0x55, 0x8B, 0x76, 0x10, 0xDB, 0x05 };
const unsigned char sig_player[] = { 0xFF, 0x50, 0x0C, 0x8B, 0xD8, 0x8B, 0x15 };
bool g_wait_for_input = false;
InterceptionDevice g_queued_input_device{};
InterceptionStroke g_queued_input_stroke{};

int main() {
    memory::initialize();

    const auto _viewport_offset = read<uint32_t>(find_pattern(sig_viewport, "xxxxxxxx", 8));
    const auto _time_offset = read<uint32_t>(find_pattern(sig_time, "xxxxxxx", 7));
    const auto _player_offset = read<uint32_t>(find_pattern(sig_player, "xxxxxxx", 7));
    std::cout << "[+] _viewport_offset: " << std::hex << _viewport_offset << std::dec << std::endl;
    std::cout << "[+] _time_offset: " << std::hex << _time_offset << std::dec << std::endl;
    std::cout << "[+] _player_offset: " << std::hex << _player_offset << std::dec << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> pre_randomizer(-11, 18);
    std::uniform_int_distribution<> post_randomizer(38, 58);
    
    InterceptionDevice device{};
    InterceptionStroke stroke{};

    auto *const context = interception_create_context();
	
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);

    while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
    {
        if (interception_is_keyboard(device))
        {
            g_queued_input_device = device;
            const auto key_code = reinterpret_cast<InterceptionKeyStroke*>(&stroke)->code;
        	
        	if (key_code == 2 || key_code == 3)
        	{
                if (reinterpret_cast<InterceptionKeyStroke*>(&stroke)->state)
                    continue;
                const auto _viewport = viewport(read<uint32_t>(_viewport_offset));
                const auto _time = read<int>(_time_offset);
                const auto _player = player(read<uint32_t>(_player_offset));
                if (_player.is_loaded())
                {
                    const auto hit_object_ = _player.get_current_hit_object(_time);
                    if (hit_object_.is_valid())
                    {
                        const auto time_info = hit_object_.get_time_info();
                        {
                            if (!g_wait_for_input)
                            {
                                reinterpret_cast<InterceptionKeyStroke*>(&g_queued_input_stroke)->state = reinterpret_cast<InterceptionKeyStroke*>(&stroke)->state;
                                reinterpret_cast<InterceptionKeyStroke*>(&g_queued_input_stroke)->code = reinterpret_cast<InterceptionKeyStroke*>(&stroke)->code;
                                reinterpret_cast<InterceptionKeyStroke*>(&g_queued_input_stroke)->information = reinterpret_cast<InterceptionKeyStroke*>(&stroke)->information;
                                g_wait_for_input = true;
                                std::thread wait_and_input([&]
                                    {
                                        const auto pre_sleep = (time_info.first - pre_randomizer(rd)) - _time;
                                        if (pre_sleep > 0)
											Sleep(pre_sleep);
                                        interception_send(context, g_queued_input_device, &g_queued_input_stroke, 1);
                                        const auto post_sleep = (time_info.second + post_randomizer(rd)) - time_info.first;
                                        if (post_sleep > 0)
											Sleep(post_sleep);
                                        reinterpret_cast<InterceptionKeyStroke*>(&g_queued_input_stroke)->state = 1;
                                        interception_send(context, g_queued_input_device, &g_queued_input_stroke, 1);
                                        g_wait_for_input = false;
                                    });

                                wait_and_input.detach();
                            }
                        }
                    }
                }

                continue;
        	}
            if (key_code == 57) // space bar
                break;

            interception_send(context, device, &stroke, 1);
        }

    	/*
        if (interception_is_mouse(device))
        {
            auto& mouse_stroke = *reinterpret_cast<InterceptionMouseStroke*>(&stroke);

            const auto _viewport = viewport(read<uint32_t>(_viewport_offset));
            const auto _time = read<uint32_t>(_time_offset);
            const auto _player = player(read<uint32_t>(_player_offset));
            if (_player.is_loaded())
            {
                const auto hit_object_ = _player.get_current_hit_object(_time);
                if (hit_object_.is_valid())
                {
                	switch (hit_object_.get_object_type())
                	{
                    case hit_object_type::circle:
	                    {
                        const auto object_pos = hit_object_.get_position();
                        const auto object_pos_screen = _viewport.playfield_to_screen(object_pos);
                        const auto object_radius = hit_object_.get_radius(_viewport.get_playfield_size(), _player.get_beatmap().get_circle_size());

                        POINT cur_point{};
                        GetCursorPos(&cur_point);
                        const auto dist_from_cursor = get_distance(object_pos_screen, std::make_pair<float, float>(
																		static_cast<float>(cur_point.x), 
																		static_cast<float>(cur_point.y)));
                    		
                    	if (settings_bot)
                        {
                            mouse_stroke.x = object_pos_screen.first - cur_point.x;
                            mouse_stroke.y = object_pos_screen.second - cur_point.y;
                            break;
                        }
                        {
                            if (dist_from_cursor > object_radius)
                            {
                                if ((object_pos_screen.first - cur_point.x) > 0) // 좌 -> 우
                                {
                                    if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                    {
                                        //mouse_stroke.x = accelerator(gen);
                                    }
                                    else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                    {
                                        mouse_stroke.x = -decelerator(gen);
                                    }
                                }
                                else // 우 -> 좌
                                {
                                    if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                    {
                                        mouse_stroke.x = decelerator(gen);
                                    }
                                    else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                    {
                                        //mouse_stroke.x = -accelerator(gen);
                                    }
                                }

                                if ((object_pos_screen.second - cur_point.y) > 0) // 상 -> 하
                                {
                                    if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                    {
                                        //mouse_stroke.y = accelerator(gen);
                                    }
                                    else if (mouse_stroke.y < 0) // 위로 이동할경우
                                    {
                                        mouse_stroke.y = -decelerator(gen);
                                    }
                                }
                                else // 하 -> 상
                                {
                                    if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                    {
                                        mouse_stroke.y = decelerator(gen);
                                    }
                                    else if (mouse_stroke.y < 0) // 위로 이동할경우
                                    {
                                        //mouse_stroke.y = -accelerator(gen);
                                    }
                                }
                            }
                        }
	                    }
                        break;
                    case hit_object_type::slider:
	                    {
                        const auto object_pos = hit_object_.get_position(_time);
                        const auto object_pos_screen = _viewport.playfield_to_screen(object_pos);
                        const auto object_radius = hit_object_.get_radius(_viewport.get_playfield_size(), _player.get_beatmap().get_circle_size());

                        POINT cur_point{};
                        GetCursorPos(&cur_point);
                        const auto dist_from_cursor = get_distance(object_pos_screen, std::make_pair<float, float>(
                                                                        static_cast<float>(cur_point.x),
                                                                        static_cast<float>(cur_point.y)));
                    		
                        if (settings_bot)
                        {
                            mouse_stroke.x = object_pos_screen.first - cur_point.x;
                            mouse_stroke.y = object_pos_screen.second - cur_point.y;
                            break;
                        }
                        {
                            if (dist_from_cursor > object_radius)
                            {
                                if ((object_pos_screen.first - cur_point.x) > 0) // 좌 -> 우
                                {
                                    if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                    {
                                        //mouse_stroke.x = accelerator(gen);
                                    }
                                    else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                    {
                                        mouse_stroke.x = -decelerator(gen);
                                    }
                                }
                                else // 우 -> 좌
                                {
                                    if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                    {
                                        mouse_stroke.x = decelerator(gen);
                                    }
                                    else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                    {
                                        //mouse_stroke.x = -accelerator(gen);
                                    }
                                }

                                if ((object_pos_screen.second - cur_point.y) > 0) // 상 -> 하
                                {
                                    if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                    {
                                        //mouse_stroke.y = accelerator(gen);
                                    }
                                    else if (mouse_stroke.y < 0) // 위로 이동할경우
                                    {
                                        mouse_stroke.y = -decelerator(gen);
                                    }
                                }
                                else // 하 -> 상
                                {
                                    if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                    {
                                        mouse_stroke.y = decelerator(gen);
                                    }
                                    else if (mouse_stroke.y < 0) // 위로 이동할경우
                                    {
                                        //mouse_stroke.y = -accelerator(gen);
                                    }
                                }
                            }
                        }
	                    }
                        break;
                    default:
                        break;
                	}
                }
        	}
        	
            interception_send(context, device, &stroke, 1);
        }
        */
    }

    interception_destroy_context(context);

    return 0;
}
