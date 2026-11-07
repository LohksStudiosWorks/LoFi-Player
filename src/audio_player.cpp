#include "audio_player.hpp"
#include "curl_wrapper.hpp"
#include <cmath>

AudioPlayer::AudioPlayer()
    : m_device(LSWE::Audio::Voice::create()),
    m_mixer(LSWE::Audio::Mixer::create())
{
    m_device << m_mixer;
}

AudioPlayer::~AudioPlayer() {
    m_playing.reset();
    m_data.clear();
    m_data_next.clear();
    m_last_uri.clear();
    m_next_uri.clear();
    m_mixer.destroy();
    m_device.destroy();
}

void AudioPlayer::play(const std::string uri, double in_sec) {
    if (uri.empty()) {
        printf("URI empty\n");
        m_playing.reset();
        m_last_uri.clear();
    }
    else if (!m_playing || m_last_uri != uri) {
        const double track_time = al_get_time();

        m_playing.reset();

        if (m_next_uri == uri) {
            m_data = std::move(m_data_next);
            m_next_uri.clear();
            printf("Got cached '%s'.\n", uri.c_str());
        }
        else {
            m_data = download(uri);
            printf("Raw download '%s'.\n", uri.c_str());
        }

        if (m_data.empty()) {
            return;
        }

        m_last_uri = uri;
        m_playing = std::make_unique<playing_data>(m_data.data(), m_data.size());
        m_mixer << m_playing->stream;
        m_playing->stream.rewind();
        m_playing->stream.seek_seconds(in_sec);// + (al_get_time() - track_time));
        m_playing->stream.set_playing(true);
    }
    else if (m_playing) {
        if (fabs(in_sec - static_cast<double>(m_playing->stream.get_position_seconds())) > 0.3) {
            printf("Resync %.1lf <= %.1lf\n", in_sec, m_playing->stream.get_position_seconds());
            m_playing->stream.rewind();
            m_playing->stream.seek_seconds(in_sec);
        }
    }
}

void AudioPlayer::prepare_next(const std::string uri) {
    if (uri.empty()) {
        m_next_uri.clear();
        m_data_next.clear();
    }
    else if (m_next_uri != uri) {
        m_next_uri.clear();
        m_data_next = download(uri);
        if (!m_data_next.empty()) {
            m_next_uri = uri;
            printf("Preload '%s' done.\n", uri.c_str());
        }
    }
}

void AudioPlayer::set_volume(int vol) {
    if (vol < 0 || vol > 100) return;
    m_vol = 0.01f * vol;
    m_mixer.set_gain(m_vol);
}

AudioPlayer::playing_data::playing_data(const char* buf, const size_t len)
    : stream(LSWE::Audio::Stream::load(LSWE::Utility::File::open_mem((void*)buf, len, "r"), ".ogg"))
{}
