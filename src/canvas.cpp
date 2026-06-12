/// Canvas data model implementation.
#include "canvas.h"
#include <algorithm>
#include <climits>
#include <set>

Color CanvasFrame::GetPixel(int x, int y, int width) const
{
    int idx = y * width + x;
    if (idx < 0 || idx >= (int)pixels.size()) return {};
    return pixels[idx];
}

void CanvasFrame::SetPixel(int x, int y, int width, Color c)
{
    int idx = y * width + x;
    if (idx >= 0 && idx < (int)pixels.size())
        pixels[idx] = c;
}

void Canvas::Init(CanvasMode m, CanvasExtent e, CanvasTarget t)
{
    mode = m;
    extent = e;
    target = t;
    frames.clear();
    currentFrame = 0;
    loopFrame = 0;
    loopEndFrame = -1;
    AddFrame();
}

void Canvas::AddFrame()
{
    if ((int)frames.size() >= MaxFrames()) return;
    CanvasFrame f;
    f.pixels.resize(Width() * Height(), Color{0, 0, 0});
    if (frames.empty())
    {
        frames.push_back(std::move(f));
        currentFrame = 0;
    }
    else
    {
        frames.insert(frames.begin() + currentFrame + 1, std::move(f));
        currentFrame++;
    }
}

void Canvas::DuplicateFrame(int idx)
{
    if ((int)frames.size() >= MaxFrames()) return;
    if (idx < 0 || idx >= (int)frames.size()) return;
    CanvasFrame copy = frames[idx];
    frames.insert(frames.begin() + idx + 1, std::move(copy));
    currentFrame = idx + 1;
}

void Canvas::DeleteFrame(int idx)
{
    if ((int)frames.size() <= 1) return;
    if (idx < 0 || idx >= (int)frames.size()) return;
    frames.erase(frames.begin() + idx);
    if (currentFrame >= (int)frames.size())
        currentFrame = (int)frames.size() - 1;
    if (loopFrame >= (int)frames.size())
        loopFrame = (int)frames.size() - 1;
    if (loopEndFrame >= (int)frames.size())
        loopEndFrame = (int)frames.size() - 1;
}

CanvasFrame &Canvas::CurrentFrame() { return frames[currentFrame]; }
const CanvasFrame &Canvas::CurrentFrame() const { return frames[currentFrame]; }

int Canvas::PanelAt(int x, int y) const
{
    if (IsGutter(x, y) || IsFlagRow(x, y)) return -1;

    if (extent == CanvasExtent::SinglePanel)
        return 0; // only one panel

    if (mode == CanvasMode::Modern)
    {
        // 8px panels + 1px gutters at x=7,15 and y=7,15
        int col, row;
        if (x < 8) col = 0;
        else if (x < 16) col = 1;
        else col = 2;

        if (y < 8) row = 0;
        else if (y < 16) row = 1;
        else row = 2;

        return row * 3 + col;
    }
    else
    {
        // 4px panels + 1px gutters at x=4,9 and y=4,9
        int col, row;
        if (x < 5) col = 0;
        else if (x < 10) col = 1;
        else col = 2;

        if (y < 5) row = 0;
        else if (y < 10) row = 1;
        else row = 2;

        return row * 3 + col;
    }
}

bool Canvas::IsGutter(int x, int y) const
{
    if (IsFlagRow(x, y)) return false;

    if (extent == CanvasExtent::SinglePanel)
        return false; // a single panel has no inter-panel gutters

    if (mode == CanvasMode::Modern)
        return (x == 7 || x == 15 || y == 7 || y == 15);
    else
        return (x == 4 || x == 9 || y == 4 || y == 9);
}

bool Canvas::IsFlagRow(int x, int y) const
{
    (void)x;
    return y == Height() - 1;
}

bool Canvas::IsLedPosition(int x, int y) const
{
    if (IsGutter(x, y) || IsFlagRow(x, y)) return false;

    if (extent == CanvasExtent::SinglePanel)
    {
        if (mode == CanvasMode::Modern)
        {
            // Single 7x7 panel: outer 4x4 at even coords, inner 3x3 at odd coords (<=5).
            if (x % 2 == 0 && y % 2 == 0) return true;
            if (x % 2 == 1 && y % 2 == 1 && x <= 5 && y <= 5) return true;
            return false;
        }
        // Legacy single 4x4 panel: every position is an LED.
        return x < 4 && y < 4;
    }

    if (mode == CanvasMode::Modern)
    {
        // Panel base: col*8, row*8 (gutters are at 7, 15 but panels start at 0, 8, 16)
        int col = (x < 8) ? 0 : (x < 16) ? 1 : 2;
        int row = (y < 8) ? 0 : (y < 16) ? 1 : 2;
        int localX = x - col * 8;
        int localY = y - row * 8;

        // Outer 4x4: even coords (0,2,4,6)
        if (localX % 2 == 0 && localY % 2 == 0) return true;
        // Inner 3x3: odd coords (1,3,5)
        if (localX % 2 == 1 && localY % 2 == 1 && localX <= 5 && localY <= 5) return true;
        return false;
    }
    else
    {
        // Legacy: all 4x4 pixels within each panel region are LED positions
        // Panel regions are 4x4 at (col*5, row*5)
        int col = (x < 5) ? 0 : (x < 10) ? 1 : 2;
        int row = (y < 5) ? 0 : (y < 10) ? 1 : 2;
        int localX = x - col * 5;
        int localY = y - row * 5;
        return localX < 4 && localY < 4;
    }
}

void Canvas::ClearPanel(int panel)
{
    if (panel < 0 || panel > 8) return;
    auto &frame = CurrentFrame();
    int w = Width(), h = Height();
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (PanelAt(x, y) == panel && IsLedPosition(x, y))
                frame.SetPixel(x, y, w, Color{0, 0, 0});
}

void Canvas::ClearPanelAllFrames(int panel)
{
    if (panel < 0 || panel > 8) return;
    int w = Width(), h = Height();
    for (auto &frame : frames)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (PanelAt(x, y) == panel && IsLedPosition(x, y))
                    frame.SetPixel(x, y, w, Color{0, 0, 0});
}

void Canvas::ClearAll()
{
    for (int p = 0; p < PanelCount(); p++)
        ClearPanel(p);
}

int Canvas::ColorCountForPanelAllFrames(int panel) const
{
    if (panel < 0 || panel > 8) return 0;
    int w = Width(), h = Height();
    std::set<uint32_t> colors;
    for (const auto &frame : frames)
    {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                if (PanelAt(x, y) != panel) continue;
                if (!IsLedPosition(x, y)) continue;
                Color c = frame.GetPixel(x, y, w);
                if (c.IsBlack()) continue;
                colors.insert((uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b);
            }
    }
    return (int)colors.size();
}

// Quantize a panel's palette to at most maxColors using a frequency-based approach:
// keep the most-used colors and remap the rest to their nearest neighbor (Euclidean
// distance in RGB space). This is simple and fast for the small palettes involved.
void Canvas::QuantizePanel(int panel, int maxColors)
{
    if (panel < 0 || panel > 8) return;
    int w = Width(), h = Height();

    // Collect all non-black colors used in this panel across all frames
    struct ColorCount
    {
        Color c;
        int count = 0;
    };
    std::vector<ColorCount> palette;

    for (const auto &frame : frames)
    {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                if (PanelAt(x, y) != panel) continue;
                if (!IsLedPosition(x, y)) continue;
                Color c = frame.GetPixel(x, y, w);
                if (c.IsBlack()) continue;
                bool found = false;
                for (auto &pc : palette)
                    if (pc.c == c) { pc.count++; found = true; break; }
                if (!found)
                    palette.push_back({c, 1});
            }
    }

    if ((int)palette.size() <= maxColors) return; // already within limit

    // Sort by usage count (most used first), keep top maxColors
    std::sort(palette.begin(), palette.end(), [](const ColorCount &a, const ColorCount &b) {
        return a.count > b.count;
    });

    // Build the kept palette
    std::vector<Color> kept;
    for (int i = 0; i < maxColors && i < (int)palette.size(); i++)
        kept.push_back(palette[i].c);

    // Map removed colors to nearest kept color
    auto Nearest = [&](Color c) -> Color {
        int bestDist = INT_MAX;
        Color best = kept[0];
        for (const auto &k : kept)
        {
            int dr = (int)c.r - k.r, dg = (int)c.g - k.g, db = (int)c.b - k.b;
            int dist = dr*dr + dg*dg + db*db;
            if (dist < bestDist) { bestDist = dist; best = k; }
        }
        return best;
    };

    // Replace colors in all frames
    for (auto &frame : frames)
    {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                if (PanelAt(x, y) != panel) continue;
                if (!IsLedPosition(x, y)) continue;
                Color c = frame.GetPixel(x, y, w);
                if (c.IsBlack()) continue;
                // Check if it's in the kept set
                bool isKept = false;
                for (const auto &k : kept)
                    if (k == c) { isKept = true; break; }
                if (!isKept)
                    frame.SetPixel(x, y, w, Nearest(c));
            }
    }
}
