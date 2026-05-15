# StepManiaX GIF Maker

A cross-platform pixel editor for creating and uploading animated LED GIFs to StepManiaX dance pads.

## Features

- **Pixel grid editor** for 14×15 (legacy) and 23×24 (modern) GIF formats
- **Drawing tools** — Draw, Erase, Fill, Replace, Pick Color
- **Animation timeline** — add/duplicate/delete/reorder frames, per-frame duration, loop point
- **Live hardware preview** — stream animations to connected pads in real-time (sync or play mode)
- **Firmware upload** — write released and pressed animations to pad EEPROM for offline playback
- **Composite preview** — load released + pressed GIFs, preview overlay behavior with live pad input
- **GIF import/export** — open and save standard GIF files compatible with the SMX SDK
- **Per-panel color tracking** — warns when exceeding the 15-color firmware limit
- **Quantization** — reduce colors to fit hardware constraints (nearest-neighbor)
- **Undo/redo** — full history with configurable depth
- **Onion skinning** — ghost previous/next frames while editing
- **Customizable keybindings** — remap all tools and timeline shortcuts
- **Cross-platform** — Windows, macOS, Linux

## Screenshots

<!-- TODO: Add screenshots -->

## Building

Requires: CMake 3.20+, C++17 compiler, hidapi (for SDK)

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The SDK is vendored as a submodule at `src/vendor/stepmaniax-sdk-mp`. Override with:
```bash
cmake .. -DSMX_SDK_DIR=/path/to/stepmaniax-sdk-mp
```

### Dependencies

Fetched automatically by CMake:
- [Dear ImGui](https://github.com/ocornut/imgui) (docking branch) — GUI framework
- [SDL3](https://github.com/libsdl-org/SDL) — windowing and input
- [nlohmann/json](https://github.com/nlohmann/json) — preferences storage

System dependency:
- [hidapi](https://github.com/libusb/hidapi) — USB HID communication (required by the SMX SDK)

## Usage

1. **File → New** to create a new animation (choose Legacy 14×15 or Modern 23×24)
2. Draw on the pixel grid — only LED positions are editable
3. Use the timeline to add frames and set durations
4. **File → Save** to export as a GIF
5. Connect a StepManiaX pad and use **Hardware → Preview on Pad** to see it live
6. **Hardware → Upload to Firmware** to write the animation permanently

### GIF Format

The editor produces GIF files compatible with the [stepmaniax-sdk-mp](https://github.com/fchorney/stepmaniax-sdk-mp) animation API:

- **14×15** — legacy 4×4 LED mode (host playback only)
- **23×24** — modern 25-LED mode (host playback + firmware upload)
- Max 32 frames, max 15 colors per panel (for firmware upload)
- Black pixels are treated as transparent/off
- Loop point encoded as a marker pixel in the flag row

### Keyboard Shortcuts

| Action | Default |
|--------|---------|
| New | Ctrl+N |
| Open | Ctrl+O |
| Save | Ctrl+S |
| Save As | Ctrl+Shift+S |
| Quit | Ctrl+Q |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
| Copy Frame | Ctrl+C |
| Paste Frame | Ctrl+V |
| Draw | 1 |
| Erase | 2 |
| Fill | 3 |
| Replace | 4 |
| Pick Color | 5 |
| Play/Pause | Space |
| Prev/Next Frame | ←/→ |
| First/Last Frame | Home/End |
| Add Frame | A |
| Duplicate Frame | D |
| Delete Frame | Delete |
| Shift Frame | ,/. |

All tool and timeline shortcuts are remappable in **Edit → Settings**.

## Configuration

Preferences are stored at:
- **Linux:** `$XDG_CONFIG_HOME/stepmaniax-gif-maker/` (default `~/.config/...`)
- **macOS:** `~/Library/Application Support/stepmaniax-gif-maker/`
- **Windows:** `%UserProfile%\Documents\stepmaniax-gif-maker\`

## License

MIT
