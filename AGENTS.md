# AGENTS.md

## Project Overview

**stepmaniax-gif-maker** is a cross-platform GUI tool for creating, editing, previewing, and uploading animated GIFs to StepManiaX dance pads. It targets non-developer SMX pad owners and is distributed as a free, open-source, single-binary application.

## Technology Stack

- **Framework:** Dear ImGui (docking branch) + SDL3 (C++17)
- **Build:** CMake 3.20+ with FetchContent for ImGui, SDL3, nlohmann/json
- **SDK:** stepmaniax-sdk-mp (git submodule at `src/vendor/stepmaniax-sdk-mp`)
- **GIF Decode:** gif_load.h (from SDK vendor, public domain)
- **GIF Encode:** Custom uncompressed LZW encoder (no external dependency)
- **License:** MIT

## Project Structure

```
├── CMakeLists.txt
├── AGENTS.md
├── TODO.md
├── README.md
├── LICENSE
├── src/
│   ├── main.cpp              # Application entry, UI rendering, event loop
│   ├── canvas.h/cpp          # Canvas data model (pixel buffer, panels, LED positions)
│   ├── gif_export.h/cpp      # Custom GIF encoder (exact palettes, uncompressed LZW)
│   ├── gif_import.h/cpp      # GIF decoder using gif_load.h
│   ├── preferences.h/cpp     # User preferences (JSON, platform-aware paths)
│   ├── undo.h/cpp            # Snapshot-based undo/redo system
│   ├── default_layout.h      # Default ImGui window layout (embedded ini)
│   └── vendor/
│       └── stepmaniax-sdk-mp/ # SDK submodule (pinned to v1.0.1)
└── build/
```

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

Panels separated by 1-pixel gutters. Bottom row (y=height-1) is a flag row.

### LED Sampling (23×24 Modern Mode)

- Outer 4×4 grid: even coordinates within each panel's 8×8 region (dx*2, dy*2)
- Inner 3×3 grid: odd coordinates (dx*2+1, dy*2+1), only positions where dx≤2 and dy≤2

### Firmware Upload Constraints

- **23×24 only** (modern mode required)
- **Max 32 frames** per animation type
- **Max 15 unique colors per panel across ALL frames** (black = transparent)
- All 9 panels share the same frame timing
- Each panel has its own independent 15-color palette
- 0.6666 color scaling applied by the SDK before sending to hardware (editor sends full 0-255 values)
- Pressed animation overlays on released: black pixels are transparent (show-through)

### Loop Frame Marker

Bottom-left pixel (x=0, y=height-1) with R≥128 marks the loop point. Only the first marker found is used. Default loops to frame 0.

### Playback

- 30 FPS hardware refresh rate
- Frame durations from GIF delay field (centiseconds)

## Architecture

### Editing Model

- Editor works on **one GIF at a time** (single animation)
- Mode (Legacy/Modern) chosen at creation time, not hot-swappable
- Canvas shows full 3×3 panel grid; only LED positions are editable
- Timeline shared across all 9 panels (same frame count/durations)
- 15-color limit is per-panel across ALL frames (firmware palette constraint)

### Hardware Integration

- **Live Preview:** Drives `SMX_SetLights2` directly at 30 FPS
  - Sync mode: mirrors editor's current frame (1:1 with canvas)
  - Play mode: animates independently with proper frame timing
- **Composite Preview:** Loads released + pressed GIFs, composites based on `SMX_GetInputState`
  - Black pixels in pressed GIF are transparent (released shows through)
  - "Fill black" option replaces black with (1,1,1) for full replacement
- **Firmware Upload:** `SMX_LightsUpload_PrepareUpload` + `BeginUpload`
  - Supports released and pressed animation types
  - Per-pad selection (Pad 1, Pad 2, or Both)
  - "Fill black" option for pressed uploads

### Undo System

- Snapshot-based (full canvas state per entry)
- Configurable max history (default 100, user-adjustable)
- Tracks save point for accurate dirty state
- ~53 KB per snapshot worst case (32 frames × 23×24 × 3 bytes)

### Preferences

Stored at platform-specific paths:
- Linux: `$XDG_CONFIG_HOME/stepmaniax-gif-maker/` (fallback `~/.config/...`)
- macOS: `~/Library/Application Support/stepmaniax-gif-maker/`
- Windows: `%UserProfile%\Documents\stepmaniax-gif-maker\`

Files:
- `imgui.ini` — window layout (managed by ImGui)
- `preferences.json` — keybindings, recent files, settings

### GIF Encoder Design

Custom encoder chosen over libraries for exact palette control:
- Builds global palette from all frames (no quantization/dithering)
- Uncompressed LZW (clear code + literals, resets before code table grows)
- Writes loop marker pixel on the designated frame
- Files are small (~2-5 KB for 23×24 animations)

## Coding Style

- **C++17** minimum (SDK uses C++14, but we need C++17 for structured bindings, etc.)
- **Formatting:** 4-space indentation, no tabs
- **Braces:** Same line for control flow (`if`, `for`, `while`), next line for function/class definitions
- **Naming:**
  - Functions/methods: PascalCase (`GetInputState`, `AddFrame`)
  - Member variables: plain camelCase or descriptive names (no `m_` prefix — our structs are simple)
  - Local variables: camelCase (`cellSize`, `totalFrames`, `drawColor`)
  - Constants/macros: UPPER_SNAKE_CASE
- **`using namespace std;`** in implementation files is acceptable
- **Comments:** `///` Doxygen-style for public functions; `// ---` section separators
- **Const:** Prefer `const` parameters and references
- **Error handling:** Return bool + error string out-parameter; no exceptions
- **Memory:** Stack allocation preferred; vectors for dynamic data; no raw new/delete
- **Platform:** Use `#ifdef __APPLE__` / `_WIN32` for platform-specific code
- **Dependencies:** Prefer single-header/vendored libraries; FetchContent for larger deps
- **Build:** Self-contained (no system-installed deps beyond compiler + hidapi)
- **Whitespace:** No trailing whitespace; files end with a newline

## Key Design Decisions

1. **No .smxgifs project format** — editor works on plain GIF files; composite preview loads released+pressed separately
2. **No multi-pad editing** — pads are independent; edit one GIF at a time
3. **Snapshot undo over delta** — canvas is tiny (~1.6 KB/frame), simplicity wins
4. **Custom GIF encoder over library** — exact palette control needed for SMX compatibility
5. **gif_load.h for decode** — proven (same as SDK uses), handles all GIF complexity
6. **SMX_SetLights2 for preview** — full editor control over playback (not SetAuto)
7. **All-frames color counting** — firmware uses one palette per panel across all frames
