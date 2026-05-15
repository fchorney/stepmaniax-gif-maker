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
    fread(data.data(), 1, size, fp);
    fclose(fp);

    DecodeState state;
    GIF_Load(data.data(), (long)size, FrameCallback, nullptr, &state, 0L);

    if (state.frames.empty())
    {
        outError = "Failed to decode GIF (no frames found).";
        return false;
    }

    int w = state.width, h = state.height;
    if ((w == 14 && h == 15) || (w == 23 && h == 24))
    {
        // Valid SMX dimensions
    }
    else
    {
        outError = "GIF must be 14x15 or 23x24. Got " + std::to_string(w) + "x" + std::to_string(h) + ".";
        return false;
    }

    CanvasMode mode = (w == 23) ? CanvasMode::Modern : CanvasMode::Legacy;
    canvas.mode = mode;
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

    if (pixelsModified) *pixelsModified = modified;
    return true;
}
