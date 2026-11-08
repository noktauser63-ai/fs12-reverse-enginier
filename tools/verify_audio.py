#!/usr/bin/env python3
"""audio_test ciktisini ffmpeg ile karsilastirir. Kullanim: verify_audio.py <audio_dir> <out_dir>"""
import glob, os, subprocess, sys
import numpy as np

audio_dir, out_dir = sys.argv[1], sys.argv[2]
tmp = os.path.join(out_dir, "_x")
bad = 0
for p in sorted(glob.glob(os.path.join(audio_dir, "*.p1d"))):
    base = os.path.basename(p)[:-4]
    d = open(p, "rb").read()
    typ = int.from_bytes(d[:4], "little")
    open(tmp, "wb").write(d[8:])
    if typ == 2:
        ref = np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-f", "wav", "-i", tmp, "-f", "s16le", "-acodec", "pcm_s16le", "-"], capture_output=True).stdout, dtype="<i2")
        ours = np.fromfile(os.path.join(out_dir, base + ".raw"), dtype="<i2")
        ok = np.array_equal(ref, ours)
        print(f"{base:26s} WAV  {'OK' if ok else 'FARKLI'}")
    else:
        # Muzikler mono: ffmpeg'e -ac 1 verip bizim sol kanalla (motor mono'yu iki kanala kopyalar) karsilastir
        ref = np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-f", "ogg", "-i", tmp, "-t", "12", "-f", "s16le", "-ac", "1", "-ar", "44100", "-"], capture_output=True).stdout, dtype="<i2").astype(float)
        ours = np.fromfile(os.path.join(out_dir, base + ".mix.raw"), dtype="<i2").astype(float)[0::2]
        n = min(len(ref), len(ours)) - 8
        best = max(np.corrcoef(ref[:n], ours[s:n + s])[0, 1] for s in range(0, 4))
        ok = best > 0.9999
        print(f"{base:26s} OGG  corr={best:.6f} {'OK' if ok else 'FARKLI'}")
    bad += not ok
sys.exit(1 if bad else 0)
