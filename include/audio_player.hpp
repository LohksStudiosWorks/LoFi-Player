#pragma once

#include <LSWE/lswe.hpp>
#include <string>
#include <memory>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    void play(const std::string uri, double in_sec);
    void prepare_next(const std::string uri);
    void set_volume(int vol);
private:
    struct playing_data {
        LSWE::Utility::File file;
        LSWE::Audio::Sample sample;
        LSWE::Audio::SampleInstance instance;
        playing_data(const char* buf, const size_t len);
    };

    LSWE::Audio::Voice m_device;
    LSWE::Audio::Mixer m_mixer;
    float m_vol = 1.0f;
    std::string m_data, m_data_next, m_last_uri, m_next_uri;
    std::unique_ptr<playing_data> m_playing;

};