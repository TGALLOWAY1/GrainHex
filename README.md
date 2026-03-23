# GrainHex

A standalone desktop granular synthesizer built for bass music producers.

## Overview

GrainHex combines three core systems:

1. **High-density granular engine** — the primary sound source with 9 parameters and 128+ grain support
2. **Auto-tuned sub layer** — tracks the granular output's detected pitch (not MIDI) to generate a clean sub bass
3. **Internal resample loop** — one-click capture, linear history (up to 8 iterations), revert, undo, and WAV export

Built with **C++17** and **JUCE 7**.

## Current Status — v1.0.0 (All Phases Complete)

| Phase | Status | Description |
|-------|--------|-------------|
| 1. Foundation | Done | App shell, audio I/O, sample loading, waveform, playback, looping, root note detection |
| 2. Granular Engine | Done | 9-parameter grain engine, scheduler, visualization, 128+ grain optimization |
| 3. Sub Tuner | Done | Sub oscillator, auto/manual tuning, YIN pitch detector, crossover filters |
| 4. Effects & Modulation | Done | Distortion (soft/hard/wavefold), multi-mode filter, LFO, ADSR, MIDI input |
| 5. Resample & Export | Done | Output capture, reload-as-source, 8-entry history with thumbnails, WAV export (16/24/32-float) |
| 6. Polish & Release | Done | PRD layout, dark theme, factory content, sample browser, mono lock, resize-safe layout |

## Features

- **Granular Engine**: 9 controls (size, count, position, spray, pitch, quantize, window, direction, spread) with 128+ concurrent grains
- **Sub Tuner**: Auto-tuned sub oscillator follows detected granular pitch with YIN algorithm
- **Effects**: Distortion (soft clip/hard clip/wavefold) and multi-mode filter (LP/HP/BP) with envelope amount
- **Modulation**: LFO (sine/triangle/square/S&H) and ADSR envelope with assignment routing
- **Resample Loop**: One-click capture, 8-entry history with mini waveform thumbnails, undo/revert, WAV export
- **Factory Content**: 18 synthesized bass samples (reeses, growls, FM basses, noise textures, vocal formants)
- **Sample Browser**: Category filter, preview, double-click to load
- **MIDI Input**: Note-to-pitch mapping with per-note envelope trigger
- **Mono Lock**: Sum stereo to mono for consistent bass monitoring
- **Dark Theme**: Studio-ready UI with consistent typography and accent colours
- **First-Launch Sound**: Auto-loads a factory sample with granular enabled — sounds interesting immediately

## Building

### Prerequisites

| Platform | Required                          | Notes |
|----------|-----------------------------------|-------|
| **macOS**   | Xcode, CMake                   | CoreAudio/CoreMIDI are built in — no extra deps |
| **Linux**   | CMake, g++, X11/GL/ALSA dev libs | See below for package list |
| **Windows** | Visual Studio 2019+ or MSVC Build Tools, CMake | WASAPI/DirectX are built in |

**Linux dependency install (Debian/Ubuntu):**
```bash
sudo apt-get install -y \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libfreetype6-dev libfontconfig1-dev \
    libasound2-dev \
    mesa-common-dev libgl1-mesa-dev \
    cmake g++ pkg-config
```

### Build Steps

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

On Linux, if Xrandr/Xinerama/Xcursor dev headers are not installed, pass defines via CXXFLAGS:
```bash
CXXFLAGS="-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0 -DJUCE_ALSA=0 -DJUCE_JACK=0" cmake ..
```

JUCE 7.0.12 is fetched automatically via CMake `FetchContent` — no manual download needed.

### Tests

```bash
cmake -DGRAINHEX_BUILD_TESTS=ON ..
cmake --build .
ctest
```

## UI Layout (PRD Specification)

```
+----------------------------------------------------------+
|  GrainHex v1.0   [info]              [root note] [MIDI]  |
+----------------------------------------------------------+
|                                       |                   |
|  [Waveform / Drop Zone]              | [Sample Browser]  |
|                                       |                   |
+---------------------------------------+-------------------+
|  [Play] [Stop] [Loop] [Granular] [Load]                  |
+----------------------------------------------------------+
|  GRANULAR: Size Count Position Spray Pitch Spread         |
|            Quantize  Window  Direction                    |
+------------------------------+---------------------------+
|                              |                           |
|  SUB TUNER                   |  RESAMPLE HISTORY         |
|  [Enable] Level  Pitch      |  [Resample] [Undo] [Clear]|
|  Waveform Mode Snap         |  Length  BitDepth          |
|  Gran HP  Sub LP             |  [thumbnails ...]         |
|                              |                           |
+------------------------------+---------------------------+
|  Master [====] Mono  Export WAV  |  status message       |
+----------------------------------------------------------+
```

## Project Structure

```
Source/
├── App/           # Application entry point, main window
├── Audio/         # Audio engine, parameter smoothing
├── SourceInput/   # Sample loading, root note detection, factory samples
├── Granular/      # Grain voices, scheduling, window functions
├── Sub/           # Sub oscillator engine, pitch detection
├── FX/            # Distortion and filter processors
├── Modulation/    # LFO, ADSR envelope, modulation routing
├── MIDI/          # MIDI input, note-to-pitch mapping
├── Resample/      # Resample engine, history, WAV export
├── UI/            # Editor panels, waveform, sample browser, look-and-feel
└── Tests/         # Unit tests
```

## Tech Stack

- **Language:** C++17
- **Framework:** JUCE 7.0.12
- **Build system:** CMake (minimum 3.22)
- **Platforms:** macOS, Linux, Windows (standalone app)

## Design Principles

- Sound quality first — the granular engine is the product
- Protect the audio thread — no allocations, file I/O, or blocking locks in the audio callback
- Sub follows detected granular pitch, not MIDI
- Effects process granular only; sub bypasses by default to preserve clean low end
- Resample history is linear-only in V1 — max 8 iterations
- Bass-music-first defaults throughout
- Factory content sounds interesting on first launch
