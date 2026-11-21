"""Referencia en Python del núcleo (mismo algoritmo que juce/Source/OctaveCore.h)."""
import math
import numpy as np

PI = math.pi


class OnePole:
    def __init__(s, fs, fc): s.a = math.exp(-2 * PI * fc / fs); s.y = 0.0
    def p(s, x): s.y = (1 - s.a) * x + s.a * s.y; return s.y


class Biquad:
    def __init__(s, fs, fc, Q):
        w = 2 * PI * fc / fs; c = math.cos(w); al = math.sin(w) / (2 * Q); a0 = 1 + al
        s.b0 = (1 - c) / 2 / a0; s.b1 = (1 - c) / a0; s.b2 = s.b0
        s.a1 = -2 * c / a0; s.a2 = (1 - al) / a0
        s.x1 = s.x2 = s.y1 = s.y2 = 0.0
    def p(s, x):
        y = s.b0 * x + s.b1 * s.x1 + s.b2 * s.x2 - s.a1 * s.y1 - s.a2 * s.y2
        s.x2 = s.x1; s.x1 = x; s.y2 = s.y1; s.y1 = y
        return y


def lp1(r, c): return 1 / (2 * PI * r * c)
def sk(r, cfb, cg): return 1 / (2 * PI * r * math.sqrt(cfb * cg)), 0.5 * math.sqrt(cfb / cg)


IN_GAIN = 1 + 10e3 / 2.7e3
CLIP = 3.5
DIODE = -0.2
G_ON = -27 / 47
G_OFF = (1 + 27 / 47) * (100 / 147) - 27 / 47
EPS = 1e-5
RLOAD = 730e3


def octaver(x, fs=44100.0, knobs=(0.7, 0.8, 0.6)):
    pre1 = OnePole(fs, lp1(22e3, 10e-9)); f, q = sk(330e3, 2.2e-9, 220e-12); pre2 = Biquad(fs, f, q)
    a1_ = OnePole(fs, lp1(22e3, 22e-9));  f, q = sk(330e3, 4.7e-9, 220e-12); a2_ = Biquad(fs, f, q)
    b1_ = OnePole(fs, lp1(22e3, 47e-9));  f, q = sk(330e3, 10e-9, 1e-9);     b2_ = Biquad(fs, f, q)
    aAtt = math.exp(-1 / (1e-3 * fs)); aRel = math.exp(-1 / (11e-3 * fs))
    cp = cn = 0.0; latch = ff1 = ff2 = False
    w = [k ** 3 for k in knobs]
    out = np.empty(len(x))
    for n, xn in enumerate(x):
        u = CLIP * math.tanh(IN_GAIN * xn / CLIP)
        pf = pre2.p(pre1.p(u))
        cp += ((1 - aAtt) if pf > cp else (1 - aRel)) * (pf - cp)
        cn += ((1 - aAtt) if pf < cn else (1 - aRel)) * (pf - cn)
        st = pf > cp + EPS; rs = pf < cn - EPS
        prev = latch
        if st and not rs: latch = True
        elif rs and not st: latch = False
        if latch and not prev:
            ff1 = not ff1
            if ff1: ff2 = not ff2
        vc = u if u > DIODE else DIODE
        o1 = a2_.p(a1_.p(vc * (G_ON if ff1 else G_OFF)))
        o2 = b2_.p(b1_.p(vc * (G_ON if ff2 else G_OFF)))
        num = 0.0; den = 1 / RLOAD
        for wi, vi in zip(w, (u, o1, o2)):
            z = 100e3 + 100e3 * wi * (1 - wi)
            num += wi * vi / z; den += 1 / z
        out[n] = num / den
    return out
