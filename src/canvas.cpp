/// Canvas data model implementation.
#include "canvas.h"
#include <algorithm>
#include <climits>
#include <cmath>
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
    CanvasFrame f;
    f.pixels.resize(Width() * Height(), Color{0, 0, 0});
    if (frames.empty())
    {
        frames.push_back(std::move(f));
        currentFrame = 0;
        return;
    }
    InsertFrame(currentFrame + 1, f);
}

void Canvas::DuplicateFrame(int idx)
{
    if (idx < 0 || idx >= (int)frames.size()) return;
    CanvasFrame copy = frames[idx];
    InsertFrame(idx + 1, copy);
}

void Canvas::InsertFrame(int idx, const CanvasFrame &frame)
{
    if ((int)frames.size() >= MaxFrames()) return;
    if (idx < 0) idx = 0;
    if (idx > (int)frames.size()) idx = (int)frames.size();
    frames.insert(frames.begin() + idx, frame);
    currentFrame = idx;
    // Markers at or after the insertion point shift right with their frame image.
    if (loopFrame >= idx)
        loopFrame++;
    if (loopEndFrame >= idx)
        loopEndFrame++;
}

void Canvas::DeleteFrame(int idx)
{
    if ((int)frames.size() <= 1) return;
    if (idx < 0 || idx >= (int)frames.size()) return;
    frames.erase(frames.begin() + idx);
    if (currentFrame >= (int)frames.size())
        currentFrame = (int)frames.size() - 1;

    // Markers after the deleted frame shift left with their frame image. A loop
    // start on the deleted frame moves to the next frame (same index); a loop
    // end on it shrinks to the previous frame, clearing if the region vanishes.
    if (loopFrame > idx)
        loopFrame--;
    else if (loopFrame >= (int)frames.size())
        loopFrame = (int)frames.size() - 1;
    if (loopEndFrame >= idx)
        loopEndFrame--;
    if (loopEndFrame >= 0 && loopEndFrame < loopFrame)
        loopEndFrame = -1;
}

void Canvas::DeleteFrames(const std::vector<int> &indices)
{
    std::vector<int> sorted;
    for (int i : indices)
        if (i >= 0 && i < (int)frames.size())
            sorted.push_back(i);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.empty()) return;

    // Delete back to front so earlier indices stay valid; DeleteFrame keeps
    // at least one frame and tracks the loop markers.
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
        DeleteFrame(*it);

    // Land on the frame that took the first deleted slot.
    currentFrame = std::min(sorted.front(), (int)frames.size() - 1);
}

int Canvas::DuplicateFrames(const std::vector<int> &indices)
{
    std::vector<int> sorted;
    for (int i : indices)
        if (i >= 0 && i < (int)frames.size())
            sorted.push_back(i);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.empty()) return -1;

    std::vector<CanvasFrame> copies;
    for (int i : sorted)
        copies.push_back(frames[i]);

    int firstCopy = sorted.back() + 1;
    int at = firstCopy;
    for (const auto &copy : copies)
    {
        if ((int)frames.size() >= MaxFrames()) break; // stop cleanly at the cap
        InsertFrame(at++, copy);
    }
    return at > firstCopy ? firstCopy : -1;
}

int Canvas::InsertFrames(int idx, const std::vector<CanvasFrame> &newFrames)
{
    if (idx < 0) idx = 0;
    if (idx > (int)frames.size()) idx = (int)frames.size();
    int inserted = 0;
    for (const auto &f : newFrames)
    {
        if ((int)frames.size() >= MaxFrames()) break; // stop cleanly at the cap
        InsertFrame(idx + inserted, f);
        inserted++;
    }
    return inserted;
}

// Validate, sort, and dedupe a frame index list against the current frame count.
static std::vector<int> SortedValidIndices(const std::vector<int> &indices, int frameCount)
{
    std::vector<int> sorted;
    for (int i : indices)
        if (i >= 0 && i < frameCount)
            sorted.push_back(i);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    return sorted;
}

std::vector<int> Canvas::MoveFrames(const std::vector<int> &indices, int dir)
{
    std::vector<int> sorted = SortedValidIndices(indices, (int)frames.size());
    if (sorted.empty() || (dir != -1 && dir != 1))
        return sorted;

    // SwapFrames tracks the loop markers; the current frame follows its image
    // whether it is part of the move or gets hopped over.
    auto swapFollowingCurrent = [&](int a, int b)
    {
        SwapFrames(a, b);
        if (currentFrame == a) currentFrame = b;
        else if (currentFrame == b) currentFrame = a;
    };

    std::vector<int> result;
    if (dir < 0)
    {
        for (int i : sorted)
        {
            bool blocked = (i == 0) || (!result.empty() && result.back() == i - 1);
            if (!blocked)
                swapFollowingCurrent(i, i - 1);
            result.push_back(blocked ? i : i - 1);
        }
    }
    else
    {
        for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            int i = *it;
            bool blocked = (i == (int)frames.size() - 1)
                           || (!result.empty() && result.back() == i + 1);
            if (!blocked)
                swapFollowingCurrent(i, i + 1);
            result.push_back(blocked ? i : i + 1);
        }
        std::reverse(result.begin(), result.end());
    }
    return result;
}

void Canvas::ReverseFrames(const std::vector<int> &indices)
{
    std::vector<int> sorted = SortedValidIndices(indices, (int)frames.size());
    int n = (int)sorted.size();
    if (n < 2) return;

    std::vector<CanvasFrame> imgs;
    for (int i : sorted)
        imgs.push_back(std::move(frames[i]));
    for (int k = 0; k < n; k++)
        frames[sorted[k]] = std::move(imgs[n - 1 - k]);

    // Markers and the current frame follow their images to the mirrored slot.
    auto remap = [&](int m)
    {
        auto it = std::lower_bound(sorted.begin(), sorted.end(), m);
        if (it == sorted.end() || *it != m) return m;
        return sorted[n - 1 - (int)(it - sorted.begin())];
    };
    loopFrame = remap(loopFrame);
    if (loopEndFrame >= 0) loopEndFrame = remap(loopEndFrame);
    currentFrame = remap(currentFrame);
    // Reversing can invert the loop region; keep it spanning the same frames.
    if (loopEndFrame >= 0 && loopEndFrame < loopFrame)
        std::swap(loopFrame, loopEndFrame);
}

void Canvas::SwapFrames(int a, int b)
{
    if (a == b) return;
    if (a < 0 || a >= (int)frames.size()) return;
    if (b < 0 || b >= (int)frames.size()) return;
    std::swap(frames[a], frames[b]);
    if (loopFrame == a) loopFrame = b;
    else if (loopFrame == b) loopFrame = a;
    if (loopEndFrame == a) loopEndFrame = b;
    else if (loopEndFrame == b) loopEndFrame = a;
    // Following the images can invert the region (e.g. swapping the loop start
    // past the loop end); keep the region spanning the same frames.
    if (loopEndFrame >= 0 && loopEndFrame < loopFrame)
        std::swap(loopFrame, loopEndFrame);
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
    ClearPanel(panel, currentFrame);
}

void Canvas::ClearPanel(int panel, int frameIndex)
{
    if (panel < 0 || panel > 8) return;
    if (frameIndex < 0 || frameIndex >= (int)frames.size()) return;
    auto &frame = frames[frameIndex];
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

// Global position of the outer-ring LED at panel-local grid cell (dx, dy), with
// dx, dy in 0..3. The outer 4x4 ring is the only LED set the two modes share:
// Modern spaces it on even coords within an 8px panel, Legacy fills a 4px panel.
// col/row are 0 for a single-panel canvas.
static void OuterLedXY(CanvasMode mode, int col, int row, int dx, int dy, int &x, int &y)
{
    if (mode == CanvasMode::Modern)
    {
        x = col * 8 + dx * 2;
        y = row * 8 + dy * 2;
    }
    else
    {
        x = col * 5 + dx;
        y = row * 5 + dy;
    }
}

void Canvas::ConvertMode(CanvasMode newMode)
{
    if (newMode == mode) return;

    CanvasMode oldMode = mode;
    int oldW = Width();
    mode = newMode; // Width()/Height() now report the new geometry.
    int newW = Width(), newH = Height();
    int panels = PanelCount();

    for (auto &frame : frames)
    {
        std::vector<Color> dst(newW * newH, Color{0, 0, 0});
        for (int p = 0; p < panels; p++)
        {
            int col = (extent == CanvasExtent::FullPad) ? p % 3 : 0;
            int row = (extent == CanvasExtent::FullPad) ? p / 3 : 0;
            for (int dy = 0; dy < 4; dy++)
                for (int dx = 0; dx < 4; dx++)
                {
                    int sx, sy, tx, ty;
                    OuterLedXY(oldMode, col, row, dx, dy, sx, sy);
                    OuterLedXY(newMode, col, row, dx, dy, tx, ty);
                    dst[ty * newW + tx] = frame.pixels[sy * oldW + sx];
                }
        }
        frame.pixels = std::move(dst);
    }

    // Firmware upload requires a Modern canvas; a Legacy canvas can only be Host.
    if (newMode == CanvasMode::Legacy)
        target = CanvasTarget::Host;
}

Color AdjustColorHsv(Color in, const HsvAdjust &adj)
{
    float r = in.r / 255.0f, g = in.g / 255.0f, b = in.b / 255.0f;
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    float d = mx - mn;

    // RGB -> HSV.
    float h = 0.0f;
    if (d > 1e-6f)
    {
        if (mx == r) h = 60.0f * std::fmod((g - b) / d, 6.0f);
        else if (mx == g) h = 60.0f * ((b - r) / d + 2.0f);
        else h = 60.0f * ((r - g) / d + 4.0f);
        if (h < 0.0f) h += 360.0f;
    }
    float s = (mx <= 1e-6f) ? 0.0f : d / mx;
    float v = mx;

    // Apply the adjustment: hue rotates; saturation and value are gain (mul)
    // then bias (add), so a bias can push any pixel across the full range.
    h = std::fmod(h + adj.hue_deg, 360.0f);
    if (h < 0.0f) h += 360.0f;
    s = std::clamp(s * adj.sat_mul + adj.sat_add, 0.0f, 1.0f);
    v = std::clamp(v * adj.val_mul + adj.val_add, 0.0f, 1.0f);

    // HSV -> RGB.
    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rr, gg, bb;
    if (h < 60.0f) { rr = c; gg = x; bb = 0.0f; }
    else if (h < 120.0f) { rr = x; gg = c; bb = 0.0f; }
    else if (h < 180.0f) { rr = 0.0f; gg = c; bb = x; }
    else if (h < 240.0f) { rr = 0.0f; gg = x; bb = c; }
    else if (h < 300.0f) { rr = x; gg = 0.0f; bb = c; }
    else { rr = c; gg = 0.0f; bb = x; }

    auto to8 = [](float f) {
        return (uint8_t)std::lround(std::clamp((f) * 255.0f, 0.0f, 255.0f));
    };
    return Color{to8(rr + m), to8(gg + m), to8(bb + m)};
}

void Canvas::AdjustHsv(int frame_index, const HsvAdjust &adj, uint16_t panelMask)
{
    if (frame_index < 0 || frame_index >= (int)frames.size()) return;
    auto &frame = frames[frame_index];
    int w = Width(), h = Height();
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            if (!IsLedPosition(x, y)) continue;
            int panel = PanelAt(x, y);
            if (panel < 0 || !(panelMask & (1u << panel))) continue;
            frame.SetPixel(x, y, w, AdjustColorHsv(frame.GetPixel(x, y, w), adj));
        }
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
