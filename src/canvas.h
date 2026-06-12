#pragma once
/// Canvas data model: pixel buffer, panels, LED positions, and frame management.

#include <climits>
#include <cstdint>
#include <vector>

enum class CanvasMode
{
    Legacy,  // 14x15, 4x4 LEDs per panel (16 LEDs)
    Modern,  // 23x24, 4x4 outer + 3x3 inner per panel (25 LEDs)
};

// Full pad (9 panels) vs a single panel. Single-panel canvases are authored for
// deadsync host playback (per-panel judgement GIFs) and cannot be uploaded to firmware.
enum class CanvasExtent
{
    FullPad,      // 3x3 grid of 9 panels
    SinglePanel,  // one panel only
};

// Firmware = EEPROM upload target (Modern + FullPad only): 32-frame and 15-color/panel caps.
// Host = deadsync set_lights playback: uncapped, full color, not uploadable.
enum class CanvasTarget
{
    Firmware,
    Host,
};

struct Color
{
    uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color &o) const { return !(*this == o); }
    bool IsBlack() const { return r == 0 && g == 0 && b == 0; }
};

// A single frame of the full canvas (all 9 panels).
struct CanvasFrame
{
    std::vector<Color> pixels; // width * height
    float duration = 1.0f / 30.0f; // seconds

    Color GetPixel(int x, int y, int width) const;
    void SetPixel(int x, int y, int width, Color c);
};

// The full canvas state for one (pad, animation_type) combination.
struct Canvas
{
    CanvasMode mode = CanvasMode::Modern;
    CanvasExtent extent = CanvasExtent::FullPad;
    CanvasTarget target = CanvasTarget::Firmware;
    std::vector<CanvasFrame> frames;
    int currentFrame = 0;
    int loopFrame = 0; // frame to loop back to (0 = loop from start)
    // Last frame of the loop region (-1 = none). Frames after it form a release
    // outro. This is a deadsync host-playback extension (per-panel judgement
    // GIFs); SMX firmware and the official SDK ignore the marker.
    int loopEndFrame = -1;

    // Canvas dimensions. Full pad is 23x24 / 14x15; a single panel is just the panel
    // area (7x7 / 4x4) plus the trailing flag row, so 7x8 / 4x5.
    int Width() const
    {
        if (extent == CanvasExtent::SinglePanel)
            return mode == CanvasMode::Modern ? 7 : 4;
        return mode == CanvasMode::Modern ? 23 : 14;
    }
    int Height() const
    {
        if (extent == CanvasExtent::SinglePanel)
            return mode == CanvasMode::Modern ? 8 : 5;
        return mode == CanvasMode::Modern ? 24 : 15;
    }

    // Number of panels on the canvas (1 for single-panel, 9 for full pad).
    int PanelCount() const { return extent == CanvasExtent::SinglePanel ? 1 : 9; }

    // Frame limit. Firmware upload caps animations at 32 frames; host playback is uncapped.
    int MaxFrames() const { return target == CanvasTarget::Firmware ? 32 : INT_MAX; }

    void Init(CanvasMode m, CanvasExtent e = CanvasExtent::FullPad,
              CanvasTarget t = CanvasTarget::Firmware);
    void AddFrame();
    void DuplicateFrame(int idx);
    void DeleteFrame(int idx);
    CanvasFrame &CurrentFrame();
    const CanvasFrame &CurrentFrame() const;

    // Returns which panel (0-8) a pixel belongs to, or -1 if it's a gutter/flag pixel.
    int PanelAt(int x, int y) const;

    // Returns true if (x, y) is a gutter pixel between panels.
    bool IsGutter(int x, int y) const;

    // Returns true if (x, y) is in the bottom flag row.
    bool IsFlagRow(int x, int y) const;

    // Returns true if (x, y) is an LED-sampled position (visible on hardware).
    bool IsLedPosition(int x, int y) const;

    // Count unique non-black colors used in a panel across ALL frames.
    int ColorCountForPanelAllFrames(int panel) const;

    // Clear all LED positions in a specific panel to black, on the current frame.
    void ClearPanel(int panel);

    // Clear a specific panel to black across every frame.
    void ClearPanelAllFrames(int panel);

    // Clear all LED positions in all panels to black.
    void ClearAll();

    // Quantize a panel's colors to at most maxColors across all frames.
    void QuantizePanel(int panel, int maxColors = 15);
};
