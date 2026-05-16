# StepManiaX GIF Maker [Beta]

A cross-platform pixel editor for creating, editing, previewing, and uploading animated LED GIFs to StepManiaX dance pads.

Warning: This is currently a Beta project. Expect some bugs, but please make and [issue](#reporting-issues) if you find one.

![Editor Preview](docs/screenshots/whole-app.png)

## Icon

We still need an icon for this app! I'm terrible at drawing/graphics related stuff, so if you have a suggestion let me know!

## Table of Contents

- [Features](#features)
- [Screenshots](#screenshots)
- [Installation](#installation)
- [Building](#building)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows](#windows)
  - [Dependencies](#dependencies)
- [Usage](#usage)
  - [GIF Format](#gif-format)
  - [Keyboard Shortcuts](#keyboard-shortcuts)
- [Configuration](#configuration)
- [Contributing](#contributing)
- [Reporting Issues](#reporting-issues)
- [Acknowledgments](#acknowledgments)
- [License](#license)

## Features

- **Pixel grid editor** for 14×15 (legacy) and 23×24 (modern) GIF formats
- **Drawing tools** - Draw, Erase, Fill, Replace, Pick Color
- **Animation timeline** - add/duplicate/delete/reorder frames, per-frame duration, loop point
- **Live hardware preview** - stream animations to connected pads in real-time (sync or play mode)
- **Firmware upload** - write released and pressed animations to pad EEPROM for offline playback
- **Composite preview** - load released + pressed GIFs, preview overlay behavior with live pad input
- **GIF import/export** - open and save standard GIF files compatible with the SMX SDK
- **Per-panel color tracking** - warns when exceeding the 15-color firmware limit
- **Quantization** - reduce colors to fit hardware constraints (nearest-neighbor)
- **Undo/redo** - full history with configurable depth
- **Onion skinning** - ghost previous/next frames while editing
- **Customizable keybindings** - remap all tools and timeline shortcuts
- **Cross-platform** - Windows, macOS, Linux

## Installation

Download the latest release from the [Releases page](https://github.com/fchorney/stepmaniax-gif-maker/releases).

**macOS:** Unzip, drag "StepManiaX GIF Maker.app" to your Applications folder, double-click to run. macOS will warn that the app is from an unidentified developer - click "Open Anyway" in System Settings → Privacy & Security. Alternatively, remove the quarantine flag from the terminal:
```bash
xattr -dr com.apple.quarantine "/Applications/StepManiaX GIF Maker.app"
```

**Linux:** Extract the tar.gz, run `./stepmaniax-gif-maker`. Requires `libudev` (install via `sudo apt install libudev-dev` on Debian/Ubuntu or equivalent for your distro). Optionally copy the `.desktop` file to `~/.local/share/applications/` for menu integration.

**Windows:** Unzip, double-click `stepmaniax-gif-maker.exe`. Windows SmartScreen may warn that the app is unrecognized - click "More info" then "Run anyway".

## Building

Requires: CMake 3.20+, C++17 compiler

```bash
git clone --recursive https://github.com/fchorney/stepmaniax-gif-maker.git
cd stepmaniax-gif-maker
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake libudev-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev libwayland-dev libxkbcommon-dev libegl-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./stepmaniax-gif-maker
```

### macOS

```bash
# Install dependencies (Homebrew)
brew install cmake

# Build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Run
./stepmaniax-gif-maker
```

### Windows

```powershell
# Build
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release

# Run
.\Release\stepmaniax-gif-maker.exe
```

### Dependencies

Fetched automatically by CMake (no manual install needed):
- [Dear ImGui](https://github.com/ocornut/imgui) (docking branch) - GUI framework
- [SDL3](https://github.com/libsdl-org/SDL) - windowing and input
- [nlohmann/json](https://github.com/nlohmann/json) - preferences storage
- [hidapi](https://github.com/libusb/hidapi) - USB HID communication (required by the SMX SDK)

System dependency (Linux only):
- `libudev-dev` - required by the hidapi hidraw backend

The SMX SDK is vendored as a submodule at `src/vendor/stepmaniax-sdk-mp`. Override with:
```bash
cmake .. -DSMX_SDK_DIR=/path/to/stepmaniax-sdk-mp
```

## Usage

For a detailed walkthrough of all features, see the [User Guide](docs/guide.md).

1. **File → New** to create a new animation (choose Legacy 14×15 or Modern 23×24)
2. Draw on the pixel grid - only LED positions are editable
3. Use the timeline to add frames and set durations
4. **File → Save** to export as a GIF
5. Connect a StepManiaX pad and use **Hardware → Preview on Pad** to see it live
6. **Hardware → Upload to Firmware** to write the animation permanently

### GIF Format

The editor produces GIF files compatible with the [stepmaniax-sdk-mp](https://github.com/fchorney/stepmaniax-sdk-mp) animation API:

- **14×15** - legacy 4×4 LED mode (host playback only)
- **23×24** - modern 25-LED mode (host playback + firmware upload)
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

On macOS, Ctrl shortcuts use Cmd instead.

## Configuration

All panels (Tools, Palette, Canvas, Preview, Timeline, History) are dockable - drag them to rearrange, resize by dragging borders. Your layout is saved automatically and restored on next launch.

Preferences are stored at:
- **Linux:** `$XDG_CONFIG_HOME/stepmaniax-gif-maker/` (default `~/.config/...`)
- **macOS:** `~/Library/Application Support/stepmaniax-gif-maker/`
- **Windows:** `%UserProfile%\Documents\stepmaniax-gif-maker\`

Files:
- `imgui.ini` - window layout and docking arrangement (managed by ImGui)
- `preferences.json` - user settings (see below)

### preferences.json

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `mode` | string | `"modern"` | Default canvas mode (`"modern"` or `"legacy"`) |
| `window_width` | int | 1440 | Window width in pixels |
| `window_height` | int | 900 | Window height in pixels |
| `canvas_zoom` | float | 28.0 | Canvas editor zoom level (8–40) |
| `preview_zoom` | float | 15.0 | LED preview zoom level (3–15) |
| `max_undo_history` | int | 100 | Maximum number of undo states |
| `prompt_on_unsaved` | bool | true | Warn before closing with unsaved changes |
| `recent_files` | array | `[]` | List of recently opened file paths (max 20) |
| `keybindings` | object | - | Remapped keyboard shortcuts (ImGuiKey values) |

## Contributing

Contributions are welcome! Please:

1. Fork the repository and create a branch prefixed with your initials (e.g., `fc/add-feature`).
2. Follow the code style conventions below.
3. Ensure the build passes with no warnings (`cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"`)
4. Submit a pull request with a clear description of what changed and why.

Keep PRs focused on a single concern. If you're fixing a bug and also refactoring nearby code, split them into separate PRs.

### Code style

- 4-space indentation, no tabs.
- Opening braces on the same line for control flow (`if`, `for`, `while`), next line for function/class definitions.
- Functions and methods use PascalCase (`AddFrame`, `ExportGif`).
- Local variables and struct members use camelCase (`cellSize`, `currentFrame`, `loopFrame`).
- Constants and macros use UPPER_SNAKE_CASE.
- `using namespace std;` is acceptable within implementation files.
- Doxygen-style `///` comments for public functions; `// ---` section separators.
- Prefer `const` parameters and references.
- No exceptions - use bool return + error string out-parameter for fallible operations.
- No trailing whitespace. Files end with a newline.

### Key considerations

- **Cross-platform.** All code must build on Linux, macOS, and Windows.
- **Keep dependencies minimal.** Prefer single-header/vendored libraries. FetchContent for larger deps.
- **Enforce SMX constraints.** The editor should prevent users from creating GIFs that the hardware can't handle (15 colors, 32 frames, correct dimensions).
- **ImGui best practices.** Avoid allocations in the render loop. Cache expensive computations. Use `static` locals for ImGui state that persists across frames.

## Reporting Issues

If you encounter a bug, please [open an issue](https://github.com/fchorney/stepmaniax-gif-maker/issues/new) with:

- **OS and version** (e.g., Ubuntu 24.04, macOS 15.1, Windows 11)
- **Steps to reproduce** - what you did leading up to the bug
- **Expected behavior** - what should have happened
- **Actual behavior** - what happened instead
- **GIF file** (if applicable) - attach the GIF that triggers the issue
- **Pad info** (if hardware-related) - firmware version, which panels are affected

## Acknowledgments

- [StepRevolution](https://www.steprevolution.com/) - creators of StepManiaX
- [stepmaniax-sdk](https://github.com/steprevolution/stepmaniax-sdk) - the original SDK that this project's communication layer is based on
- [stepmaniax-sdk-mp](https://github.com/fchorney/stepmaniax-sdk-mp) - the cross-platform SDK rewrite used by this editor
- [Dear ImGui](https://github.com/ocornut/imgui) - immediate-mode GUI framework
- [SDL3](https://github.com/libsdl-org/SDL) - cross-platform windowing
- [gif_load](https://github.com/hidefromkgb/gif_load) - single-header GIF decoder

## License

MIT
