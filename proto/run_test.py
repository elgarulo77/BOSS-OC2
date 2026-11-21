import subprocess, numpy as np
from scipy.io import wavfile
import octaver_ref as R
SR = 44100

def pluck(f, dur=1.0, amp=0.25):
    t = np.arange(int(SR * dur)) / SR
    s = sum(a * np.sin(2 * np.pi * f * k * t) for k, a in ((1, 1), (2, .5), (3, .3), (4, .15)))
    return s * np.minimum(1, t * 200) * np.exp(-t * 2.0) * amp

def peak(sig, lo, hi):
    sp = np.abs(np.fft.rfft(sig * np.hanning(len(sig)), 1 << 18)); fr = np.fft.rfftfreq(1 << 18, 1 / SR)
    m = (fr > lo) & (fr < hi); return fr[m][np.argmax(sp[m])]

def cpp(x, k):
    x.astype('<f8').tofile('/tmp/in.f64')
    subprocess.run(['./core_test', '/tmp/in.f64', '/tmp/out.f64', *map(str, k)], check=True)
    return np.fromfile('/tmp/out.f64', dtype='<f8')

notes = [("E2", 82.41), ("A2", 110.0), ("D3", 146.83), ("G3", 196.0), ("B3", 246.94), ("E4", 329.63)]
seq = np.concatenate([pluck(f) for _, f in notes])

# 1) equivalencia Python vs C++
ref = R.octaver(seq, knobs=(0.7, 0.8, 0.6)); c = cpp(seq, (0.7, 0.8, 0.6))
print(f"Python vs C++: error maximo = {np.abs(ref - c).max():.2e}  (pico de salida {np.abs(ref).max():.3f})")

# 2) tracking, un canal a la vez
o1 = cpp(seq, (0, 1, 0)); o2 = cpp(seq, (0, 0, 1))
print("nota   f0      oct1 detectada (esp)    oct2 detectada (esp)")
for k, (name, f) in enumerate(notes):
    seg = slice(k * SR + 3000, (k + 1) * SR - 5000)
    p1 = peak(o1[seg], f / 4, f); p2 = peak(o2[seg], f / 8, f / 2)
    ok1 = "ok" if abs(p1 - f / 2) < 1.5 else "FALLA"; ok2 = "ok" if abs(p2 - f / 4) < 1.5 else "FALLA"
    print(f"{name:4s} {f:7.2f}  {p1:7.2f} ({f/2:6.2f}) {ok1:5s}    {p2:7.2f} ({f/4:6.2f}) {ok2}")

# 3) audio de demo
full = cpp(seq, (0.7, 0.8, 0.6)); g = 0.9 / max(np.abs(full).max(), np.abs(seq).max())
wavfile.write("demo_dry.wav", SR, (seq / np.abs(seq).max() * 0.9 * 32767).astype(np.int16))
wavfile.write("demo_octaver.wav", SR, (full * g * 32767).astype(np.int16))
