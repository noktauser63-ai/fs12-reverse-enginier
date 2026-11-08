// Ana makinede calisan ses motoru testi: tum .p1d dosyalarini cozer, ham PCM dosyalarina yazar.
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "../src/audio/engine.h"

using namespace fs12::audio;

static std::vector<uint8_t> readAll(const std::string& p) {
    std::vector<uint8_t> v;
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return v;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    v.resize(n);
    if (fread(v.data(), 1, n, f) != (size_t)n) v.clear();
    fclose(f);
    return v;
}

int main(int argc, char** argv) {
    if (argc < 3) return 2;
    std::string in = argv[1], out = argv[2];
    DIR* d = opendir(in.c_str());
    if (!d) return 2;
    int fails = 0;
    Engine eng;
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n.size() < 5 || n.substr(n.size() - 4) != ".p1d") continue;
        std::string base = n.substr(0, n.size() - 4);
        auto data = readAll(in + "/" + n);
        P1D p;
        if (!p1dParse(data.data(), data.size(), p)) { printf("FAIL parse %s\n", n.c_str()); ++fails; continue; }
        if (p.type == P1D_WAV) {
            Pcm pcm;
            if (!wavDecode(p.payload, p.payloadSize, pcm)) { printf("FAIL wav %s\n", n.c_str()); ++fails; continue; }
            FILE* f = fopen((out + "/" + base + ".raw").c_str(), "wb");
            fwrite(pcm.samples.data(), 2, pcm.samples.size(), f);
            fclose(f);
            printf("WAV %s ch=%d rate=%d frames=%zu\n", base.c_str(), pcm.channels, pcm.sampleRate, pcm.frames());
            BufferId b = eng.createBuffer(data.data(), data.size());
            if (b < 0) { printf("FAIL createBuffer %s\n", n.c_str()); ++fails; }
        } else {
            if (!eng.musicOpen(data.data(), data.size())) { printf("FAIL music %s\n", n.c_str()); ++fails; continue; }
            eng.musicSetVolume(1.f);
            eng.musicPlay(false);
            std::vector<int16_t> all, blk(1024 * 2);
            int guard = 0;
            while (!eng.musicFinished() && guard++ < 100000 && all.size() < 44100 * 2 * 12) {
                eng.render(blk.data(), 1024);
                all.insert(all.end(), blk.begin(), blk.end());
            }
            FILE* f = fopen((out + "/" + base + ".mix.raw").c_str(), "wb");
            fwrite(all.data(), 2, all.size(), f);
            fclose(f);
            printf("OGG %s rendered frames=%zu finished=%d\n", base.c_str(), all.size() / 2, (int)eng.musicFinished());
            eng.musicClose();
        }
    }
    closedir(d);
    // Efekt miksi: iki kaynak calsin, NaN/tasma kontrolu
    {
        auto data = readAll(in + "/horn.p1d");
        BufferId b = eng.createBuffer(data.data(), data.size());
        SourceId s1 = eng.createSource(b), s2 = eng.createSource(b);
        eng.play(s1, 1.f, 1.f, true);
        eng.play(s2, 0.5f, 1.5f, false);
        std::vector<int16_t> o(512 * 2);
        long long energy = 0;
        for (int i = 0; i < 200; ++i) { eng.render(o.data(), 512); for (int16_t v : o) energy += (long long)v * v; }
        printf("MIX energy=%lld s1_playing=%d s2_playing=%d\n", energy, eng.isPlaying(s1), eng.isPlaying(s2));
        if (energy == 0 || !eng.isPlaying(s1)) ++fails;
    }
    printf("fails=%d\n", fails);
    return fails ? 1 : 0;
}
