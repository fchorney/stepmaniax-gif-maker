#pragma once

#include <cstdint>
#include <vector>

enum class CanvasMode {
    Legacy,  // 14x15, 4x4 LEDs per panel (16 LEDs)
    Modern,  // 23x24, 4x4 outer + 3x3 inner per panel (25 LEDs)
};

struct Color {
    uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color &o) const { return !(*this == o); }
    bool IsBlack() const { return r == 0 && g == 0 && b == 0; }
};

// A single frame of the full canvas (all 9 panels).
struct CanvasFrame {
    std::vector<Color> pixels; // width * height
    float duration = 1.0f / 30.0f; // seconds

    Color GetPixel(int x, int y, int width) const;
    void SetPixel(int x, int y, int width, Color c);
};

// The full canvas state for one (pad, animation_type) combination.
struct Canvas {
    CanvasMode mode = CanvasMode::Modern;
    std::vector<CanvasFrame> frames;
    int currentFrame = 0;

    int Width() const { return mode == CanvasMode::Modern ? 23 : 14; }
    int Height() const { return mode == CanvasMode::Modern ? 24 : 15; }
    static constexpr int MaxFrames = 32;

    void Init(CanvasMode m);
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

    // Count unique non-black colors used in a panel for the current frame.
    int ColorCountForPanel(int panel) const;

    // Count unique non-black colors used in a panel across ALL frames.
    int ColorCountForPanelAllFrames(int panel) const;

    // Clear all LED positions in a specific panel to black.
    void ClearPanel(int panel);

    // Clear all LED positions in all panels to black.
    void ClearAll();

    // Quantize a panel's colors to at most maxColors across all frames.
    void QuantizePanel(int panel, int maxColors = 15);
};
