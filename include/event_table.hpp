#pragma once

enum class custom_event_table : int {
    TRACK_DATA_UPDATED = 512,
    TOGGLED_PLAYLIST = 513,
    CHANGED_VOLUME_PERC = 514
};

constexpr int operator+(const custom_event_table& e) { return static_cast<int>(e); }