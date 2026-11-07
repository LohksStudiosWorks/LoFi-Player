#pragma once

#include <LSWE/lswe.hpp>
#include <thread>

class DisplayHandle {
public:
    DisplayHandle();
    ~DisplayHandle();

    DisplayHandle& operator<<(ALLEGRO_EVENT_SOURCE* append);

    operator ALLEGRO_EVENT_SOURCE*() const;
private:
    void thr_update_display();

    LSWE::Events::EventQueue m_waiter;
    LSWE::Events::UserEvent m_wrapper;

    std::thread m_thread;
    bool m_running = true;
};