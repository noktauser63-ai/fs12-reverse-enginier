// p1d: FS12 ses kapsayicisi.
//   u32 type      (2 = RIFF/WAVE, 5 = OggVorbis)
//   u32 hdrSize   (hep 0; oyun payload'u data + 8 + hdrSize olarak hesapliyor)
//   payload       (ham WAV veya OGG dosyasi)
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace fs12 {
namespace audio {

enum P1DType : uint32_t { P1D_WAV = 2, P1D_OGG = 5 };

struct P1D {
    uint32_t type = 0;
    const uint8_t* payload = nullptr;
    size_t payloadSize = 0;
};

bool p1dParse(const uint8_t* data, size_t size, P1D& out);

// Kod cozulmus, mono veya stereo, int16 PCM.
struct Pcm {
    int channels = 0;
    int sampleRate = 0;
    std::vector<int16_t> samples;  // interleaved
    size_t frames() const { return channels ? samples.size() / channels : 0; }
};

// WAV (PCM8/PCM16/MS-IMA-ADPCM) -> Pcm
bool wavDecode(const uint8_t* data, size_t size, Pcm& out);

}  // namespace audio
}  // namespace fs12
