# StepManiaX GIF Maker - User Guide

A complete guide to creating, editing, previewing, and uploading animated LED GIFs to StepManiaX dance pads.

## Table of Contents

- [Overview](#overview)
- [Interface](#interface)
- [Canvas Editor](#canvas-editor)
- [Drawing Tools](#drawing-tools)
- [Color Palette](#color-palette)
  - [Color Picker](#color-picker)
  - [Panel Colors](#panel-colors)
- [Timeline](#timeline)
- [LED Preview](#led-preview)
- [Undo/Redo & History](#undoredo--history)
- [File Operations](#file-operations)
- [Hardware Preview](#hardware-preview)
- [Composite Preview](#composite-preview)
- [Firmware Upload](#firmware-upload)
- [Settings](#settings)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Tips & Tricks](#tips--tricks)

---

## Overview

StepManiaX GIF Maker is a pixel editor purpose-built for the LED panels on StepManiaX dance pads. It produces GIF files compatible with the SMX SDK animation format and can upload animations directly to pad firmware.

---

## Interface

The application uses a dockable panel layout. All panels can be rearranged by dragging their title bars, resized by dragging borders, and docked to any edge or combined as tabs. Your layout is saved automatically.

![Default Layout](screenshots/whole-app.png)

### Panels

| Panel | Purpose |
|-------|---------|
| Canvas | Main pixel editing area |
| Tools | Drawing tool selection |
| Palette | Color picker and per-panel color count |
| Preview | LED preview showing how the animation looks on hardware |
| Timeline | Frame management, playback controls, duration editing |
| History | Undo/redo history list |

---

## Canvas Editor

The canvas displays the full 3×3 panel grid. Only LED positions are editable - non-LED pixels are shown as dark cells and cannot be drawn on.

![Canvas Editor](screenshots/canvas.png)

The legacy canvas has no blank spaces as it is simply a 4x4 grid.

![Canvas Editor Legacy](screenshots/canvas-legacy.png)

### Canvas Modes

- **Modern (23×24)** - 25 LEDs per panel (4×4 outer + 3×3 inner grid). Supports both host playback and firmware upload.
- **Legacy (14×15)** - 16 LEDs per panel (4×4 grid). Host playback only.

The mode is chosen when creating a new file (File → New) and cannot be changed after creation.

### Navigation

- Ctrl(Cmd)+Mouse Wheel to zoom in/out on the canvas
- The grid shows panel boundaries and gutter lines

---

## Drawing Tools and Options

![Tools Panel](screenshots/tools.png)

| Tool | Description |
|------|-------------|
| **Draw** | Paint the selected color onto LED pixels |
| **Erase** | Set pixels to black (transparent/off) |
| **Fill** | Flood-fill a contiguous region with the selected color |
| **Replace** | Replace all pixels of the clicked color with the selected color (within the same panel) |
| **Pick Color** | Click a pixel to set it as the active drawing color |

Select tools from the Tools panel or use keyboard shortcuts (1–5 by default).

| Extra Options | Description |
|---------------|-------------|
| **Zoom** | Adjust canvas zoom (8px to 40px) |
| **Onion Skin** | Display previous or next frame overlay |

---

## Color Palette

![Palette Panel](screenshots/palette.png)

### Color Picker

- Select any color using the hue bar and saturation/value square
- You can also enter exact RGB/HSV/Hex values manually
- The selected color is used by the Draw, Fill, and Replace tools

### Panel Colors

- The palette displays the per-panel color count for the current animation
- Each panel's unique color count is shown across all frames
- **Firmware limit: 15 unique colors per panel** (black doesn't count)
- Panels exceeding 15 colors are highlighted with a warning
- Use **Quantize** (Edit menu) to automatically reduce colors to fit the limit

---

## Timeline

![Timeline Panel](screenshots/timeline.png)

### Frame Management

- **Add Frame** - append a new blank frame
- **Duplicate Frame** - copy the current frame
- **Delete Frame** - remove the current frame
- **Reorder** - drag frames or use Shift Left/Right shortcuts

### Playback

- **Play/Pause** - animate the timeline at the configured frame durations
- **Previous/Next Frame** - step through frames one at a time
- **First/Last Frame** - jump to the beginning or end

### Frame Duration

- Click the duration field below a frame to edit it (in milliseconds)
- Default: 100ms per frame
- Range: 10ms – 2550ms

### Loop Point

- Right-click a frame to set it as the loop point
- After the last frame plays, the animation loops back to this frame
- Default loop point is frame 0

---

## LED Preview

![LED Preview](screenshots/preview.png)

The preview panel shows how the current frame (or animation) will appear on the actual LED hardware. LEDs are rendered as circles matching the physical panel layout.

- In **play mode**, animates with proper frame timing, independently of the timeline
- In **sync frame mode**, shows the current frame that the canvas shows (follows timeline play/loop)

The SMX Hardware clamps the maximum color value to 66% as any values higher than that don't show up any brighter on the LEDs.
To see a more accurate color representation of what the hardware will look like, select the `Hardware colors (66%)` option.

---

## Undo/Redo & History

![History Panel](screenshots/history.png)

- Full snapshot-based undo/redo (configurable depth, default 100)
- The History panel shows all undo states - click any entry to jump to that state
- The current save point is marked, so you can see if you have unsaved changes

---

## File Operations

### New (Ctrl+N)

Create a new animation. Choose between Modern (23×24) and Legacy (14×15) mode.

### Open (Ctrl+O)

Open an existing GIF file. The editor auto-detects the mode from the GIF dimensions.

### Save (Ctrl+S) / Save As (Ctrl+Shift+S)

Export the animation as a GIF file compatible with the SMX SDK.

### Import Notes

- GIF files from other sources will be loaded if they match the expected dimensions
- Colors are preserved exactly - no quantization is applied on import
- The loop point marker is read from the flag row if present

---

## Hardware Preview

Stream your animation to a connected StepManiaX pad in real-time.

![Hardware Preview](screenshots/hardware-preview.png)

### Sync Mode

Mirrors the editor's current frame to the pad. As you draw or navigate frames, the pad updates instantly.

### Play Mode

Plays the animation on the pad independently with proper frame timing, regardless of what the editor is showing.

### Setup

1. Connect your SMX pad via USB
2. Go to **Hardware → Preview on Pad**
3. Select Sync or Play mode
4. Choose which pad (Pad 1, Pad 2, or Both)

---

## Composite Preview

Load both a **released** and **pressed** animation to preview how they overlay based on live pad input.

![Composite Preview](screenshots/composite-preview.png)

- The released animation plays continuously
- When you step on a panel, the pressed animation overlays on top
- Black pixels in the pressed animation are transparent (the released animation shows through)
- **Fill Black** option replaces black with (1,1,1) for full panel replacement

### Setup

1. Go to **Hardware → Composite Preview**
2. Load a released GIF and a pressed GIF
3. Step on the pad to see the overlay behavior

---

## Firmware Upload

Write animations permanently to pad EEPROM for offline playback (no computer needed).

![Firmware Upload Released](screenshots/firmware-upload-released.png)

![Firmware Upload Pressed](screenshots/firmware-upload-pressed.png)

### Requirements

- **Modern mode (23×24) only**
- **Max 32 frames** per animation
- **Max 15 colors per panel** across all frames
- A connected SMX pad

### Steps

1. Ensure your animation meets the constraints above
2. Go to **Hardware → Upload to Firmware**
3. Select animation type: Released or Pressed
4. Select target: Pad 1, Pad 2, or Both
5. Click Upload and wait for completion

### Fill Black Option

For pressed animations, enable "Fill Black" to replace black pixels with (1,1,1). This makes the pressed animation fully opaque instead of showing the released animation through black pixels.

---

## Settings

Access via **Edit → Settings**.

![Settings Dialog](screenshots/settings.png)

### General

- **Default mode** - Legacy or Modern for new files
- **Max undo history** - number of undo states to keep (default 100)
- **Prompt on unsaved changes** - warn before closing with unsaved work

### Keybindings

All tool and timeline shortcuts are remappable. Click a binding and press the desired key to reassign it.

---

## Keyboard Shortcuts

| Action | Default | Notes |
|--------|---------|-------|
| New | Ctrl+N | |
| Open | Ctrl+O | |
| Save | Ctrl+S | |
| Save As | Ctrl+Shift+S | |
| Quit | Ctrl+Q | |
| Undo | Ctrl+Z | |
| Redo | Ctrl+Y | |
| Copy Frame | Ctrl+C | |
| Paste Frame | Ctrl+V | |
| Draw | 1 | Remappable |
| Erase | 2 | Remappable |
| Fill | 3 | Remappable |
| Replace | 4 | Remappable |
| Pick Color | 5 | Remappable |
| Play/Pause | Space | Remappable |
| Prev/Next Frame | ←/→ | Remappable |
| First/Last Frame | Home/End | Remappable |
| Add Frame | A | Remappable |
| Duplicate Frame | D | Remappable |
| Delete Frame | Delete | Remappable |
| Shift Frame | ,/. | Remappable |

On macOS, Ctrl shortcuts use Cmd instead.

---

## Tips & Tricks

- **Start with the released animation** - design your idle/attract pattern first, then create a pressed animation that complements it.
- **Use Pick Color liberally** - when editing existing animations, pick colors from the canvas to stay within the 15-color limit.
- **Check color counts early** - the palette panel shows per-panel counts. Fix overages before attempting firmware upload.
- **Quantize as a last resort** - automatic quantization uses nearest-neighbor matching which may not produce ideal results. Manual color reduction gives better control.
- **Test with hardware preview** - always preview on the actual pad before uploading to firmware. Colors appear differently on LEDs vs. your monitor.
- **Use composite preview** - verify that your pressed animation looks good overlaid on the released animation with actual pad input.
- **Frame duration matters** - the pad refreshes at 30 FPS, so durations below ~33ms won't produce visible differences on hardware.
- **Black = transparent** - in pressed animations, black pixels let the released animation show through. Use (1,1,1) if you want "almost black" that's still opaque.
