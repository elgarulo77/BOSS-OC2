#pragma once
// Núcleo DSP del octavador, derivado del esquema "BOSS OCTAVE OC2 MODIFIED" (B. Busser, 1999).
// Sin dependencias de JUCE: se puede probar solo con g++. Trabaja en double, mono.
#include <cmath>
#include <initializer_list>

namespace oct
{
constexpr double kPi = 3.14159265358979323846;

struct OnePole
{
    double a = 0, y = 0;
    void set (double fs, double fc) { a = std::exp (-2.0 * kPi * fc / fs); }
    double process (double x) { y = (1.0 - a) * x + a * y; return y; }
    void reset() { y = 0; }
};

// Pasa-bajos de 2º orden (el Sallen-Key unitario del circuito), fc y Q calculados del esquema
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void setLowpass (double fs, double fc, double Q)
    {
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), al = std::sin (w) / (2.0 * Q), a0 = 1.0 + al;
        b0 = (1.0 - c) / 2.0 / a0; b1 = (1.0 - c) / a0; b2 = b0;
        a1 = -2.0 * c / a0;        a2 = (1.0 - al) / a0;
    }
    double process (double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
    void reset() { x1 = x2 = y1 = y2 = 0; }
};

struct OctaveCore
{
    // --- valores sacados del esquema ---
    static constexpr double kInGain   = 1.0 + 10e3 / 2.7e3; // IC01: 1 + R7/R6
    static constexpr double kClipV    = 3.5;                // excursión del TL072 con 9 V (±V sobre 4,9 V)
    static constexpr double kDiodeV   = -0.2;               // 1N34A: recorta la semionda negativa
    static constexpr double kGainOn   = -27.0 / 47.0;       // JFET conduciendo (R36/R30)
    static constexpr double kGainOff  = (1.0 + 27.0 / 47.0) * (100.0 / 147.0) - 27.0 / 47.0; // JFET cortado
    static constexpr double kEps      = 1e-5;               // 10 uV de "offset": evita que el latch vibre con ruido numérico
    static constexpr double kRload    = 730e3;              // BC548 + R51 sobre el bus de mezcla

    // Escalas de tiempo (1 = circuito original). Equivalen a cambiar capacitores:
    //   preScale: capacitores del pre-filtro x preScale (fc / preScale)
    //   attScale / relScale: C16 y R24 del detector de picos
    void prepare (double sampleRate, double preScale = 1.0, double attScale = 1.0, double relScale = 1.0)
    {
        fs = sampleRate;
        // Pre-filtro: R9/C7 (1 polo) + Sallen-Key R11/R12 330k, C10 2n2, C8 220p
        preLP1.set (fs, 1.0 / (2.0 * kPi * 22e3 * 10e-9 * preScale));
        preSK.setLowpass (fs, 1.0 / (2.0 * kPi * 330e3 * std::sqrt (2.2e-9 * 220e-12) * preScale),
                          0.5 * std::sqrt (2.2e-9 / 220e-12));
        // Octava 1: R38/C23 + SK (R40/R41 330k, C25 4n7, C26 220p)
        o1LP1.set (fs, 1.0 / (2.0 * kPi * 22e3 * 22e-9));
        o1SK.setLowpass (fs, 1.0 / (2.0 * kPi * 330e3 * std::sqrt (4.7e-9 * 220e-12)),
                         0.5 * std::sqrt (4.7e-9 / 220e-12));
        // Octava 2: R31/C14 + SK (R37/R39 330k, C22 10n, C24 1n)
        o2LP1.set (fs, 1.0 / (2.0 * kPi * 22e3 * 47e-9));
        o2SK.setLowpass (fs, 1.0 / (2.0 * kPi * 330e3 * std::sqrt (10e-9 * 1e-9)),
                         0.5 * std::sqrt (10e-9 / 1e-9));
        // Detector de picos: ataque R32*C16 = 1 ms, caída (R24+R32)*C16 = 11 ms
        aAtt = std::exp (-1.0 / (1e-3 * attScale * fs));
        aRel = std::exp (-1.0 / (11e-3 * relScale * fs));
        reset();
    }

    void reset()
    {
        for (auto* f : { &preLP1, &o1LP1, &o2LP1 }) f->reset();
        for (auto* f : { &preSK, &o1SK, &o2SK }) f->reset();
        cp = cn = 0; latch = false; ff1 = ff2 = false;
    }

    // Potes de 100k log: aproximación de la curva audio-taper
    static double taper (double k) { return k * k * k; }

    // x: señal de entrada (1.0 = 1 V). knobs 0..1. Devuelve señal mono.
    double process (double x, double knobDirect, double knobOct1, double knobOct2)
    {
        // 1) etapa de entrada IC01 (ganancia + límite del op-amp)
        const double u = kClipV * std::tanh (kInGain * x / kClipV);

        // 2) rama de detección: pre-filtro -> detectores de pico -> latch SET/RESET
        const double pf = preSK.process (preLP1.process (u));
        cp += ((pf > cp) ? (1.0 - aAtt) : (1.0 - aRel)) * (pf - cp);
        cn += ((pf < cn) ? (1.0 - aAtt) : (1.0 - aRel)) * (pf - cn);
        const bool set = pf > cp + kEps, rst = pf < cn - kEps;
        const bool prev = latch;
        if (set && !rst) latch = true; else if (rst && !set) latch = false;
        if (latch && !prev)            // flanco ascendente: MC14013 (÷2)
        {
            ff1 = !ff1;
            if (ff1) ff2 = !ff2;       // flanco ascendente del ÷2: MC14027 (÷4)
        }

        // 3) generación de octavas: semionda (diodo) x ganancia conmutada por el JFET
        const double vc = (u > kDiodeV) ? u : kDiodeV;
        const double s1 = vc * (ff1 ? kGainOn : kGainOff);
        const double s2 = vc * (ff2 ? kGainOn : kGainOff);
        const double o1 = o1SK.process (o1LP1.process (s1));
        const double o2 = o2SK.process (o2LP1.process (s2));

        // 4) mezcla: cada canal = pote 100k log + R 100k serie sobre un bus cargado por 730k
        const double w[3] = { taper (knobDirect), taper (knobOct1), taper (knobOct2) };
        const double v[3] = { u, o1, o2 };
        double num = 0, den = 1.0 / kRload;
        for (int i = 0; i < 3; ++i)
        {
            const double z = 100e3 + 100e3 * w[i] * (1.0 - w[i]);
            num += w[i] * v[i] / z;
            den += 1.0 / z;
        }
        return num / den;
    }

    double fs = 44100.0;
    OnePole preLP1, o1LP1, o2LP1;
    Biquad preSK, o1SK, o2SK;
    double aAtt = 0, aRel = 0, cp = 0, cn = 0;
    bool latch = false, ff1 = false, ff2 = false;
};
} // namespace oct
