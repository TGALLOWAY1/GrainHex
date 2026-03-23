# GrainHex V1 — Implementation Instructions

## Purpose

This document turns the current **development plan** and **PRD** into an implementation-focused build guide for GrainHex V1. It starts from the beginning of the development plan and translates each phase into concrete engineering work, architecture decisions, and completion criteria.

It is aligned to the project intent that GrainHex is a **standalone desktop granular synthesizer for bass music producers** with three defining systems:

1. a high-density granular engine,
2. an auto-tuned sub layer that tracks the granular output,
3. an internal resample loop for iterative sound design.

## Non-Negotiable Product Rules

These rules should guide every implementation decision.

- **Sound quality first.** The granular engine is the product. Do not rush past Phase 2.
- **Playable every week.** Every milestone should end with something you can open, hear, and test.
- **Bass-music-first defaults.** Every default sound and workflow should feel relevant to bass producers.
- **Keep V1 small and correct.** Do not add branching history, mod matrixes, MPE, or extra effects during MVP.
- **Protect the audio thread.** No allocations, file I/O, locks with unpredictable blocking, or expensive UI work on the audio thread.
- **Sub follows detected granular pitch, not MIDI.** This is a defining product decision.
- **Effects process granular only; sub bypasses by default.** This preserves clean low end.
- **Resample history is linear-only in V1.** Max 8 visible iterations.

---

# 1. Recommended Technical Foundation

## 1.1 Stack

Use the stack already implied by the plan and PRD:

- **Language:** C++17
- **Framework:** JUCE 7+
- **Build system:** CMake
- **Platforms:** macOS and Windows
- **App target:** Standalone desktop app only for V1

## 1.2 High-Level Runtime Architecture

Implement the app as a small set of clear subsystems.

### Core subsystems

- `AudioEngine`
  - owns audio device setup
  - owns callback entry point
  - pulls audio from current synthesis/render pipeline
- `SourceSampleManager`
  - loads WAV / AIFF / FLAC
  - stores source sample buffer
  - manages sample metadata and root note detection result
- `GranularEngine`
  - owns grain pool, scheduling, per-grain playback, interpolation, and summing
- `SubEngine`
  - owns sub oscillator, tuning mode, pitch-follow logic, LP filter, saturation, mono behavior
- `PitchDetector`
  - analyzes granular-only output before sub mix
- `EffectsChain`
  - distortion + filter for granular output only
- `ModulationEngine`
  - LFO, ADSR, target assignments
- `ResampleEngine`
  - captures full output, stores history, reloads source, exports WAV
- `AppState`
  - owns current parameter values and UI-visible state
- `WaveformViewModel`
  - read-only model for waveform rendering, playhead, loop region, grain activity markers

## 1.3 Threading Model

Keep thread responsibilities strict.

### Audio thread

Allowed:
- reading preloaded sample buffers
- grain scheduling and rendering
- sub generation
- effects processing
- pitch analysis on preallocated rolling buffers
- parameter smoothing

Disallowed:
- file I/O
- dynamic allocation in steady-state rendering
- UI mutation
- logging every callback

### UI thread

Allowed:
- waveform rendering
- control changes
- drag-and-drop handling
- history panel rendering
- tuner display updates from atomics / lock-free snapshots

### Worker threads

Use dedicated workers for:
- sample loading / decoding
- resample rendering / WAV encoding
- optional offline analysis steps

## 1.4 Suggested Folder Structure

```text
Source/
  App/
    Main.cpp
    Application.cpp
    MainWindow.cpp
  Audio/
    AudioEngine.h/.cpp
    AudioTypes.h
    ParameterSmoother.h/.cpp
  SourceInput/
    SourceSampleManager.h/.cpp
    AudioFileLoader.h/.cpp
    RootNoteDetector.h/.cpp
  Granular/
    GranularEngine.h/.cpp
    GrainVoice.h/.cpp
    GrainScheduler.h/.cpp
    Interpolator.h/.cpp
    WindowFunctions.h/.cpp
  Sub/
    SubEngine.h/.cpp
    PitchDetector.h/.cpp
    Oscillator.h/.cpp
  FX/
    DistortionProcessor.h/.cpp
    FilterProcessor.h/.cpp
  Modulation/
    Lfo.h/.cpp
    Envelope.h/.cpp
    ModulationRouter.h/.cpp
  Resample/
    ResampleEngine.h/.cpp
    WavExporter.h/.cpp
    HistoryManager.h/.cpp
  UI/
    MainEditor.h/.cpp
    WaveformView.h/.cpp
    GranularPanel.h/.cpp
    SubPanel.h/.cpp
    EffectsPanel.h/.cpp
    ResamplePanel.h/.cpp
    FooterPanel.h/.cpp
  Tests/
    ...
```

---

# 2. Phase 1 — Foundation

**Goal:** a running app that loads and plays audio, shows a waveform, supports looping, and detects root note.

This phase exists to establish the substrate the rest of the product depends on. According to the development plan, Milestone 1 is reached when the app loads, displays, and plays samples with loop region selection and root-note detection. fileciteturn1file0

## 2.1 Phase 1 deliverables

Build these first:

- app launches on macOS and Windows
- audio device opens and renders stable audio
- user can drag in WAV / AIFF / FLAC
- sample is decoded and stored in memory
- waveform is rendered
- sample can play and stop
- loop region can be selected and auditioned
- detected root note is displayed

## 2.2 Implementation order

### Step 1 — Create the JUCE standalone shell

Implement:
- JUCE app target with CMake
- `AudioDeviceManager` initialization
- a minimal main window with placeholder controls
- Git repo and milestone tags from day one

Done when:
- app compiles and launches on both target OSes
- audio device opens without errors

### Step 2 — Prove audio output with a sine test

Before sample playback, verify the callback path.

Implement:
- simple sine generator inside `AudioEngine`
- buffer size configuration with 256-sample default and 512 fallback
- sample-rate handshake with device

Done when:
- sine playback is clean and stable
- no crackles under normal use

### Step 3 — Implement file loading and source buffer ownership

Implement:
- `AudioFormatManager`
- drag-and-drop file ingestion
- decode to floating-point `AudioBuffer<float>`
- stereo/mono normalization policy
- metadata capture: sample length, channels, original sample rate, filename

Rules:
- decode off the audio thread
- swap current sample atomically or during a safe state transition
- do not let the audio callback read a partially loaded buffer

Done when:
- dropped files load reliably
- unsupported formats fail gracefully

### Step 4 — Build the waveform display

Implement:
- downsampled min/max peak data for drawing
- full-file waveform overview
- playhead overlay
- loop-start and loop-end overlays
- source region highlight

Done when:
- waveform renders quickly even for long files
- playhead updates smoothly

### Step 5 — Implement playback engine

Implement:
- play/stop transport state
- playback cursor
- optional resampling if file rate != device rate
- looping between selected markers

Design note:
- even though GrainHex is not a DAW, transport state still needs a stable timebase and should not be hacked into the UI layer

Done when:
- user can load a sample, play it, stop it, and hear clean looping

### Step 6 — Implement root note detection on load

Implement:
- offline or semi-offline pitch analysis after file load
- use YIN or autocorrelation, as recommended by plan/PRD
- confidence score + fallback to “unknown” for noisy content

Store:
- detected frequency
- nearest note
- cents offset
- confidence

Important:
- this Phase 1 detector is for source metadata and MIDI-relative pitch reference
- it is not the same as Phase 3 real-time sub-follow analysis

Done when:
- tonal bass samples produce plausible note labels
- noisy content does not produce misleading confidence

## 2.3 Phase 1 code decisions to lock down

Lock these decisions before Phase 2:

- sample buffers are immutable once published to the renderer
- engine parameters are owned in one central state object
- UI reads snapshots; UI does not directly own synthesis truth
- all audio-facing parameters are smoothed
- file load and render history are asynchronous

## 2.4 Phase 1 tests

### Manual tests
- launch app with no sample
- load mono WAV
- load stereo WAV
- load unsupported file
- set 2-second loop region and verify looping
- resize window and ensure waveform remains usable

### Automated tests
- sample decode smoke tests
- root note detection unit tests on known tonal files
- loop-boundary logic tests

## 2.5 Exit criteria

Do not leave Phase 1 until:
- playback is stable
- looping is click-free or acceptably smoothed
- root note detection is reliable enough for tonal sources
- source loading is robust

---

# 3. Phase 2 — Granular Engine

**Goal:** implement the core product: a competitive, high-density granular engine with 9 primary parameters and real-time grain visualization. The PRD and plan make clear this phase is the foundation and should not be rushed. fileciteturn1file0 fileciteturn1file1

## 3.1 Required V1 granular parameters

Implement these as the primary visible control surface:

- Grain Size
- Grain Count
- Position
- Spray
- Pitch
- Pitch Quantize
- Window Shape
- Direction
- Spread

These are explicitly defined in the PRD as the core surface of the granular engine. fileciteturn1file1

## 3.2 Internal engine model

Use a grain-voice pool architecture.

### Each `GrainVoice` should contain
- active flag
- source start position
- playback cursor
- grain length in samples
- playback increment
- pan gains
- window type
- reverse flag
- remaining samples

### Scheduler responsibilities
- determine when to spawn grains
- assign parameters at spawn time
- reuse inactive grain voices from a fixed pool
- avoid heap allocation during callback

### Render responsibilities
- render active grains into temp mix buffers
- sum to granular output bus
- apply parameter smoothing where needed

## 3.3 Implementation order

### Step 1 — Single grain playback

Implement one grain reading from the loaded source.

Requirements:
- Hann window first
- source position and length in samples
- interpolation on read

Done when:
- a manually triggered grain plays correctly from chosen position

### Step 2 — Continuous grain scheduler

Implement repeated spawning.

Requirements:
- fixed spawn timing first
- overlapping grains
- controlled density

Done when:
- the output becomes a continuous granular texture rather than isolated blips

### Step 3 — Grain size, count, and position

Implement the most important controls first.

Requirements:
- real-time updates
- waveform overlay showing current region / read position
- smooth parameter transitions

Done when:
- changing size, count, and position clearly changes the sound and visual state

### Step 4 — Spray and direction

Implement:
- randomized position offset from current base position
- forward, reverse, random direction per grain

Done when:
- 0% spray is stable and focused
- high spray meaningfully scatters grain starts

### Step 5 — Pitch and interpolation quality

Implement:
- semitone pitch shift with fine tune
- playback-increment based pitch shift
- cubic interpolation minimum; higher quality if needed

Do not settle for rough linear interpolation unless it is strictly temporary.

Done when:
- +/- 24 semitone range works and sounds acceptable on bass material

### Step 6 — Pitch quantize

Implement:
- chromatic / major / minor quantization
- quantize per grain pitch target

Done when:
- randomized pitch still locks to musical notes

### Step 7 — Window shape and spread

Implement:
- Hann
- triangle
- trapezoid
- stereo spread with deterministic or randomized panning strategy

Done when:
- window changes are audible and useful
- stereo spread remains mono-compatible at 0%

### Step 8 — High grain count optimization

Implement only after musical correctness is established.

Target:
- stable 128+ grains on target hardware

Likely optimizations:
- fixed-capacity grain pool
- precomputed window tables
- branch reduction in inner loops
- vector-friendly grain mixing
- cached parameter transforms
- separate dry temp buffers for easier profiling

Done when:
- 128 grains is stable without obvious dropouts

### Step 9 — Grain visualization

Implement:
- markers for active grain read positions
- visible density cue
- possible width / opacity based on grain size or activity

This is explicitly part of the UX concept in both the plan and PRD. fileciteturn1file0 fileciteturn1file1

## 3.4 Sound quality gates

Before moving on, validate with bass-relevant material.

Test sources:
- reese basses
- growls
- FM basses
- vocal chops
- noise textures

Reference comparisons:
- Quanta 2
- Grainferno
- Pigments

The development plan explicitly requires A/B comparison and says not to proceed if the reese-bass quality gate fails. fileciteturn1file0

## 3.5 Common failure modes to guard against

- thin or phasey output at high grain count
- ugly aliasing at extreme pitch shifts
- zipper noise from unsmoothed controls
- reverse grains clicking at boundaries
- spray causing out-of-bounds reads
- CPU spikes from bursty scheduling

## 3.6 Exit criteria

Do not enter Phase 3 until:
- all 9 granular controls work
- parameter changes are smooth
- grain visualization is informative
- 128 grains is at least mostly stable on target machine
- bass content sounds genuinely strong, not just technically functional

---

# 4. Phase 3 — Sub Tuner

**Goal:** add the second defining system: a dedicated sub layer that tracks the *granular output* in Auto mode and offers a fixed-note Manual mode. This behavior is central to GrainHex’s identity. fileciteturn1file1

## 4.1 Non-negotiable design behavior

- sub is **muted by default**
- sub tuning is based on **granular output**, not MIDI
- pitch analysis must be taken **before sub mix** to avoid feedback
- when confidence is low, **hold last note** instead of jumping randomly
- Manual mode must always exist as fallback for noisy material

## 4.2 Sub signal chain

Recommended order:

```text
Granular Output
  -> granular HP filter
  -> split:
       A) to pitch detector (pre-sub)
       B) to FX chain / main mix

Pitch detector result
  -> tuning logic
  -> sub oscillator frequency
  -> saturation
  -> sub LP filter
  -> mono lock behavior

Final mix = processed granular + clean sub
```

## 4.3 Implementation order

### Step 1 — Fixed-frequency sub oscillator

Implement:
- sine oscillator first
- triangle oscillator second
- independent level control
- muted-by-default state

Done when:
- sub can be enabled and heard under the granular layer

### Step 2 — Frequency isolation

Implement:
- granular HP filter
- sub LP filter
- overlap allowed but defaults should avoid low-end conflict
- optional mono-below threshold behavior

The PRD explicitly favors static crossover-style isolation for V1 instead of dynamic ducking. fileciteturn1file1

Done when:
- low end feels cleaner with sub engaged
- enabling sub does not make bass weak or phasey

### Step 3 — Real-time pitch detector

Implement:
- YIN or robust autocorrelation on rolling analysis window (~50 ms target per PRD)
- confidence output
- note + cents conversion
- nearest-semitone quantization

Important implementation note:
- use a decoupled analysis ring buffer fed from audio thread
- compute as lightly as possible, or move non-hard-real-time portions to a safe worker pipeline if latency remains acceptable

Done when:
- sustained tonal granular output yields stable note estimates

### Step 4 — Auto Tune mode

Implement:
- sub target note derives from detected pitch
- strict mode snaps to semitone
- loose mode allows more direct frequency following
- smoothing modes: slow / medium / fast

Done when:
- changing granular pitch characteristics causes sub to follow in a musically sensible way

### Step 5 — Manual Select mode

Implement:
- chromatic note selector C0–B3
- octave offset -2 / -1 / 0
- mode toggle that switches cleanly with no glitch

Done when:
- user can lock a note regardless of granular analysis

### Step 6 — Pitch display

Implement:
- note label
- octave
- cents bar
- visible in both Auto and Manual modes

The PRD explicitly states the pitch display should remain visible regardless of mode. fileciteturn1file1

Done when:
- tuner is readable at a glance and updates smoothly

## 4.4 Failure handling

Guard against:
- unstable note flapping on noisy textures
- feedback contamination if analysis accidentally includes the sub
- lag so high that auto-follow feels disconnected
- filter crossover defaults that hollow out the bass

## 4.5 Exit criteria

Do not move to Phase 4 until:
- Auto and Manual modes both work
- pitch display is trustworthy
- sub remains clean and useful
- noisy material degrades gracefully

---

# 5. Phase 4 — Effects, Modulation

**Goal:** add the minimum performance and shaping layer needed for a complete V1 sound-design workflow: distortion, filter, one LFO, one envelope, and MIDI input / learn. The PRD is explicit that V1 should stay minimal here. fileciteturn1file1

## 5.1 Effects scope

Implement only:
- Distortion / waveshaper
  - soft clip
  - hard clip
  - wavefold
  - drive
  - mix
- Multi-mode filter
  - LP / HP / BP
  - cutoff
  - resonance
  - envelope amount

Routing rule:
- effects apply to **granular only**
- sub bypasses effects by default

## 5.2 Modulation scope

Implement only:
- 1 LFO
  - sine, triangle, square, random/S&H
  - free or tempo-synced rate
  - depth
- 1 ADSR envelope
  - filter-targeted by default
- right-click assignment to parameters
- no modulation matrix in V1

# 5.3 Skipped (REMOVED MIDI REQUIREMENTS)

## 5.4 Implementation order

### Step 1 — Distortion and filter processors

Implement these first as standalone DSP units with tests.

Done when:
- each distortion mode is audibly distinct
- filter sweeps smoothly
- bypass toggles are click-free

### Step 2 — Granular-only routing

Be strict here. The sub should remain clean.

Done when:
- engaging distortion/filter changes only the granular layer

### Step 3 — LFO engine

Implement:
- phase accumulator
- shape selection
- normalized mod output
- depth scaling per target

Done when:
- LFO can modulate filter cutoff and at least one grain parameter smoothly

### Step 4 — Envelope

Implement:
- ADSR with clean state transitions
- default route to filter amount

Done when:
- envelope opening/closing the filter sounds musical

### Step 5 — Assignment UX

Implement:
- right-click context menu on parameters
- assign LFO / envelope
- per-target depth
- remove assignment
- visible target state indicator

Done when:
- modulation assignment feels obvious and inspectable


## 5.5 Exit criteria

Do not move to Phase 5 until:
- distortion and filter feel production-relevant
- modulation is smooth and inspectable

---

# 6. Phase 5 — Resample Engine and Export

**Goal:** implement the third defining system: fast iterative resampling with linear history, revert, undo, WAV export, and drag-and-drop out. The PRD explicitly frames this as a core workflow differentiator. fileciteturn1file1

## 6.1 Required V1 behavior

- one-click resample captures full output
- captured audio becomes the new source
- root note re-detects on reload
- history stores up to 8 iterations
- user can revert to any history entry
- single-step undo
- export current or history entry as WAV
- drag history item into DAW / file manager where platform support allows

## 6.2 Data model

Create a `ResampleHistoryEntry` containing:
- unique id
- audio buffer or path to temp rendered file
- waveform preview data
- timestamp
- source iteration index
- exported flag
- metadata snapshot of relevant parameters
- detected root note info

## 6.3 Implementation order

### Step 1 — Output capture

Implement:
- capture of final mixed output buffer
- configurable capture length or render-length strategy
- background handoff for storing captured audio

Decide early whether resample is:
- live capture of current playback output, or
- controlled render over a defined duration

For V1, favor the simplest version that feels immediate and predictable.

Done when:
- pressing resample creates a usable new audio source

### Step 2 — Reload as source

Implement:
- captured audio becomes current source
- waveform refreshes
- root note detection reruns
- playback/granular state remains coherent

Done when:
- user clearly sees and hears the new iteration as source material

### Step 3 — Linear history UI

Implement:
- list or stack of up to 8 mini waveform thumbnails
- current entry highlight
- click-to-revert
- hover tooltip for metadata

Done when:
- user can understand iteration history at a glance

### Step 4 — Undo and overflow policy

Implement:
- one-step undo minimum
- when entry 9 is added, oldest is removed
- warn if oldest was never exported

Done when:
- history remains manageable and predictable

### Step 5 — WAV export

Implement:
- export current state
- export any history item
- bit depth choice: 16 / 24 / 32-float
- sample rate matches project/device policy

Done when:
- exported files open correctly in DAWs

### Step 6 — Drag-and-drop out

Implement:
- thumbnail drag source via JUCE drag-and-drop support
- temp file handoff if necessary
- early compatibility testing with Ableton, FL, Logic, Reaper

The development plan explicitly warns this may be platform-fragile, so test it early and keep file export as the fallback path. fileciteturn1file0

Done when:
- at least one solid cross-app drag workflow works reliably

## 6.4 Resample-specific pitfalls

Guard against:
- blocking render thread during capture/export
- inconsistent source reload timing
- history confusion after many passes
- unstable memory growth over repeated iterations

## 6.5 Exit criteria

Do not move to Phase 6 until:
- resampling feels fast and fun
- revert/undo are reliable
- export works cleanly
- history is understandable

---

# 7. Phase 6 — Polish, Factory Content, Testing, Release

**Goal:** turn the working engine into a shippable product with polished layout, factory content, robust defaults, and release-level testing. The plan defines the final layout and Definition of Done clearly. fileciteturn1file0

## 7.1 Final layout to implement

Match the PRD layout concept:

- **Top:** source input, waveform, sample browser, drag-and-drop zone
- **Center:** granular controls
- **Bottom-left:** sub tuner
- **Bottom-right:** resample history
- **Right sidebar:** effects + modulation
- **Footer:** master level, mono lock, export controls

This layout is explicitly specified in the PRD and dev plan. fileciteturn1file1 fileciteturn1file0

## 7.2 Visual polish checklist

Implement:
- coherent dark theme for studio use
- consistent control spacing and typography
- parameter labels that are readable at a glance
- strong affordances for active assignments / toggles / current mode
- polished tuner and waveform overlays
- resize-safe layout with minimum supported window size

## 7.3 Factory content

Implement:
- 15–20 factory samples
- categories: reeses, growls, FM basses, noise textures, vocal stabs/chops
- internal browser with preview + load
- first-launch default state that already sounds interesting

This first-impression requirement is emphasized repeatedly in the PRD. fileciteturn1file1

## 7.4 Release hardening

### Test matrix
- macOS + Windows
- multiple device sample rates
- multiple buffer sizes
- no MIDI device
- disconnected audio device
- corrupt file
- tiny sample
- very long sample
- repeated resample cycles
- max grain count + sub + effects + modulation + pitch detection together

### Stability requirements
- no crashes in 1-hour sessions
- memory remains stable across repeated loads/resamples
- max-settings stress does not produce unacceptable dropouts on target hardware

### Packaging
- macOS DMG
- Windows installer
- code signing if possible
- quick-start README

## 7.5 Final ship criteria

Use the development plan’s V1 ship criteria as the release gate:

- granular engine complete and competitive
- sub tuner accurate and clean
- effects and modulation working
- resample and export working
- factory content present
- UI polished
- stable on macOS and Windows

These are explicitly listed in the Definition of Done / ship criteria. fileciteturn0file15

---

# 8. Cross-Phase Engineering Rules

## 8.1 Parameter system

Implement all parameters through a unified model.

Each parameter should support:
- current value
- normalized value
- default value
- smoothing strategy
- modulation contribution
- automation contribution

Avoid ad hoc parameter ownership spread throughout UI widgets.

## 8.2 Audio safety

Protect the renderer with these rules:
- fixed-capacity voice pools
- no per-sample dynamic allocations
- precompute as much as possible
- log only outside the callback
- keep UI polling lightweight

## 8.3 State snapshots for UI

Use read-only snapshots / atomics for:
- playhead
- active grain markers
- detected pitch
- output meters

Do not let the UI lock the DSP pipeline.

## 8.4 Testing cadence

At the end of every week, run:
- one functional smoke pass
- one stress pass
- one bass-music sound-quality pass
- one regression pass against the previous milestone

---

# 9. Suggested Milestone-by-Milestone Build Sequence

## Milestone 1
Foundation working:
- app shell
- audio I/O
- sample load
- waveform
- playback
- looping
- root note detection

## Milestone 2
Granular engine complete:
- all 9 controls
- stable scheduler
- grain visualization
- high grain count optimization
- bass-material validation

## Milestone 3
Sub tuner complete:
- sub oscillator
- auto/manual tuning
- pitch detector
- display
- crossover isolation

## Milestone 4
Shaping and control:
- distortion
- filter
- LFO
- ADSR

## Milestone 5
Workflow differentiator complete:
- resample capture
- reload
- history
- undo
- WAV export
- drag-and-drop

## Milestone 6
Ship pass:
- polished final layout
- factory content
- testing
- packaging

---

# 10. What Not to Build During V1

Do not let these slip into MVP unless they are required to fix a core flaw:

- plugin version (VST3/AU)
- branching resample history
- A/B compare mode
- modulation matrix
- extra FX like delay, chorus, reverb, compressor
- MPE
- sample recording input
- motion recording / macro automation
- spectral layer ideas
- preset-management system beyond absolute necessity

These belong in later roadmap stages, not MVP. fileciteturn1file1

---

# 11. Best Immediate Next Step

Start with a **Phase 1 implementation branch** and build in this exact order:

1. standalone JUCE shell
2. audio callback with sine test
3. drag-and-drop sample loading
4. waveform rendering
5. transport + loop region
6. root note detection on load

Only after Milestone 1 is solid should you begin the grain engine.

---

# 12. Source Basis

This implementation guide is based on the current GrainHex development plan and PRD, especially:

- six sequential phases and milestone structure from the development plan, including the emphasis on playable outputs and sound-quality-first execution fileciteturn1file0
- the PRD’s architecture, granular engine parameter surface, sub tuner rules, resample workflow, UI layout, and roadmap boundaries fileciteturn1file1
