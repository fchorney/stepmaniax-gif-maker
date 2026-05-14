# TODO - Next Steps

## Completed
- [x] Project scaffolding (ImGui + SDL3 + SDK)
- [x] Pixel grid canvas with LED-only editing
- [x] Draw/Erase/Fill/Pick tools
- [x] Color palette with per-panel color count warnings
- [x] Timeline with play/pause, frame navigation, add/dup/delete/reorder
- [x] Per-frame duration editing
- [x] GIF export (custom encoder, exact palettes)
- [x] GIF import (gif_load.h from SDK)
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

## Up Next

### .smxgifs Project Files
- Save/load full projects as ZIP archives (.smxgifs extension)
- Contains: pad1_released.gif, pad1_pressed.gif, pad2_released.gif, pad2_pressed.gif, metadata.json
- Metadata: mode, loop frames, project settings
- File → Save / File → Open .smxgifs

### Multi-Pad Support
- Tab-based UI: "Pad 1 | Pad 2" with "Released | Pressed" toggle
- Each tab has its own canvas (independent animations)
- Side-by-side preview of both pads animating independently

### Preview Panel Improvements
- Render LEDs at physical positions (circles, not just pixels)
- Show panel outlines like the physical pad shape
- Adjustable preview zoom
- Animate preview in real-time (independent of timeline scrubbing)

### Loop Frame Marker
- UI to set which frame is the loop point
- Visual indicator on timeline (loop arrow icon)
- Write the white pixel marker on export

### Hardware Features (requires connected pad)
- Live preview via SMX_SetLights2 (drive LEDs directly from editor)
- Firmware upload via SMX_LightsUpload_PrepareUpload + BeginUpload
- Connection status indicator
- Block upload if >15 colors per panel

### Polish / Nice-to-Have
- Onion skinning (ghost of previous/next frame)
- Copy/paste entire frames
- Custom theming (fonts, accent colors)
- Confirmation dialog on Import GIF (warns about discarding current work)
- Recent files list
- Drag-and-drop GIF import
- Window title shows current file name
- Dirty state indicator (unsaved changes)
