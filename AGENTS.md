# AGENTS.md

## Project Overview

**stepmaniax-gif-maker** is a cross-platform GUI tool for creating, editing, previewing, and uploading animated GIFs to StepManiaX dance pads. It targets non-developer SMX pad owners and is distributed as a free, open-source, single-binary application.

## Technology Decisions

### Framework: Dear ImGui + SDL3 (C++)

Chosen because:
- Direct linking to the C++ stepmaniax-sdk-mp library (no FFI/bindings needed)
- Single static binary distribution (~2-5 MB), no runtime dependencies
- Excellent for tool-style UIs with custom rendering (pixel grids, color palettes, timelines)
- Cross-platform via SDL3 (Windows, macOS, Linux)
- MIT licensed, active community
- Docking support built into ImGui for rearrangeable panel layout

### Build System: CMake

- FetchContent for ImGui and SDL3
- stepmaniax-sdk-mp linked as a sibling project or FetchContent from git

### Project File Format: .smxgifs

- Custom extension, but internally a ZIP archive containing:
  - `pad1_released.gif` (optional)
  - `pad1_pressed.gif` (optional)
  - `pad2_released.gif` (optional)
  - `pad2_pressed.gif` (optional)
  - `metadata.json` (mode, loop frames, project settings)
- This keeps files portable and inspectable (users can unzip to get raw GIFs)

## GIF Format Constraints (from stepmaniax-sdk-mp)

### Dimensions

| Mode | GIF Size | LEDs/Panel | Panel Region | Use |
|------|----------|-----------|--------------|-----|
| Legacy | 14×15 | 16 (4×4) | 4×4 px at (col*5, row*5) | Host playback only |
| Modern | 23×24 | 25 (4×4 outer + 3×3 inner) | 8×8 px at (col*8, row*8) | Host playback + firmware upload |

### Panel Layout in GIF

```
+----+----+----+
| 0  | 1  | 2  |
+----+----+----+
| 3  | 4  | 5  |
+----+----+----+
| 6  | 7  | 8  |
+----+----+----+
```

Panels are separated by 1-pixel gutters. Bottom row is a flag row (not visible on hardware).

### 23×24 LED Sampling

- Outer 4×4 grid: sampled at even coordinates within each panel's 8×8 region
- Inner 3×3 grid: sampled at odd coordinates within each panel's 8×8 region

### Firmware Upload Constraints

- **23×24 only** (modern mode required)
- **Max 32 frames** per animation type (released or pressed)
- **Max 15 unique colors per panel** (black = transparent)
- All 9 panels share the same frame timing (count + durations)
- Each panel has its own independent 15-color palette
- A 0.6666 color scaling factor is applied before sending to hardware

### Loop Frame Marker

Bottom-left pixel (x=0, y=height-1) set to white (R≥128, A=255) marks the loop point. Only the first marker is used. Default loops to frame 0.

### Playback

- 30 FPS target
- Frame durations from GIF delay field (10ms units); 30ms/40ms snapped to exactly 1/30s

## Architecture Decisions

### Pad Independence

Each pad is fully independent. The SDK accepts separate GIFs per (pad, animation_type):
- Pad 0 Released, Pad 0 Pressed
- Pad 1 Released, Pad 1 Pressed

Pads loop independently and do not sync with each other.

### Hardware Preview

- Drive `SMX_SetLights2` directly (not `SMX_LightsAnimation_SetAuto`) for full editor control over playback (scrubbing, pausing, per-pad control)
- `SMX_SetLights2` takes a single 1350-byte buffer for both pads simultaneously
- Preview thread composites both pad animations into one buffer at 30 FPS

### UI Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│ Menu: File | Edit | Animation | Hardware                            │
├──────────┬──────────────────────────────────┬───────────────────────┤
│ Tool     │ Main Canvas (zoomed pixel grid)  │ Preview Panel         │
│ Panel    │                                  │ - Pad 1 (actual size) │
│ - Draw   │ 3×3 panel grid with gutters      │ - Pad 2 (optional)   │
│ - Erase  │ visible, zoomable                │ - Zoom selector       │
│ - Fill   │                                  │                       │
│ - Pick   │                                  │ LED positions shown   │
│          │                                  │ as physical layout    │
├──────────┤                                  ├───────────────────────┤
│ Color    │                                  │ Per-panel color count │
│ Palette  │                                  │ "Panel 4: 8/15"      │
│ [15 max] │                                  │                       │
├──────────┴──────────────────────────────────┴───────────────────────┤
│ Timeline / Frame Strip                                              │
│ [1][2][3]...[32]  ▶ Play  ⏸ Pause  🔁 Loop: frame 0               │
│ Duration: 33ms | Frames: 12/32                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Editing Model

- Tab-based: "Pad 1 | Pad 2", with toggle for "Released | Pressed"
- Canvas always shows full 3×3 grid (all 9 panels)
- Each panel region is an independent drawing area with its own color budget
- Timeline (frame count, durations) is shared across all 9 panels

### Planned Features

- Mode selection: Legacy (14×15) vs Modern (23×24)
- Zoomed pixel grid editor with pan/zoom
- Side-by-side pad preview (both pads animating independently)
- Live hardware preview via SMX_SetLights2
- Per-panel color palette tracking with warnings at 15-color limit
- Color quantization tool
- GIF import (with validation) and export
- Firmware upload via SMX_LightsUpload_PrepareUpload + BeginUpload
- Onion skinning (ghost of previous/next frame)
- Copy/paste frames
- Undo/redo
- Custom theming (fonts, colors, rounded corners)

### Constraints the Editor Should Enforce

- Cannot exceed 15 colors per panel (warn, block upload)
- Cannot exceed 32 frames (for firmware upload; host playback has no limit)
- GIF dimensions must be exactly 14×15 or 23×24
- Upload only available in Modern (23×24) mode

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui) (MIT) — immediate-mode GUI
- [SDL3](https://github.com/libsdl-org/SDL) (zlib) — cross-platform windowing/input/rendering
- [stepmaniax-sdk-mp](https://github.com/fchorney/stepmaniax-sdk-mp) — SMX pad communication
- GIF encoder (TBD — likely a lightweight single-header library)

## Coding Style

- C++17 minimum
- Follow the style conventions of stepmaniax-sdk-mp where applicable
- Prefer single-header/vendored libraries for small dependencies
- Keep the build self-contained (FetchContent, no system-installed deps beyond compiler)
