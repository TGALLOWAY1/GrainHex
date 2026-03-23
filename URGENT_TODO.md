# URGENT TODO — GrainHex Setup & Verification Guide

## Quick Start — Build & Verify

### 1. Clone and Build

```bash
git clone <repo-url>
cd GrainHex
mkdir build && cd build
cmake ..
cmake --build .
```

### 2. Run the Application

```bash
# Linux
./GrainHex_artefacts/GrainHex

# macOS
open GrainHex_artefacts/GrainHex.app

# Windows
GrainHex_artefacts\Release\GrainHex.exe
```

### 3. Verify Core Functionality

**First Launch Check:**
- App should open with a factory sample ("Classic Reese") already loaded
- Granular mode should be enabled by default
- Waveform should be visible with the loaded sample
- You should hear sound when pressing Play (requires audio output device)

**Manual Verification Checklist:**

| Feature | How to Test | Expected Result |
|---------|------------|-----------------|
| **Factory Samples** | Open browser panel (right of waveform), select a sample, click Load | Sample loads, waveform updates |
| **File Loading** | Click "Load" button or drag-and-drop WAV/AIFF/FLAC | File loads, root note detected |
| **Granular Engine** | Toggle "Granular" on, adjust Size/Count/Position knobs | Sound changes with parameters |
| **Sub Tuner** | Enable Sub toggle, verify pitch display updates | Sub bass added, pitch detected |
| **Effects** | Enable Distortion and/or Filter (visible when Granular is on) | Sound is processed |
| **Modulation** | Enable LFO/Envelope (visible when Granular is on) | Filter modulates over time |
| **Resample** | Click "Resample" button with Granular enabled | Captures output, adds to history |
| **History** | Click history thumbnails to revert | Source changes to clicked entry |
| **Undo** | Click "Undo" after resampling | Reverts to previous state |
| **WAV Export** | Click "Export WAV" or right-click history thumbnail | File save dialog, WAV written |
| **Mono Lock** | Toggle "Mono" in footer | Stereo collapses to mono |
| **Master Volume** | Drag volume slider in footer | Output level changes smoothly |
| **Loop Region** | Click-drag on waveform to select region | Loop markers appear, playback loops region |
| **MIDI** | Connect MIDI controller, play notes | Pitch shifts, MIDI indicator lights up |

### 4. Run Unit Tests

```bash
cd build
cmake -DGRAINHEX_BUILD_TESTS=ON ..
cmake --build .
ctest
```

---

## Build System & Dependencies by Platform

### macOS (Recommended Dev Environment)

**What you need:**
1. **Xcode** (from Mac App Store — includes compiler toolchain)
2. **CMake** (`brew install cmake` or https://cmake.org)

**Build:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

macOS ships with CoreAudio, CoreMIDI, and all required frameworks. JUCE 7.0.12 is fetched automatically.

### Linux

**Dependencies — install manually:**
```bash
# Debian / Ubuntu
sudo apt-get install -y \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libfreetype6-dev libfontconfig1-dev \
    libasound2-dev \
    mesa-common-dev libgl1-mesa-dev \
    cmake g++ pkg-config

# Fedora / RHEL
sudo dnf install -y \
    libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel \
    freetype-devel fontconfig-devel \
    alsa-lib-devel \
    mesa-libGL-devel \
    cmake gcc-c++ pkg-config
```

**Build:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

> **Note:** If some X11/ALSA dev headers are not available, pass CXXFLAGS:
> ```bash
> CXXFLAGS="-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0 -DJUCE_ALSA=0 -DJUCE_JACK=0" cmake ..
> ```

### Windows

**What you need:**
1. **Visual Studio 2019+** (with "Desktop development with C++" workload) or standalone **MSVC Build Tools**
2. **CMake** (bundled with Visual Studio, or install separately)

**Build (Developer Command Prompt):**
```bat
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

---

## Cross-Platform Notes

- JUCE handles all platform abstractions. Use `JUCE_MAC`, `JUCE_LINUX`, `JUCE_WINDOWS` for platform-specific code.
- JUCE 7.0.12 is fetched automatically by CMake `FetchContent` — no manual download needed.
- Factory samples are generated at runtime (no external asset files to manage).

---

## Outstanding Items

- [ ] Verify full build and functionality on macOS and Windows (Linux build confirmed)
- [ ] Decide whether to re-enable ALSA/JACK on Linux for real audio output
- [ ] Test MIDI input on all three platforms
- [ ] Multi-sample-rate testing (44.1k, 48k, 96k)
- [ ] Multi-buffer-size testing (64, 128, 256, 512, 1024)
- [ ] Stress test: max grain count + sub + effects + modulation running simultaneously
- [ ] 1-hour session stability test (memory should remain stable)
- [ ] Cross-app drag-and-drop testing (temp file infrastructure ready)
- [ ] Create macOS DMG installer
- [ ] Create Windows installer
- [ ] Code signing (if certificates available)
