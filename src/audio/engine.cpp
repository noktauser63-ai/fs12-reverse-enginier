#include "engine.h"

#include <math.h>
#include <string.h>

#include "stb_vorbis_cfg.h"

namespace fs12 {
namespace audio {

static inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 4.f ? 4.f : v); }

Engine::Engine() { sources_.reserve(256); }

Engine::~Engine() { musicClose(); }

// ---------------- tamponlar ----------------
BufferId Engine::createBuffer(const uint8_t* data, size_t size) {
    P1D p;
    if (!p1dParse(data, size, p) || p.type != P1D_WAV) return kInvalid;
    Pcm pcm;
    if (!wavDecode(p.payload, p.payloadSize, pcm)) return kInvalid;
    std::lock_guard<std::mutex> l(mu_);
    for (size_t i = 0; i < buffers_.size(); ++i) {
        if (!buffers_[i].used) {
            buffers_[i].used = true;
            buffers_[i].pcm = std::move(pcm);
            return (BufferId)i;
        }
    }
    buffers_.emplace_back();
    buffers_.back().used = true;
    buffers_.back().pcm = std::move(pcm);
    return (BufferId)(buffers_.size() - 1);
}

void Engine::destroyBuffer(BufferId id) {
    std::lock_guard<std::mutex> l(mu_);
    if (id < 0 || id >= (int)buffers_.size()) return;
    for (auto& s : sources_)
        if (s.used && s.buf == id) s.state = Source::Stopped;
    buffers_[id].used = false;
    buffers_[id].pcm = Pcm();
}

// ---------------- kaynaklar ----------------
SourceId Engine::createSource(BufferId buf, Priority) {
    std::lock_guard<std::mutex> l(mu_);
    for (size_t i = 0; i < sources_.size(); ++i) {
        if (!sources_[i].used) {
            sources_[i] = Source();
            sources_[i].used = true;
            sources_[i].buf = buf;
            return (SourceId)i;
        }
    }
    sources_.emplace_back();
    sources_.back().used = true;
    sources_.back().buf = buf;
    return (SourceId)(sources_.size() - 1);
}

#define SRC_OR_RETURN(ret)                                       \
    if (id < 0 || id >= (int)sources_.size() || !sources_[id].used) return ret; \
    Source& s = sources_[id];

void Engine::destroySource(SourceId id) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    s = Source();
}

bool Engine::play(SourceId id, float volume, float pitch, bool loop) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN(false);
    if (s.buf < 0 || s.buf >= (int)buffers_.size() || !buffers_[s.buf].used) return false;
    s.volume = s.lastGain = clamp01(volume);
    s.pitch = pitch > 0.01f ? (pitch > 8.f ? 8.f : pitch) : 0.01f;
    s.loop = loop;
    s.pos = 0.0;
    s.state = Source::Playing;
    return true;
}

void Engine::stop(SourceId id) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    s.state = Source::Stopped;
    s.pos = 0.0;
}
void Engine::pause(SourceId id) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    if (s.state == Source::Playing) s.state = Source::Paused;
}
void Engine::resume(SourceId id) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    if (s.state == Source::Paused) s.state = Source::Playing;
}
void Engine::setVolume(SourceId id, float v) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    s.volume = clamp01(v);
}
void Engine::setPitch(SourceId id, float p) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN();
    s.pitch = p > 0.01f ? (p > 8.f ? 8.f : p) : 0.01f;
}
bool Engine::isPlaying(SourceId id) {
    std::lock_guard<std::mutex> l(mu_);
    SRC_OR_RETURN(false);
    return s.state == Source::Playing;
}

// ---------------- muzik ----------------
bool Engine::musicOpen(const uint8_t* data, size_t size) {
    P1D p;
    if (!p1dParse(data, size, p) || p.type != P1D_OGG) return false;
    std::lock_guard<std::mutex> l(mu_);
    if (music_.v) {
        stb_vorbis_close(music_.v);
        music_.v = nullptr;
    }
    music_.file.assign(p.payload, p.payload + p.payloadSize);
    int err = 0;
    music_.v = stb_vorbis_open_memory(music_.file.data(), (int)music_.file.size(), &err, nullptr);
    if (!music_.v) return false;
    stb_vorbis_info info = stb_vorbis_get_info(music_.v);
    music_.channels = info.channels > 2 ? 2 : info.channels;
    music_.rate = (int)info.sample_rate;
    music_.blk.clear();
    music_.blkFrames = 0;
    music_.pos = 0.0;
    music_.playing = music_.paused = music_.finished = false;
    return true;
}

void Engine::musicClose() {
    std::lock_guard<std::mutex> l(mu_);
    if (music_.v) stb_vorbis_close(music_.v);
    music_.v = nullptr;
    music_.file.clear();
    music_.file.shrink_to_fit();
    music_.playing = music_.paused = false;
}

void Engine::musicPlay(bool loop) {
    std::lock_guard<std::mutex> l(mu_);
    if (!music_.v) return;
    stb_vorbis_seek_start(music_.v);
    music_.blkFrames = 0;
    music_.pos = 0.0;
    music_.loop = loop;
    music_.finished = false;
    music_.paused = false;
    music_.playing = true;
    music_.lastGain = music_.volume;
}
void Engine::musicPause() {
    std::lock_guard<std::mutex> l(mu_);
    if (music_.playing) music_.paused = true;
}
void Engine::musicResume() {
    std::lock_guard<std::mutex> l(mu_);
    music_.paused = false;
}
void Engine::musicStop() {
    std::lock_guard<std::mutex> l(mu_);
    music_.playing = false;
    music_.paused = false;
}
void Engine::musicSetVolume(float v) {
    std::lock_guard<std::mutex> l(mu_);
    music_.volume = clamp01(v);
}
float Engine::musicVolume() {
    std::lock_guard<std::mutex> l(mu_);
    return music_.volume;
}
bool Engine::musicFinished() {
    std::lock_guard<std::mutex> l(mu_);
    return music_.finished;
}
bool Engine::musicActive() {
    std::lock_guard<std::mutex> l(mu_);
    return music_.v && music_.playing && !music_.paused;
}

void Engine::setMasterVolume(float v) {
    std::lock_guard<std::mutex> l(mu_);
    master_ = clamp01(v);
}
float Engine::masterVolume() {
    std::lock_guard<std::mutex> l(mu_);
    return master_;
}

bool Engine::musicFill() {
    // Yeni blok coz (~2048 kare).
    const int kFrames = 2048;
    music_.blk.resize((size_t)kFrames * music_.channels);
    int n = stb_vorbis_get_samples_short_interleaved(music_.v, music_.channels, music_.blk.data(),
                                                     kFrames * music_.channels);
    if (n <= 0) {
        if (music_.loop) {
            stb_vorbis_seek_start(music_.v);
            n = stb_vorbis_get_samples_short_interleaved(music_.v, music_.channels, music_.blk.data(),
                                                         kFrames * music_.channels);
        }
        if (n <= 0) {
            music_.playing = false;
            music_.finished = true;
            music_.blkFrames = 0;
            return false;
        }
    }
    music_.blkFrames = (size_t)n;
    return true;
}

// ---------------- mixer ----------------
void Engine::mixSources(float* acc, int frames) {
    const float inv = 1.f / 32768.f;
    for (auto& s : sources_) {
        if (!s.used || s.state != Source::Playing) continue;
        if (s.buf < 0 || s.buf >= (int)buffers_.size() || !buffers_[s.buf].used) {
            s.state = Source::Stopped;
            continue;
        }
        const Pcm& pcm = buffers_[s.buf].pcm;
        const size_t total = pcm.frames();
        if (total < 2) {
            s.state = Source::Stopped;
            continue;
        }
        const double step = (double)s.pitch * pcm.sampleRate / kOutRate;
        const float g0 = s.lastGain, g1 = s.volume;
        const int ch = pcm.channels;
        for (int i = 0; i < frames; ++i) {
            size_t i0 = (size_t)s.pos;
            if (i0 >= total) {
                if (s.loop) {
                    s.pos = fmod(s.pos, (double)total);
                    i0 = (size_t)s.pos;
                } else {
                    s.state = Source::Stopped;
                    s.pos = 0.0;
                    break;
                }
            }
            size_t i1 = i0 + 1;
            if (i1 >= total) i1 = s.loop ? 0 : i0;
            float f = (float)(s.pos - (double)i0);
            float gain = g0 + (g1 - g0) * ((float)i / (float)frames);
            float l, r;
            if (ch == 1) {
                float a = pcm.samples[i0], b = pcm.samples[i1];
                l = r = (a + (b - a) * f) * inv;
            } else {
                float a0 = pcm.samples[i0 * 2], b0 = pcm.samples[i1 * 2];
                float a1 = pcm.samples[i0 * 2 + 1], b1 = pcm.samples[i1 * 2 + 1];
                l = (a0 + (b0 - a0) * f) * inv;
                r = (a1 + (b1 - a1) * f) * inv;
            }
            acc[i * 2] += l * gain;
            acc[i * 2 + 1] += r * gain;
            s.pos += step;
        }
        s.lastGain = g1;
    }
}

void Engine::mixMusic(float* acc, int frames) {
    Music& m = music_;
    if (!m.v || !m.playing || m.paused) return;
    const float inv = 1.f / 32768.f;
    const double step = (double)m.rate / kOutRate;
    const float g0 = m.lastGain, g1 = m.volume;
    for (int i = 0; i < frames; ++i) {
        size_t i0 = (size_t)m.pos;
        while (i0 + 1 >= m.blkFrames) {
            // blok bitti: kalan konumu koruyarak yeni blok al
            size_t used = m.blkFrames;
            // son kareyi interpolasyon icin tasi
            int16_t last[2] = {0, 0};
            if (used > 0)
                for (int c = 0; c < m.channels; ++c) last[c] = m.blk[(used - 1) * m.channels + c];
            if (!musicFill()) return;
            // yeni blogun basina bir onceki bloktan son kareyi ekle
            m.blk.insert(m.blk.begin(), last, last + m.channels);
            m.blkFrames += 1;
            m.pos = (used > 0) ? (m.pos - (double)(used - 1)) : 0.0;
            if (m.pos < 0) m.pos = 0;
            i0 = (size_t)m.pos;
        }
        float f = (float)(m.pos - (double)i0);
        float gain = g0 + (g1 - g0) * ((float)i / (float)frames);
        float l, r;
        if (m.channels == 1) {
            float a = m.blk[i0], b = m.blk[i0 + 1];
            l = r = (a + (b - a) * f) * inv;
        } else {
            float a0 = m.blk[i0 * 2], b0 = m.blk[(i0 + 1) * 2];
            float a1 = m.blk[i0 * 2 + 1], b1 = m.blk[(i0 + 1) * 2 + 1];
            l = (a0 + (b0 - a0) * f) * inv;
            r = (a1 + (b1 - a1) * f) * inv;
        }
        acc[i * 2] += l * gain;
        acc[i * 2 + 1] += r * gain;
        m.pos += step;
    }
    m.lastGain = g1;
}

void Engine::render(int16_t* out, int frames) {
    std::lock_guard<std::mutex> l(mu_);
    if ((int)acc_.size() < frames * 2) acc_.resize((size_t)frames * 2);
    memset(acc_.data(), 0, sizeof(float) * (size_t)frames * 2);
    mixSources(acc_.data(), frames);
    mixMusic(acc_.data(), frames);
    const float m = master_;
    for (int i = 0; i < frames * 2; ++i) {
        float v = acc_[i] * m;
        // yumusak sinirlama: |v|>0.9 uzerinde tanh benzeri
        if (v > 0.9f) v = 0.9f + 0.1f * tanhf((v - 0.9f) * 10.f);
        else if (v < -0.9f) v = -0.9f + 0.1f * tanhf((v + 0.9f) * 10.f);
        int s = (int)lrintf(v * 32767.f);
        out[i] = (int16_t)(s > 32767 ? 32767 : (s < -32768 ? -32768 : s));
    }
}

}  // namespace audio
}  // namespace fs12
