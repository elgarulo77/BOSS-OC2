import numpy as np
from bass_test import SR, run, NOTES

rng = np.random.default_rng(1)

def bass_hard(f, dur=1.6, amp=0.25, weak_fund=True):
    t = np.arange(int(SR * dur)) / SR
    h = [0.45, 1.0, 0.7, 0.4, 0.25, 0.15, 0.1] if weak_fund else [1.0, 1.1, .55, .4, .25, .15, .1]
    s = sum(a * np.sin(2 * np.pi * f * k * t + 0.5 * k) for k, a in enumerate(h, 1)) / 2.0
    s = s * np.minimum(1, t * 100) * np.exp(-t * 1.5) * amp
    hum = 0.004 * np.sin(2 * np.pi * 50 * t)          # zumbido de red (Argentina: 50 Hz)
    noise = 0.002 * rng.standard_normal(len(t))        # ruido de captación/cable
    return s + hum + noise

def purity(sig, f):
    n = 1 << 19; sp = np.abs(np.fft.rfft(sig * np.hanning(len(sig)), n)) ** 2; fr = np.fft.rfftfreq(n, 1 / SR)
    tot = sp[(fr > 3) & (fr < 4 * f)].sum(); band = sp[(fr > f / 2 * 0.95) & (fr < f / 2 * 1.05)].sum()
    return band / tot

def evaluate(scales, verbose=False):
    res = []
    for name, f in NOTES:
        x = bass_hard(f); o1 = run(x, (0, 1, 0), scales)
        seg = slice(int(0.25 * SR), int(1.5 * SR)); res.append(purity(o1[seg], f))
    if verbose:
        for (name, f), p in zip(NOTES, res): print(f"  {name:3s} {f:6.2f} Hz  pureza en f/2 = {p*100:5.1f}%")
    return res

if __name__ == "__main__":
    print("== circuito original =="); evaluate((1, 1, 1), True)
    print("\n== busqueda (pureza media / minima, y nota peor) ==")
    best = []
    for ps in (1, 2, 4, 8):
        for at in (1, 2, 4):
            for rl in (1, 3, 6, 12):
                r = evaluate((ps, at, rl)); best.append((np.mean(r), min(r), ps, at, rl))
                print(f"pre x{ps:<2} att x{at} rel x{rl:<2}: media {np.mean(r)*100:5.1f}%  minima {min(r)*100:5.1f}%")
    best.sort(reverse=True); print("\nTop 5 por media:"); [print(f"  pre x{b[2]} att x{b[3]} rel x{b[4]}: media {b[0]*100:.1f}% min {b[1]*100:.1f}%") for b in best[:5]]
