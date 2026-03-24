# GrainHex V2 UI Redesign Plan

## Context

GrainHex V1 has all the right functional regions but the interface feels dense, uneven, and visually under-prioritized. Everything competes for attention equally — "six adjacent control boxes" instead of one focused instrument. The goal is to transform the UI into a scannable, immediate, bass-first experience where the waveform is the visual centerpiece, controls have clear hierarchy, and the app feels like a premium sound-design tool.

**Window size:** 1100x780 (fixed)
**Framework:** JUCE (C++), all styling via code — no CSS

---

## Files to Modify

| File | Role |
|------|------|
| `Source/UI/GrainHexLookAndFeel.h` | Theme constants + custom draw methods |
| `Source/UI/MainEditor.h` | Component declarations, new members |
| `Source/UI/MainEditor.cpp` | Master layout, paint(), transport, footer |
| `Source/UI/WaveformView.h` | Waveform interface additions |
| `Source/UI/WaveformView.cpp` | Waveform rendering, grain overlays, hover |
| `Source/UI/SampleBrowser.h` | Browser layout, selected states |
| `Source/UI/GranularPanel.h` | Granular interface (grouped layout) |
| `Source/UI/GranularPanel.cpp` | Granular layout + paint |
| `Source/UI/SubPanel.h` | Sub panel interface |
| `Source/UI/SubPanel.cpp` | Sub panel layout + paint |
| `Source/UI/EffectsPanel.h` | Effects interface (collapsible) |
| `Source/UI/EffectsPanel.cpp` | Effects layout + paint |
| `Source/UI/ModulationPanel.h` | Modulation interface (collapsible) |
| `Source/UI/ModulationPanel.cpp` | Modulation layout + paint |
| `Source/UI/ResamplePanel.h` | Resample interface |
| `Source/UI/ResamplePanel.cpp` | Resample layout + paint |

---

## Implementation Phases

### Phase 1: Theme Foundation & Typography
**Files:** `GrainHexLookAndFeel.h`

Update the `Theme` struct to establish the new design system before touching any panels.

**Color changes:**
```cpp
// Refined backgrounds — more elevation separation
bgDarkest    = 0xff08080f   // Deeper black
bgDark       = 0xff0c0c14   // Main background
bgPanel      = 0xff141428   // Panel backgrounds
bgPanelHover = 0xff1a1a34   // NEW: Panel hover/active state
bgControl    = 0xff1c1c32   // Control backgrounds (slightly lighter)
bgHover      = 0xff252548   // Hover state
bgElevated   = 0xff1e1e3a   // NEW: Elevated surfaces (active modules)

// Refined borders — less reliance on borders, more on elevation
border       = 0xff2a2a44   // Softer border (less prominent)
borderActive = 0xff4a4a77   // Active border
borderSubtle = 0xff1e1e36   // NEW: Very subtle separator

// Accent colors — reduced saturation, more intentional
accentGreen  = 0xff14b876   // Slightly desaturated green
accentPurple = 0xffb366e6   // Softer purple
accentRed    = 0xffee6666   // Softer red (warning/destructive only)
accentCyan   = 0xff00bbee   // Softer cyan
accentOrange = 0xffee8833   // Browser/load emphasis

// NEW: Semantic button colors
buttonPrimary      = 0xff14b876   // Green — main actions (Play, Resample, Load)
buttonSecondary    = 0xff2a2a4a   // Neutral — standard actions
buttonDestructive  = 0xffee6666   // Red — Clear, destructive
buttonUtility      = 0xff1c1c32   // Muted — Undo, secondary utilities
```

**Typography changes:**
```cpp
fontTitle       = 22.0f   // Slightly smaller, cleaner
fontSectionHead = 12.0f   // Reduced from 13
fontLabel       = 10.5f   // Reduced from 11
fontSmall       = 9.5f    // Reduced from 10
fontValue       = 11.0f   // Reduced from 12
fontMetadata    = 9.0f    // NEW: footer/metadata text
```

**Geometry changes:**
```cpp
cornerRadius    = 8.0f    // Increased from 6 — more modern
borderWidth     = 0.5f    // Thinner from 1 — less boxy
panelPadding    = 10      // Increased from 8 — more breathing room
controlGap      = 6       // Increased from 4
sectionGap      = 8       // NEW: gap between major sections
knobSize        = 48      // NEW: standardized knob diameter
knobSizeLarge   = 56      // NEW: primary knobs
```

**LookAndFeel updates:**
- `drawRotarySlider()`: Increase arc stroke to 3.5px, add subtle glow on filled arc, larger thumb dot (5px)
- `drawButtonBackground()`: Use `cornerRadius` instead of hardcoded 4.0f, add 1px accent-colored bottom border for primary buttons
- `drawLinearSlider()`: Thicker track (5px), larger thumb (12px)
- `drawToggleButton()`: Override to draw custom power-button style toggles (filled circle when on, outline when off) instead of default checkbox
- Add `drawComboBox()` override: Consistent styling with buttons

---

### Phase 2: Master Layout Restructure
**Files:** `MainEditor.h`, `MainEditor.cpp`

Rework the `resized()` layout to establish proper visual hierarchy.

**New layout proportions:**
```
Title Bar:     32px (reduced from 36)
Top Section:   210px (increased from 180 — more waveform dominance)
  Waveform:    full width minus 220px browser
  Browser:     220px (increased from 200)
Transport:     36px (increased from 30 — better touch targets)
Granular:      160px (reduced from 170, tighter with grouped knobs)
Bottom:        remaining (~220px split)
  Sub:         55% width (increased — it's a differentiator)
  Resample:    45% width
Footer:        36px (increased from 32, combines sample info + controls)
Sidebar:       260px (reduced from 280)
```

**Transport bar redesign:**
- Add a filled background strip (`bgPanel`) to visually group transport
- Play button: primary style (green bg when playing)
- Stop button: secondary style
- Loop/Granular toggles: custom toggle style with accent color fill when active
- Load button: right-aligned with separator, orange accent
- Add 1px top/bottom border lines for separation

**Footer consolidation** (merge sample info row + footer into one 36px bar):
- Left zone: Sample info text (filename | duration | rate) in `textDim`
- Center zone: Master label + volume slider + Mono toggle
- Right zone: Export WAV button (primary style)
- Status text overlays on left zone temporarily when messages arrive

**Paint changes:**
- Title: Use `textBright` instead of `accentGreen` for "GrainHex", add subtle "v1.0" in `textMuted`
- Remove version from title area, move to footer metadata
- Add MIDI indicator as a small colored dot rather than text label

---

### Phase 3: Waveform Panel Premium Treatment
**Files:** `WaveformView.h`, `WaveformView.cpp`

**Rendering improvements:**
- Background: `bgDarkest` (deepest black for maximum contrast)
- Waveform fill: Gradient from `accentGreen` at peaks to `accentGreen` at 30% opacity at center
- Add 1px center line in `borderSubtle`
- Increase visual density: render both channels overlaid if stereo
- Anti-alias: ensure paths use `juce::PathStrokeType` with rounded end caps

**Grain overlay improvements:**
- Draw grain read positions as vertical lines (not just dots) with 20% opacity, extending from waveform center
- Add grain density heat overlay: subtle horizontal gradient showing where grains cluster
- Active grains: brighter green with slight glow effect

**Loop region improvements:**
- Loop zone fill: `accentCyan` at 8% opacity (reduced from 12%)
- Loop start/end: Draw as 12px tall handle bars at top with `accentCyan`, 2px lines extending down
- Handles: rounded rect shape that looks draggable (filled when hovered)
- Add subtle shadow inside loop region edges

**Playhead:**
- 1.5px line in `textBright` with slight glow
- Add small triangle marker at top

**Hover feedback (new):**
- Add `mouseMove()` override to track hover position
- On hover: show thin vertical line + time position tooltip
- On drag: show position in samples/time near cursor

---

### Phase 4: Browser & Transport Polish
**Files:** `SampleBrowser.h`, `MainEditor.cpp`

**Browser improvements:**
- Increase row height from 28px to 32px
- Selected row: `bgElevated` background + left accent bar (3px `accentOrange`)
- Loaded sample: persistent accent bar + bold text
- Category dropdown: full width, cleaner styling
- Preview button: subtle outline style; Load button: primary style (orange)
- Add hover state for rows (`bgHover`)
- Better text layout: sample name left-aligned `textNormal`, category right-aligned `textDim`

---

### Phase 5: Granular Panel — Grouped Layout
**Files:** `GranularPanel.h`, `GranularPanel.cpp`

**Regroup controls into 3 logical clusters:**
```
┌─GRANULAR──────────────────────────────────────────────────────┐
│                                                                │
│  ┌─ Core ─────────┐  ┌─ Scatter ─────┐  ┌─ Tuning ──────┐   │
│  │ Size   Count   │  │ Position      │  │ Pitch         │   │
│  │ [knob] [knob]  │  │ [KNOB]        │  │ [KNOB]        │   │
│  │                │  │ Spray         │  │ Spread        │   │
│  │ Window         │  │ [knob]        │  │ [knob]        │   │
│  │ [dropdown]     │  │ Direction     │  │ Quantize      │   │
│  │                │  │ [dropdown]    │  │ [dropdown]    │   │
│  └────────────────┘  └───────────────┘  └───────────────┘   │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

- Position and Pitch get `knobSizeLarge` (56px) — they're the core sound-shaping controls
- Size, Count, Spray, Spread get `knobSize` (48px)
- Each group has a subtle background (`bgControl`) with rounded corners
- Group labels in `textDim` at top of each group
- Dropdowns below their associated knobs within each group
- Section header "GRANULAR" in `accentGreen`, `fontSectionHead`, left-aligned

---

### Phase 6: Sub Tuner — Pitch Centerpiece
**Files:** `SubPanel.h`, `SubPanel.cpp`

**Layout redesign:**
```
┌─ SUB TUNER ──────────────────────────────────────────┐
│ [On/Off]                                              │
│                                                        │
│  ┌─ Pitch Display ──────┐  ┌─ Controls ────────────┐ │
│  │      ┌──────┐        │  │ Level  [knob]         │ │
│  │      │  G2  │        │  │ Wave   [dropdown]     │ │
│  │      │ +2ct │        │  │ Mode   [dropdown]     │ │
│  │      └──────┘        │  │ Snap   [dropdown]     │ │
│  │  [confidence meter]  │  │ Smooth [dropdown]     │ │
│  │  100%                │  │                       │ │
│  └──────────────────────┘  │ Crossover             │ │
│                            │ Grain HP  [knob]      │ │
│                            │ Sub LP    [knob]      │ │
│                            └───────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

- Pitch note display: Large (fontTitle size), centered, `accentPurple`
- Cents indicator: +-50 cent bar below note name
- Confidence: thin horizontal bar below cents
- Rename "Gran HP" to "Grain Highpass" (or "Grain HP" with "Hz" unit shown)
- Rename "Sub LP" to "Sub Lowpass" (or "Sub LP" with "Hz" unit shown)
- Enable toggle: Custom power button style, `accentPurple` when on
- When disabled: entire panel content dims to 40% opacity
- Group "pitch + mode" together, "level + wave" together, "crossover" together

---

### Phase 7: Effects & Modulation Sidebar
**Files:** `EffectsPanel.h`, `EffectsPanel.cpp`, `ModulationPanel.h`, `ModulationPanel.cpp`

**Effects panel — collapsible sections:**
- Each sub-section (Distortion, Filter) has a header bar with:
  - Toggle on/off (power button style)
  - Section name
  - Click header to expand/collapse
- When module is OFF: collapse to just the header bar (~24px), controls hidden
- When module is ON: expand to show controls with `bgElevated` background
- This saves vertical space and reduces visual noise

**Distortion section (when expanded):**
- Mode dropdown + Drive knob + Mix knob in a row
- Accent: `accentOrange`

**Filter section (when expanded):**
- Mode dropdown
- Cutoff (large knob) + Resonance (standard) + Env Amount (standard)
- Accent: `accentOrange`

**Modulation panel — same collapsible pattern:**
- LFO section: Shape dropdown + Rate knob + Depth knob
- Envelope section: A, D, S, R knobs in a row
  - Add mini ADSR visualization curve above knobs (simple path drawing)
  - Shows current envelope shape in real-time as knobs change
- Accent: `accentRed` for both

**Inactive module dimming:**
- When toggle is off, section controls render at 40% opacity
- Background reverts to `bgPanel` from `bgElevated`

---

### Phase 8: Resample History — Show the Workflow
**Files:** `ResamplePanel.h`, `ResamplePanel.cpp`

**Layout redesign:**
```
┌─ RESAMPLE ─────────────────────────────────────────┐
│                                                      │
│  [Resample]  Length [===slider===] 4.0s  24-bit [v] │
│                                                      │
│  ┌─ History ──────────────────────────────────────┐ │
│  │ [thumb1] [thumb2] [thumb3] [thumb4]  ...       │ │
│  │  #1 2.0s  #2 2.0s  #3 4.0s  #4 4.0s           │ │
│  └────────────────────────────────────────────────┘ │
│                                                      │
│  [Undo]  [Clear]                                    │
└──────────────────────────────────────────────────────┘
```

- Resample button: primary style (`buttonPrimary`), full accent green, prominent
- History area: horizontal scrolling row of thumbnail cards
- Each thumbnail card:
  - Mini waveform in `accentGreen` (current) or `textDim` (history)
  - Iteration label "#1", "#2" etc below
  - Duration label below iteration
  - Current entry: `accentGreen` border (2px), `bgElevated` background
  - Hover: brighten background, show "click to revert" tooltip
- Empty state: show placeholder text "Resample to start building history"
- Undo: `buttonUtility` style
- Clear: `buttonDestructive` style (red tinted)
- Action row at bottom with Undo left, Clear right

---

### Phase 9: Hover States, Affordances & Polish
**Files:** All panel files, `GrainHexLookAndFeel.h`

**Global hover/focus states:**
- Knobs: on hover, brighten the value arc by 15%
- Buttons: already have hover via LookAndFeel, ensure consistent
- ComboBoxes: brighten border on hover
- Browser rows: `bgHover` background on hover
- History thumbnails: border brightens on hover
- Waveform loop handles: fill on hover (currently just outline)

**Draggable affordances:**
- Loop region handles: cursor changes to `LeftRightResizeCursor`
- History thumbnails: cursor changes to `DraggingHandCursor` when draggable
- Volume slider: standard horizontal resize cursor

**Selected state cues:**
- Active sample in browser: left accent bar
- Active history entry: green border + elevated background
- Active effect/mod toggle: bright accent fill on power button
- Active tuning mode: highlighted dropdown

**Brand polish:**
- Title "GrainHex" in a semi-bold weight, tracked slightly wider (letter spacing via manual character drawing or spaced string)
- MIDI indicator: small filled circle (6px) that pulses `accentGreen` on activity, `textMuted` when idle

---

## Implementation Order & Dependencies

The phases above should be executed in order because:
1. **Phase 1 (Theme)** must come first — all subsequent phases reference new theme values
2. **Phase 2 (Layout)** establishes the spatial foundation everything else builds on
3. **Phases 3-8** can largely proceed independently per-panel, but should follow the layout
4. **Phase 9 (Polish)** is a final pass across all files

Within each phase, edit the `.h` file first (new members/methods), then the `.cpp` (implementation).

---

## Verification

After each phase:
1. **Build:** `cmake --build build` — verify no compilation errors
2. **Launch:** Run the app and visually inspect the changed panels
3. **Functional check:** Ensure all controls still work (play/stop, load sample, adjust knobs, resample, export)

After all phases:
1. Load a factory sample → verify waveform displays correctly with new rendering
2. Enable granular → verify grouped controls layout, all knobs functional
3. Enable sub → verify pitch display, mode switching, crossover controls
4. Toggle effects/modulation → verify collapsible sections expand/collapse
5. Resample → verify history thumbnails appear and are clickable
6. Export WAV → verify file export still works
7. Drag-and-drop a file → verify loading works
8. Check all accent colors match semantic roles (green=granular, purple=sub, orange=effects, red=modulation)
