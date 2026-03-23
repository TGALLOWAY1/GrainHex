# GrainHex

A standalone desktop granular synthesizer built for bass music producers.

## Overview

GrainHex combines three core systems:

1. **High-density granular engine** — the primary sound source
2. **Auto-tuned sub layer** — tracks the granular output's detected pitch (not MIDI) to generate a clean sub bass
3. **Internal resample loop** — for iterative sound design (linear history, up to 8 iterations in V1)

Built with **C++17** and **JUCE 7**.

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

JUCE 7.0.12 is fetched automatically via CMake `FetchContent` — no manual download needed.

### Tests

```bash
cmake -DGRAINHEX_BUILD_TESTS=ON ..
cmake --build .
ctest
```

## Project Structure

```
Source/
├── App/           # Application entry point, main window
├── Audio/         # Audio engine, parameter smoothing
├── SourceInput/   # Sample loading, root note detection
├── Granular/      # Grain voices, scheduling, window functions
├── Sub/           # Sub oscillator engine, pitch detection
├── UI/            # Editor panels, waveform display
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
- Bass-music-first defaults throughout
