#include "canvas.h"
#include <algorithm>
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

void Canvas::Init(CanvasMode m)
{
    mode = m;
    frames.clear();
    currentFrame = 0;
    AddFrame();
}

void Canvas::AddFrame()
{
    CanvasFrame f;
    f.pixels.resize(Width() * Height(), Color{0, 0, 0});
    frames.push_back(std::move(f));
}

void Canvas::DuplicateFrame(int idx)
{
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
}

CanvasFrame &Canvas::CurrentFrame() { return frames[currentFrame]; }
const CanvasFrame &Canvas::CurrentFrame() const { return frames[currentFrame]; }

int Canvas::PanelAt(int x, int y) const
{
    if (IsGutter(x, y) || IsFlagRow(x, y)) return -1;

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

int Canvas::ColorCountForPanel(int panel) const
{
    if (panel < 0 || panel > 8) return 0;
    const auto &frame = CurrentFrame();
    int w = Width();
    int h = Height();

    std::set<uint32_t> colors;
    for (int y = 0; y < h; y++)
    {
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

void Canvas::ClearAll()
{
    for (int p = 0; p < 9; p++)
        ClearPanel(p);
}
