#include "display_handle.hpp"
#include "track_handler.hpp"
#include "event_table.hpp"

#include <battery/embed.hpp>

DisplayHandle::DisplayHandle() 
    : m_waiter(LSWE::Events::EventQueue::create()),
    m_wrapper(LSWE::Events::UserEvent::create()),
    m_thread([this]{thr_update_display();})
{}

DisplayHandle::~DisplayHandle() {
    if (m_running) {
        m_running = false;
        m_thread.join();
    }
    m_waiter.destroy();
}

DisplayHandle& DisplayHandle::operator<<(ALLEGRO_EVENT_SOURCE* append) {
    m_waiter << append;
    return *this;
}

DisplayHandle::operator ALLEGRO_EVENT_SOURCE*() const {
    return m_wrapper;
}

void DisplayHandle::thr_update_display() {
    using namespace LSWE::Graphics;
    using namespace LSWE::Utility;
    using namespace LSWE::Events;

    const auto font_embedded = b::embed<"resources/calibri_fix.ttf">();
    auto font_fp = File::open_mem((void*)font_embedded.data(), font_embedded.length(), "rb");

    const auto image_embedded = b::embed<"resources/template.png">();
    auto image_fp = File::open_mem((void*)image_embedded.data(), image_embedded.length(), "rb");

    Display::set_new_flags(ALLEGRO_FRAMELESS);
    auto disp = Display::create(600, 200);    
    auto font = Font::load_ttf(std::move(font_fp), 24);
    auto bmp = Bitmap::load(image_fp, ".png");
    auto timer = Timer::create(1.0 / 30);

    m_waiter << MouseEventSource();
    m_waiter << m_wrapper;
    m_waiter << timer;

    constexpr uint32_t offx_text = 3;
    constexpr uint32_t offy_text = 2;


    constexpr int toggle_lims[4] = {
        11, 158, 363, 184
    };
    constexpr int vol_lims[4] = {
        407, 169, 590, 175 
    };

    const auto is_in_lim = [](const Mouse& m, const int (&lims)[4]) {
        return m.get_axis(0) >= lims[0] && m.get_axis(0) <= lims[2]
            && m.get_axis(1) >= lims[1] && m.get_axis(1) <= lims[3];
    };

    float volume = 1.0f;
    int mx = -1, my = -1;
    int wx, wy;

    std::string album_cpy = "Loading...";
    std::string text_cpy = "Loading...";
   
    timer.start();
    
    while(m_running) {
        auto ev = m_waiter.wait();

        switch(ev.type) {
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        {
            Mouse mouse;
            if (mouse.get_down(1)) {
                if (mouse.get_axis(1) <= 20) {
                    if (mouse.get_axis(0) >= 580) {
                        m_wrapper.emit(ALLEGRO_EVENT{ .type = ALLEGRO_EVENT_DISPLAY_CLOSE }, nullptr);
                    }
                    else {
                        disp.get_window_position(wx, wy);
                        if (!Mouse::get_cursor_position(mx, my)) {
                            mx = my = -1;
                        }
                    }
                }
                else if (is_in_lim(mouse, toggle_lims)) {
                    m_wrapper.emit(ALLEGRO_EVENT{ .type = +custom_event_table::TOGGLED_PLAYLIST }, nullptr);
                }
                else if (is_in_lim(mouse, vol_lims)) {
                    volume = static_cast<int>(
                        100.0f * static_cast<float>(mouse.get_axis(0) - vol_lims[0]) / (vol_lims[2] - vol_lims[0])
                    ) * 0.01f;
                    
                    ALLEGRO_EVENT cev;
                    cev.type = +custom_event_table::CHANGED_VOLUME_PERC;
                    cev.user.data1 = static_cast<intptr_t>(100.0f * volume);
                    m_wrapper.emit(cev, nullptr);
                }
            }
        }
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        {
            Mouse mouse;
            if (!mouse.get_down(1)) {
                mx = my = -1;
            }
        }
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
        {
            Mouse mouse;
            if (mouse.get_down(1)) {
                if (mx != -1 && my != -1) {
                    int x, y;
                    if (!Mouse::get_cursor_position(x, y)) {
                        mx = my = -1;
                    }
                    else {
                        disp.set_window_position(
                            wx + (x - mx),
                            wy + (y - my)
                        );
                    }
                }
            }
        }
            break;
        case +custom_event_table::TRACK_DATA_UPDATED:
        {
            const auto* i = (TrackHandler::track_info*)ev.user.data2;
            const auto* s = (std::string*)ev.user.data4;

            if (!s || !i) continue;
            
            text_cpy = (i->time_total_str + " :: " + i->name + " :: " + i->time_str);
            album_cpy = *s;
        }
            break;
        case ALLEGRO_EVENT_TIMER:
        {
            Color(25,25,25).clear_to_this();

            constexpr int max_w = 580;
            const int text_w = font.get_width(text_cpy.c_str());
            const float diff = static_cast<float>(text_w - max_w) + 5.0f;

            float scroll_x = 0.0f;
            if (diff > 0) {
                double current_time = al_get_time();
                
                constexpr float pause_start = 1.0f;   // Seconds to pause at the beginning (0 offset)
                constexpr float speed = 90.0f;        // Scroll speed in pixels per second
                constexpr float pause_end = 1.5f;     // Seconds to pause at the end (fully scrolled)
                
                const float scroll_duration = diff / speed;
                const float total_cycle = pause_start + scroll_duration + pause_end;
                
                const float t = fmod(current_time, total_cycle);
                
                if (t < pause_start) {
                    scroll_x = 0.0f;
                } 
                else if (t < pause_start + scroll_duration) {
                    const float elapsed_scroll = t - pause_start;
                    scroll_x = elapsed_scroll * speed;
                } 
                else {
                    scroll_x = diff;
                }
            }

            font.draw(Color(255,255,255), 11 + offx_text - scroll_x, 124 + offy_text, 0, text_cpy.c_str());

            font.draw(Color(255,255,255), 11 + offx_text, 158 + offy_text, 0, fmod(al_get_time() + 20.0, 30.0) < 5.0 ? "<Click to toggle!>" : album_cpy.c_str());

            {
                constexpr float off = vol_lims[2] - vol_lims[0];
                al_draw_filled_rectangle(vol_lims[0], vol_lims[1], vol_lims[0] + volume * off, vol_lims[3], al_map_rgb(25, 195, 125));
            }

            bmp.draw(0, 0, 0);

            disp.flip();
        }
            break;
        }

    }


}