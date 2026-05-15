# TODO - Next Steps

## Completed
- [x] Project scaffolding (ImGui + SDL3 + SDK)
- [x] Pixel grid canvas with LED-only editing
- [x] Draw/Erase/Fill/Replace/Pick tools
- [x] Color palette with per-panel color count (all frames, matches firmware constraint)
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
- [x] Save color warning with per-panel all-frames detail + Quantize & Save option
- [x] Quantize panel/all (nearest-neighbor across all frames, undoable)
- [x] Copy/paste panel (right-click canvas menu)
- [x] Copy/paste entire frames (Ctrl+C/V, right-click timeline, Edit menu)
- [x] Right-click context menu with panel detection (works on non-LED spots too)
- [x] Hardware color preview (66% scaling toggle, off by default)
- [x] Preview panel: LED circles, panel outlines, dark background, zoom, independent playback
- [x] Unsaved changes prompt (New/Open/Quit/window close) with settings toggle
- [x] Loop frame marker (set/clear via right-click timeline, visual indicator, export/import)
- [x] Preview and timeline playback respect loop point
- [x] Restart button for playback from frame 0
- [x] File menu: Open (Ctrl+O), Save (Ctrl+S, falls through to Save As for new files), Save As (Ctrl+Shift+S), New (Ctrl+N), Quit (Ctrl+Q)
- [x] Window title with filename + dirty indicator (*)
- [x] Dirty state tracking (save point aware, survives undo/redo)
- [x] Platform-aware shortcut labels (Cmd on macOS, Ctrl on Windows/Linux)
- [x] Import marks dirty if non-LED pixels were stripped
- [x] Timeline controls with labeled sections and visual separators
- [x] Canvas and preview centered in their panes
- [x] Ctrl+scroll zoom for canvas and preview
- [x] Horizontal scrollbar on canvas when zoomed in
- [x] Tooltips on all interactive elements (2s hover delay)
- [x] Keyboard navigation in modal dialogs (arrow keys, Enter to confirm)
- [x] Shortcuts blocked when popups are open
- [x] Color limit check on all save paths (Save, Save As, shortcuts)
- [x] Recent files list (max 20, parent_dir/filename, grayed missing files, remove/clear options)
- [x] Cancel on file dialogs no longer triggers actions
- [x] Onion skinning (prev/next toggles, dimmed colors + corner triangles)
- [x] Recent files open directly after discard (no redundant file dialog)

## Up Next

### Hardware Features (requires connected pad)
- Live preview via SMX_SetLights2 (drive LEDs directly from editor)
- Load released + pressed GIFs for composite preview with pad input
- Firmware upload via SMX_LightsUpload_PrepareUpload + BeginUpload
- Connection status indicator
- Block upload if >15 colors per panel

## Shelved (revisit if needed)
- .smxgifs project files
- Multi-pad support
- Drag-and-drop GIF import
- Custom theming (fonts, accent colors)
