# TODO - Next Steps

## Completed
- [x] Project scaffolding (ImGui + SDL3 + SDK)
- [x] Pixel grid canvas with LED-only editing
- [x] Draw/Erase/Fill/Pick tools
- [x] Color palette with per-panel color count warnings
- [x] Timeline with play/pause, frame navigation, add/dup/delete/reorder
- [x] Per-frame duration editing (with "All" button, locked to frame while editing)
- [x] GIF export (custom encoder, exact palettes, loop marker)
- [x] GIF import (gif_load.h from SDK, strips non-LED pixels, reads loop marker)
- [x] Undo/redo (snapshot-based, configurable history size, saved position tracking)
- [x] History panel (visual, clickable, newest at top)
- [x] Keyboard shortcuts (remappable, saved to preferences)
- [x] Settings dialog (keybindings, max undo, prompt on unsaved)
- [x] Preferences (JSON, platform-aware config directory)
- [x] Default window layout
- [x] Mode selection (Legacy/Modern) via New dialog
- [x] 32-frame hard limit enforcement (buttons disabled at limit)
- [x] Export color warning with per-frame/panel detail + Quantize & Export option
- [x] Quantize panel/all (nearest-neighbor, undoable)
- [x] Copy/paste panel
- [x] Right-click context menu with panel detection (works on non-LED spots too)
- [x] Hardware color preview (66% scaling toggle, off by default)
- [x] Preview panel: LED circles, panel outlines, dark background, zoom, independent playback
- [x] Unsaved changes prompt (New/Open/Quit/window close) with settings toggle
- [x] Loop frame marker (set/clear via menu + right-click, visual indicator below thumbnails, export/import)
- [x] Preview and timeline playback respect loop point
- [x] Restart button for playback from frame 0
- [x] File menu rework: Open (Ctrl+O), Save (Ctrl+S), Save As (Ctrl+Shift+S), New (Ctrl+N), Quit (Ctrl+Q)
- [x] Window title with filename + dirty indicator (*)
- [x] Dirty state tracking (save point aware, survives undo/redo)
- [x] Platform-aware shortcut labels (Cmd on macOS, Ctrl on Windows/Linux)
- [x] Import marks dirty if non-LED pixels were stripped

## Up Next

### Hardware Features (requires connected pad)
- Live preview via SMX_SetLights2 (drive LEDs directly from editor)
- Load released + pressed GIFs for composite preview with pad input
- Firmware upload via SMX_LightsUpload_PrepareUpload + BeginUpload
- Connection status indicator
- Block upload if >15 colors per panel

### Polish / Nice-to-Have
- Onion skinning (ghost of previous/next frame)
- Copy/paste entire frames
- Custom theming (fonts, accent colors)
- Recent files list
- Drag-and-drop GIF import
- Frame copy/paste between frames

## Shelved (revisit if needed)
- .smxgifs project files (not needed — editor works on single GIFs, hardware preview can load released+pressed separately)
- Multi-pad support (pads are independent, no benefit to editing both simultaneously)
