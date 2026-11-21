import subprocess, sys, numpy as np
SR = 44100
NOTES = [("B0", 30.87), ("C1", 32.70), ("E1", 41.20), ("A1", 55.0), ("D2", 73.42), ("G2", 98.0), ("C3", 130.8), ("G3", 196.0)]

def bass(f, dur=1.6, amp=0.3):
    t = np.arange(int(SR * dur)) / SR
    h = [1.0, 1.1, 0.55, 0.4, 0.25, 0.15, 0.1]   # bajo real: 2º armónico fuerte
    s = sum(a * np.sin(2 * np.pi * f * k * t + 0.3 * k) for k, a in enumerate(h, 1))
    return s / 2.0 * np.minimum(1, t * 100) * np.exp(-t * 1.2) * amp

def peak(sig, lo, hi):
    n = 1 << 19; sp = np.abs(np.fft.rfft(sig * np.hanning(len(sig)), n)); fr = np.fft.rfftfreq(n, 1 / SR)
    m = (fr > lo) & (fr < hi); return fr[m][np.argmax(sp[m])]

def run(x, k, scales):
    x.astype('<f8').tofile('/tmp/b_in.f64')
    subprocess.run(['./core_test', '/tmp/b_in.f64', '/tmp/b_out.f64', *map(str, k), *map(str, scales)], check=True)
    return np.fromfile('/tmp/b_out.f64', dtype='<f8')

def evaluate(scales, verbose=False):
    good = 0; rows = []
    for name, f in NOTES:
        x = bass(f); o1 = run(x, (0, 1, 0), scales)
        seg = slice(int(0.25 * SR), int(1.5 * SR))
        p = peak(o1[seg], f / 4, f * 0.8)
        ok = abs(p - f / 2) < max(0.6, 0.02 * f); good += ok
        rows.append(f"{name:3s} {f:6.2f} -> oct1 {p:7.2f} (esp {f/2:6.2f}) {'ok' if ok else 'FALLA'}")
    if verbose: print("\n".join(rows))
    return good

if __name__ == "__main__":
    print("== circuito original (1,1,1) =="); evaluate((1, 1, 1), True)
    print("\n== busqueda: (preScale, attScale, relScale) -> notas ok de 8 ==")
    for ps in (1, 2, 4, 8):
        for at in (1, 2):
            for rl in (1, 3, 6, 12):
                print(f"pre x{ps:<2} att x{at} rel x{rl:<2}: {evaluate((ps, at, rl))}/8")
