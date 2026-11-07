#include "track_handler.hpp"
#include "curl_wrapper.hpp"
#include "event_table.hpp"
#include <regex>
#include <cstdint>
#include <format>

static std::string sec_to_str(uint64_t sec);
static std::string sanitize_url(std::string url);
static std::string sanitize_js_to_json(const std::string& input);

TrackHandler::TrackHandler() 
    : m_track_change(LSWE::Events::UserEvent::create()),
      m_async()
{}

TrackHandler::~TrackHandler() {
    if (m_running) {
        m_running = false;
        m_async.join();
    }
}

void TrackHandler::start() {
    m_running = true;
    m_async = std::thread([this]{thr_keep_ready();});
}

ALLEGRO_EVENT_SOURCE* TrackHandler::get_event_source() {
    return m_track_change;
}

const std::vector<std::string>& TrackHandler::get_playlists() const {
    return m_playlists;
}

void TrackHandler::toggle_playlist() {
    m_playlist_selected = (m_playlist_selected + 1) % m_playlists.size();
}

void TrackHandler::thr_keep_ready() {
    ALLEGRO_EVENT ev;

    const auto emit_ev = [&ev,this]{
        ev.any.type = +custom_event_table::TRACK_DATA_UPDATED;
        ev.user.data1 = (intptr_t)this;
        ev.user.data2 = (intptr_t)&m_last_info;
        ev.user.data3 = (intptr_t)m_playlist_selected;
        ev.user.data4 = (intptr_t)&m_playlists[m_playlist_selected];

        m_track_change.emit(ev, nullptr);
    };

    auto queue = LSWE::Events::EventQueue::create();
    auto timed = LSWE::Events::Timer::create(1.0);

    queue << timed;

    timed.start();

    while(m_running) {
        queue.wait();
        
        switch (m_state) {
        case state::FATAL_ERROR:
        case state::LOADING_PLAYLIST:
        {
            auto buf = download("https://lohkat.github.io/lofi/js/times.js");
            if (buf.empty()) {
                continue;
            }

            buf = buf.substr(buf.find('['));
            while(buf.back() != ']') buf.pop_back();
            buf = sanitize_js_to_json(buf);

            m_cloud_list = nlohmann::ordered_json::parse(buf, nullptr, false);
            if (m_cloud_list.is_discarded() || m_cloud_list.is_null()) {
                m_state = state::FATAL_ERROR;
                continue;
            }

            m_playlists.clear();
            for(const auto& plists : m_cloud_list) {
                m_playlists.push_back(plists["name"].get<std::string>());
            }

            m_last_info = {
                .time = 0.0,
                .uri = "",
                .name = "Loading...",
                .time_str = "--:--:--",
                .time_total_str = "--:--:--"
            };

            emit_ev();
            m_state = state::READY;
        }
            break;
        case state::READY:
        {
            update_track_info();
            emit_ev();
        }
            break;
        }
    }
}

void TrackHandler::update_track_info() {
    track_info current;

    double time_now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    const int64_t factor = 11987;
    const auto selected = m_cloud_list[m_playlist_selected];
    const double total_time_sec = selected["total_time"].get<int64_t>() / 1000.0;

    current.time = std::fmod(time_now, total_time_sec);
    
    double show_time = 0;
    int64_t idx = 0;

    const auto tracks = selected["tracks"];

    for(int64_t i = 0; i < tracks.size(); ++i) {
        idx = (factor + idx) % tracks.size();

        double track_time = static_cast<double>(tracks[idx][1].get<int64_t>()) * 0.001;
        if (current.time > track_time) {
            current.time -= track_time;
            show_time += track_time;
        }
        else break;
    }

    const int64_t next_track_idx = (factor + idx) % tracks.size();

    const std::string raw_name = tracks[idx][0].get<std::string>();
    const std::string raw_name_next = tracks[next_track_idx][0].get<std::string>();

    constexpr char expected_beg_str[] = "tracks/";
    constexpr size_t expected_off = std::strlen(expected_beg_str);

    current.name = raw_name.substr(expected_off);
    size_t next_slash = current.name.find('/');
    current.name = current.name.substr(next_slash + 1);
    current.name = current.name.size() > 4 ? current.name.substr(0, current.name.size() - 4) : "ERR";

    current.uri = sanitize_url("https://lohkat.github.io/lofi/" + raw_name);
    current.uri_next = sanitize_url("https://lohkat.github.io/lofi/" + raw_name_next);
    current.time_str = sec_to_str(current.time);
    current.time_total_str = sec_to_str(show_time + current.time);

    m_last_info = current;
}

std::string sec_to_str(uint64_t orig) {
    const uint64_t secs = orig % 60;
    orig /= 60;
    const uint64_t mins = orig % 60;
    orig /= 60;
    const uint64_t hour = orig;

    return std::format("{:02}:{:02}:{:02}", hour, mins, secs);
}

std::string sanitize_url(std::string url) {
    size_t pos = 0;
    while ((pos = url.find(" ", pos)) != std::string::npos) {
        url.replace(pos, 1, "%20");
        pos += 3; // Move past the newly inserted "%20"
    }
    return url;
}

std::string sanitize_js_to_json(const std::string& input) {
    // 1. Wrap unquoted keys in double quotes.
    // Matches '{' or ',' followed by whitespace, an identifier, optional space, and ':'
    std::regex key_regex(R"(([{},]\s*)([a-zA-Z_]\w*)\s*:)");
    std::string cur_str = std::regex_replace(input, key_regex, "$1\"$2\":");

    // 2. Remove trailing commas. 
    // Your example has them (e.g., after the last array item), but strict JSON 
    // and nlohmann::ordered_json will throw a parse error if they are left in.
    std::regex trailing_comma(R"(,(\s*[\]}]))");
    cur_str = std::regex_replace(cur_str, trailing_comma, "$1");

    return cur_str;
}