# StepManiaX GIF Maker

A cross-platform GUI tool for creating, editing, previewing, and uploading animated GIFs to StepManiaX dance pads.

## Features (Planned)

- Pixel grid editor for 14×15 (legacy) and 23×24 (modern) GIF formats
- Per-panel color palette tracking (15-color limit enforcement)
- Live hardware preview via SMX_SetLights2
- Firmware upload for offline playback
- Side-by-side dual-pad preview
- GIF import/export
- Project files (.smxgifs) bundling all animations + metadata

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

## License

MIT
