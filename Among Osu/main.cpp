#include "hit_object.hpp"
#include "player.hpp"
#include "viewport.hpp"
#include "utils/memory.hpp"
#include "library/interception.h"

float get_distance(const std::pair<float, float> p1, const std::pair<float, float> p2)
{
    return sqrtf(powf(p1.first - p2.first, 2) + powf(p1.second - p2.second, 2));
}

int main() {
    memory::initialize();
    
    InterceptionDevice device{};
    InterceptionStroke stroke{};

    auto *const context = interception_create_context();
	
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
    interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE);

    while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
    {
        if (interception_is_keyboard(device))
        {
            interception_send(context, device, &stroke, 1);

            if (reinterpret_cast<InterceptionKeyStroke*>(&stroke)->code == 57) // space bar
                break;
        }
    	
        if (interception_is_mouse(device))
        {
            auto& mouse_stroke = *reinterpret_cast<InterceptionMouseStroke*>(&stroke);

            const auto _viewport = viewport(read<uint32_t>(0x03E838F4));
            const auto _time = read<uint32_t>(0x14267A0);
            const auto _player = player(read<uint32_t>(0x03E83C40));
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
	                                                                   static_cast<float>(cur_point.x), static_cast<float>(cur_point.y)));
                        if (dist_from_cursor > object_radius)
                        {
                            if ((object_pos_screen.first - cur_point.x) > 0) // 좌 -> 우
                            {
                                if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                {
                                    mouse_stroke.x *= 2;
                                }
                                else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                {
                                    mouse_stroke.x = 0;
                                }
                            }
                            else // 우 -> 좌
                            {
                                if (mouse_stroke.x > 0)      // 오른쪽으로 이동할경우
                                {
                                    mouse_stroke.x = 0;
                                }
                                else if (mouse_stroke.x < 0) // 왼쪽으로 이동할경우
                                {
                                    mouse_stroke.x *= 2;
                                }
                            }
                        	
                            if ((object_pos_screen.second - cur_point.y) > 0) // 상 -> 하
                            {
                                if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                {
                                    mouse_stroke.y *= 2;
                                }
                                else if (mouse_stroke.y < 0) // 위로 이동할경우
                                {
                                    mouse_stroke.y = 0;
                                }
                            }
                            else // 하 -> 상
                            {
                                if (mouse_stroke.y > 0)      // 아래로 이동할경우
                                {
                                    mouse_stroke.y = 0;
                                }
                                else if (mouse_stroke.y < 0) // 위로 이동할경우
                                {
                                    mouse_stroke.y *= 2;
                                }
                            }
                        }
	                    }
                        break;
                    case hit_object_type::slider:
                        break;
                    default:
                        break;
                	}
                }
        	}
        	
            interception_send(context, device, &stroke, 1);
        }
    }

    interception_destroy_context(context);

    return 0;
}
