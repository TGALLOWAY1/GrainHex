# URGENT TODO — GrainHex Build Setup

## Build System & Dependencies by Platform

### macOS (Recommended Dev Environment)

**Build system options:**
- **Xcode** — Install from the Mac App Store. JUCE's Projucer can generate `.xcodeproj` files directly, or use CMake (see below).
- **CMake** — Works natively on macOS. Install via `brew install cmake` or from https://cmake.org.

**Dependencies:**
macOS ships with CoreAudio, CoreMIDI, and all required graphics frameworks. Most of what JUCE needs is already part of the OS.

**What you need to install:**
1. **Xcode** (includes the Apple compiler toolchain and macOS SDK)
2. **CMake** (if using the CMake route instead of Projucer)
3. **JUCE source** — already fetched automatically via `FetchContent` in `CMakeLists.txt`

**Build steps:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Linux

**Build system:** CMake (required).

**Dependencies — must be installed manually:**
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

> **Note:** The current `CMakeLists.txt` disables ALSA, JACK, XRandR, Xinerama, and XCursor via compile definitions. If you need these features, remove the corresponding `JUCE_*=0` flags.

**Build steps:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows

**Build system options:**
- **Visual Studio 2019+** — CMake can generate `.sln` files with `-G "Visual Studio 17 2022"`.
- **CMake + Ninja** — Faster builds if you have the MSVC toolchain installed.

**Dependencies:**
Windows ships with the audio and graphics APIs that JUCE needs (WASAPI, DirectX, etc.). No manual dependency installation required beyond the compiler.

**What you need to install:**
1. **Visual Studio 2019+** (with "Desktop development with C++" workload) or the standalone **MSVC Build Tools**
2. **CMake** (bundled with Visual Studio, or install separately)

**Build steps (Developer Command Prompt):**
```bat
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

---

## Cross-Platform Notes

- JUCE handles most cross-platform abstractions. If you add any platform-specific code, use `#ifdef` guards or JUCE's platform macros (`JUCE_MAC`, `JUCE_LINUX`, `JUCE_WINDOWS`).
- JUCE 7.0.12 is fetched automatically by CMake via `FetchContent` — no need to vendor it manually.
- To build tests: `cmake -DGRAINHEX_BUILD_TESTS=ON ..`

---

## Outstanding Items

- [ ] Verify full build on macOS and Windows (Linux sandbox build confirmed)
- [ ] Decide whether to re-enable ALSA/JACK on Linux for real audio output
- [ ] Test MIDI input on all platforms
