#pragma once

#include <LSWE/lswe.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <memory>

class TrackHandler {
public:
    enum class state {
        LOADING_PLAYLIST,
        READY,
        FATAL_ERROR
    };
    struct track_info {
        double time;
        std::string uri;
        std::string uri_next;
        std::string name;
        std::string time_str;
        std::string time_total_str;
    };

    TrackHandler();
    ~TrackHandler();

    void start();

    ALLEGRO_EVENT_SOURCE* get_event_source();

    const std::vector<std::string>& get_playlists() const;
    void toggle_playlist();
    state get_state() const;
private:
    void thr_keep_ready();
    void update_track_info();

    LSWE::Events::UserEvent m_track_change;

    nlohmann::ordered_json m_cloud_list;
    std::vector<std::string> m_playlists;
    uint32_t m_playlist_selected = 0;

    std::thread m_async;
    track_info m_last_info;
    state m_state = state::LOADING_PLAYLIST;
    bool m_running = false;
};