# TODO - Next Steps

## Completed
- [x] Project scaffolding (ImGui + SDL3 + SDK)
- [x] Pixel grid canvas with LED-only editing
- [x] Draw/Erase/Fill/Pick tools
- [x] Color palette with per-panel color count warnings
- [x] Timeline with play/pause, frame navigation, add/dup/delete/reorder
- [x] Per-frame duration editing (with "All" button to apply to all frames)
- [x] GIF export (custom encoder, exact palettes)
- [x] GIF import (gif_load.h from SDK, strips non-LED pixels)
- [x] Undo/redo (snapshot-based, configurable history size)
- [x] History panel (visual, clickable)
- [x] Keyboard shortcuts (remappable, saved to preferences)
- [x] Settings dialog
- [x] Preferences (JSON, platform-aware config directory)
- [x] Default window layout
- [x] Mode selection (Legacy/Modern) via New dialog
- [x] 32-frame hard limit enforcement
- [x] Export color warning with per-frame/panel detail
- [x] Quantize panel/all (nearest-neighbor, undoable)
- [x] Copy/paste panel
- [x] Right-click context menu with panel detection
- [x] Hardware color preview (66% scaling toggle)
- [x] Preview panel: LED circles, panel outlines, dark background, zoom, independent playback
- [x] Unsaved changes prompt (New/Import/Quit) with settings toggle
- [x] Loop frame marker (set/clear via menu + right-click, visual indicator, export/import)
- [x] Preview and timeline playback respect loop point
- [x] Restart button for playback from frame 0

## Up Next

### Hardware Features (requires connected pad)
- Live preview via SMX_SetLights2 (drive LEDs directly from editor)
- Load released + pressed GIFs for composite preview with pad input
- Firmware upload via SMX_LightsUpload_PrepareUpload + BeginUpload
- Connection status indicator
- Block upload if >15 colors per panel

### Polish / Nice-to-Have
- Rework File menu: "Open" (import), "Save" (overwrite current file), "Save As" (save to new path) — replaces Import/Export GIF
- Onion skinning (ghost of previous/next frame)
- Copy/paste entire frames
- Custom theming (fonts, accent colors)
- Recent files list
- Drag-and-drop GIF import
- Window title shows current file name
- Dirty state indicator (asterisk in title)

## Shelved (revisit if needed)
- .smxgifs project files (not needed — editor works on single GIFs, hardware preview can load released+pressed separately)
- Multi-pad support (pads are independent, no benefit to editing both simultaneously)
