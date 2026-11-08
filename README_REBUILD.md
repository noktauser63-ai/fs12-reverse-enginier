# FS12 yeniden inşa

Bu klasör, `libapp.so`'nun kaynaktan yeniden yazımının iskeletidir. Plan ve durum: `ROADMAP.md`.

- `src/audio/` — FMOD'suz ses motoru (p1d, IMA-ADPCM, Ogg Vorbis, mixer, OpenSL ES çıkışı)
- `tests/`, `tools/verify_audio.py` — ana makinede (ASan+UBSan) ve ffmpeg'e karşı doğrulama
- `.github/workflows/rebuild.yml` — host testleri + arm64-v8a / armeabi-v7a / x86_64 derlemesi

Yerelde test: `cmake -S . -B b -G Ninja -DFS12_BUILD_TESTS=ON && cmake --build b`
