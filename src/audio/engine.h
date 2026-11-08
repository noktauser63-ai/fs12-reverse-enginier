// FS12 ses motoru (FMOD'suz). Mixer + muzik akisi. Cikis katmani (OpenSL ES) ayri dosyada.
#pragma once
#include <stdint.h>
#include <memory>
#include <mutex>
#include <vector>

#include "p1d.h"

struct stb_vorbis;

namespace fs12 {
namespace audio {

typedef int BufferId;
typedef int SourceId;
static const int kInvalid = -1;

enum class Priority : int { Low = 0, Normal = 1, High = 2 };

class Engine {
public:
    static const int kOutRate = 44100;

    Engine();
    ~Engine();

    // ---- Efekt tamponlari (kisa WAV'lar, tamamen cozulup bellekte tutulur) ----
    // data = .p1d dosyasinin tamami (8 baytlik baslik dahil).
    BufferId createBuffer(const uint8_t* p1dData, size_t size);
    void destroyBuffer(BufferId id);

    // ---- Kaynaklar (oyundaki AudioSource'a karsilik gelir; her biri kendi sesidir) ----
    SourceId createSource(BufferId buf, Priority prio = Priority::Normal);
    void destroySource(SourceId id);
    bool play(SourceId id, float volume, float pitch, bool loop);  // bastan baslatir
    void stop(SourceId id);
    void pause(SourceId id);
    void resume(SourceId id);
    void setVolume(SourceId id, float v);
    void setPitch(SourceId id, float p);
    bool isPlaying(SourceId id);

    // ---- Arka plan muzigi (tek akis, Ogg Vorbis) ----
    bool musicOpen(const uint8_t* p1dData, size_t size);  // veriyi kopyalar
    void musicClose();
    void musicPlay(bool loop);
    void musicPause();
    void musicResume();
    void musicStop();
    void musicSetVolume(float v);
    float musicVolume();
    bool musicFinished();  // parca sonuna gelindi mi
    bool musicActive();    // acik ve calisiyor mu

    void setMasterVolume(float v);
    float masterVolume();

    // Cikis katmani cagirir: interleaved stereo int16.
    void render(int16_t* out, int frames);

private:
    struct Buffer {
        bool used = false;
        Pcm pcm;
    };
    struct Source {
        bool used = false;
        BufferId buf = kInvalid;
        enum State { Stopped, Playing, Paused } state = Stopped;
        float volume = 1.f, lastGain = 1.f, pitch = 1.f;
        bool loop = false;
        double pos = 0.0;  // kaynak ornek cinsinden
    };
    struct Music {
        std::vector<uint8_t> file;
        stb_vorbis* v = nullptr;
        int channels = 0, rate = 0;
        std::vector<int16_t> blk;  // son cozulen blok (interleaved)
        size_t blkFrames = 0;
        double pos = 0.0;  // blk icinde
        bool playing = false, paused = false, finished = false, loop = false;
        float volume = 1.f, lastGain = 1.f;
    };

    bool musicFill();  // sonraki blogu coz
    void mixSources(float* acc, int frames);
    void mixMusic(float* acc, int frames);

    std::mutex mu_;
    std::vector<Buffer> buffers_;
    std::vector<Source> sources_;
    Music music_;
    float master_ = 1.f;
    std::vector<float> acc_;
};

// Cikis arka ucu (Android'de OpenSL ES). Engine::render'i ses is parcaciginda cagirir.
class Output {
public:
    virtual ~Output() {}
    virtual bool start(Engine* engine) = 0;
    virtual void stop() = 0;
    virtual void setPaused(bool paused) = 0;
};
Output* createDefaultOutput();  // platforma gore; yoksa nullptr

}  // namespace audio
}  // namespace fs12
