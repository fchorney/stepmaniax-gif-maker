/// GIF decoder using gif_load.h from the SMX SDK.
#include "gif_import.h"
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "vendor/stepmaniax-sdk-mp/src/vendor/gif_load.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace {

struct DecodeState
{
    std::vector<CanvasFrame> frames;
    std::vector<uint8_t> canvas; // composited RGBA
    std::vector<uint8_t> prevCanvas;
    int width = 0;
    int height = 0;
};

void FrameCallback(void *data, struct GIF_WHDR *whdr)
{
    auto *state = static_cast<DecodeState*>(data);

    if (whdr->ifrm == 0)
    {
        state->width = whdr->xdim;
        state->height = whdr->ydim;
        state->canvas.assign(whdr->xdim * whdr->ydim * 4, 0);
    }

    if (whdr->mode == GIF_PREV)
        state->prevCanvas = state->canvas;

    int frxd = whdr->frxd, fryd = whdr->fryd;
    int frxo = whdr->frxo, fryo = whdr->fryo;

    for (int y = 0; y < fryd; y++)
    {
        int srcY = y;
        if (whdr->intr)
        {
            // GIF interlaced frames store rows in 4 passes:
            //   Pass 0: rows 0, 8, 16, ... (start=0, step=8)
            //   Pass 1: rows 4, 12, 20, ... (start=4, step=8)
            //   Pass 2: rows 2, 6, 10, ... (start=2, step=4)
            //   Pass 3: rows 1, 3, 5, ...  (start=1, step=2)
            // Map sequential storage index y to actual display row srcY.
            static const int starts[] = {0, 4, 2, 1};
            static const int steps[]  = {8, 8, 4, 2};
            int row = 0;
            for (int pass = 0; pass < 4; pass++)
                for (int r = starts[pass]; r < fryd; r += steps[pass])
                {
                    if (row == y) { srcY = r; goto found; }
                    row++;
                }
            found:;
        }

        for (int x = 0; x < frxd; x++)
        {
            int palIdx = whdr->bptr[srcY * frxd + x];
            if (palIdx == whdr->tran) continue;

            int dstIdx = ((fryo + y) * state->width + (frxo + x)) * 4;
            state->canvas[dstIdx + 0] = whdr->cpal[palIdx].R;
            state->canvas[dstIdx + 1] = whdr->cpal[palIdx].G;
            state->canvas[dstIdx + 2] = whdr->cpal[palIdx].B;
            state->canvas[dstIdx + 3] = 255;
        }
    }

    // Store frame
    CanvasFrame frame;
    int w = state->width, h = state->height;
    frame.pixels.resize(w * h);
    for (int i = 0; i < w * h; i++)
    {
        frame.pixels[i].r = state->canvas[i * 4 + 0];
        frame.pixels[i].g = state->canvas[i * 4 + 1];
        frame.pixels[i].b = state->canvas[i * 4 + 2];
    }

    // Convert GIF delay (centiseconds) to seconds.
    // Treat 3cs (30ms) and 4cs (40ms) as exactly 1/30s since SMX hardware
    // runs at 30 FPS and many tools round differently for that rate.
    int ms = whdr->time * 10;
    if (ms == 30 || ms == 40)
        frame.duration = 1.0f / 30.0f;
    else if (ms <= 0)
        frame.duration = 1.0f / 30.0f;
    else
        frame.duration = ms / 1000.0f;

    state->frames.push_back(std::move(frame));

    // Disposal
    if (whdr->mode == GIF_BKGD)
    {
        for (int y = 0; y < fryd; y++)
            for (int x = 0; x < frxd; x++)
            {
                int idx = ((fryo + y) * state->width + (frxo + x)) * 4;
                state->canvas[idx + 0] = 0;
                state->canvas[idx + 1] = 0;
                state->canvas[idx + 2] = 0;
                state->canvas[idx + 3] = 0;
            }
    }
    else if (whdr->mode == GIF_PREV)
    {
        state->canvas = state->prevCanvas;
    }
}

} // namespace

bool ImportGif(const std::string &path, Canvas &canvas, std::string &outError, bool *pixelsModified)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        outError = "Could not open file: " + path;
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::vector<char> data(size);
    if ((long)fread(data.data(), 1, size, fp) != size)
    {
        fclose(fp);
        outError = "Failed to read file: " + path;
        return false;
    }
    fclose(fp);

    DecodeState state;
    GIF_Load(data.data(), (long)size, FrameCallback, nullptr, &state, 0L);

    if (state.frames.empty())
    {
        outError = "Failed to decode GIF (no frames found).";
        return false;
    }

    int w = state.width, h = state.height;

    // Infer the canvas shape from the GIF dimensions.
    //   23x24 / 14x15 -> full pad (Modern / Legacy)
    //   7x8  / 4x5    -> single panel (Modern / Legacy), authored for host playback
    CanvasExtent extent;
    CanvasMode mode;
    if (w == 23 && h == 24)      { extent = CanvasExtent::FullPad;     mode = CanvasMode::Modern; }
    else if (w == 14 && h == 15) { extent = CanvasExtent::FullPad;     mode = CanvasMode::Legacy; }
    else if (w == 7 && h == 8)   { extent = CanvasExtent::SinglePanel; mode = CanvasMode::Modern; }
    else if (w == 4 && h == 5)   { extent = CanvasExtent::SinglePanel; mode = CanvasMode::Legacy; }
    else
    {
        outError = "GIF must be 23x24 / 14x15 (full pad) or 7x8 / 4x5 (single panel). Got "
                   + std::to_string(w) + "x" + std::to_string(h) + ".";
        return false;
    }

    canvas.mode = mode;
    canvas.extent = extent;
    canvas.frames = std::move(state.frames);
    canvas.currentFrame = 0;

    // Detect loop frame marker: bottom-left pixel (0, h-1) with R >= 128
    canvas.loopFrame = 0;
    for (int f = 0; f < (int)canvas.frames.size(); f++)
    {
        Color c = canvas.frames[f].GetPixel(0, h - 1, w);
        if (c.r >= 128)
        {
            canvas.loopFrame = f;
            // Clear the marker pixel so it doesn't show in the editor
            canvas.frames[f].SetPixel(0, h - 1, w, Color{0, 0, 0});
            break;
        }
    }

    // Detect the loop-end marker: pixel (1, h-1) with R >= 128 marks the last
    // frame of the loop region; later frames form a release outro (a deadsync
    // host-playback extension; firmware ignores it).
    canvas.loopEndFrame = -1;
    for (int f = 0; f < (int)canvas.frames.size(); f++)
    {
        Color c = canvas.frames[f].GetPixel(1, h - 1, w);
        if (c.r >= 128)
        {
            canvas.loopEndFrame = f;
            canvas.frames[f].SetPixel(1, h - 1, w, Color{0, 0, 0});
            break;
        }
    }
    // A loop end before the loop start is author error; deadsync ignores it too.
    if (canvas.loopEndFrame >= 0 && canvas.loopEndFrame < canvas.loopFrame)
        canvas.loopEndFrame = -1;

    // Clear non-LED pixels (gutters, flag row, non-sampled positions)
    bool modified = false;
    for (auto &frame : canvas.frames)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (!canvas.IsLedPosition(x, y))
                {
                    Color c = frame.GetPixel(x, y, w);
                    if (!c.IsBlack()) modified = true;
                    frame.SetPixel(x, y, w, Color{0, 0, 0});
                }

    // Target: firmware upload is only possible for a full-pad Modern animation that fits the
    // firmware caps (<=32 frames and <=15 colors per panel). Everything else is host-only.
    canvas.target = CanvasTarget::Firmware;
    if (extent != CanvasExtent::FullPad || mode != CanvasMode::Modern
        || (int)canvas.frames.size() > 32)
    {
        canvas.target = CanvasTarget::Host;
    }
    else
    {
        for (int p = 0; p < canvas.PanelCount(); p++)
            if (canvas.ColorCountForPanelAllFrames(p) > 15)
            {
                canvas.target = CanvasTarget::Host;
                break;
            }
    }

    if (pixelsModified) *pixelsModified = modified;
    return true;
}
