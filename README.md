# Octave Divider (VST3) - octavador analógico tipo divisor

Modelo del circuito del esquema "BOSS OCTAVE OC2 MODIFIED" (B. Busser, 1999).
Controles: Direct Level, Octave 1 Level (-1 oct), Octave 2 Level (-2 oct). Monofónico.

## Qué modela (valores del esquema)
- Entrada: ganancia 1+R7/R6 = 4,7 y límite del TL072.
- Pre-filtro: 1 polo 723 Hz + Sallen-Key 693 Hz (Q 1,58).
- Detector: dos detectores de pico (ataque 1 ms, caída 11 ms) -> latch SET/RESET.
- MC14013 (÷2) y MC14027 (÷4).
- Octavas: semionda (diodo 1N34A) x ganancia conmutada por JFET (-0,57 / +0,50).
- Filtros de salida: Oct1 = 329 Hz + SK 474 Hz (Q 2,3); Oct2 = 154 Hz + SK 152 Hz (Q 1,58).
- Mezcla: potes 100k log + 100k serie sobre bus cargado (BC548).
No modela: ruido, tolerancias, saturación fina de los JFET ni el interruptor de bypass.

## Compilar en Windows
Requisitos: Visual Studio 2022 (C++ de escritorio), CMake, Git.
    cd juce
    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release
El VST3 se copia a C:\Program Files\Common Files\VST3 (terminal como administrador si falla).

## Obtener el VST3 sin instalar nada (GitHub Actions)
1. Subí esta carpeta completa a un repositorio de GitHub (la carpeta `.github` tiene que quedar en la raíz).
2. Pestaña "Actions" -> "build-vst3" -> "Run workflow".
3. Al terminar, descargá el artefacto "OctaveDivider-VST3-Windows" (contiene `Octave Divider.vst3`).
4. Copialo a `C:\Program Files\Common Files\VST3`.

## Pruebas (sin JUCE)
    cd proto
    g++ -O2 -std=c++17 -o core_test core_test.cpp
    python3 run_test.py
Compara el núcleo C++ con la referencia en Python y mide el tracking de cada octava.
