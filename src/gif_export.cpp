/// Custom GIF encoder (uncompressed LZW, exact palettes).
#include "gif_export.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace {

struct GifWriter
{
    FILE *fp = nullptr;
    std::vector<char> *buf = nullptr;

    bool Open(const std::string &path)
    {
        fp = fopen(path.c_str(), "wb");
        return fp != nullptr;
    }
    void OpenMemory(std::vector<char> *outBuf) { buf = outBuf; buf->clear(); }
    void Write(const void *data, size_t size)
    {
        if (fp) fwrite(data, 1, size, fp);
        else if (buf) buf->insert(buf->end(), (const char*)data, (const char*)data + size);
    }
    void WriteByte(uint8_t b) { Write(&b, 1); }
    void WriteU16(uint16_t v)
    {
        uint8_t lo = v & 0xFF, hi = v >> 8;
        WriteByte(lo);
        WriteByte(hi);
    }
    void Close() { if (fp) fclose(fp); fp = nullptr; }
};

// Build global palette from all frames. Index 0 = black (transparent).
// Uses a hash map for O(1) deduplication.
std::vector<Color> BuildGlobalPalette(const Canvas &canvas)
{
    std::vector<Color> palette;
    palette.push_back(Color{0, 0, 0});
    std::unordered_map<uint32_t, uint8_t> seen;
    seen[0] = 0;

    for (const auto &frame : canvas.frames)
    {
        int w = canvas.Width(), h = canvas.Height();
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                Color c = frame.GetPixel(x, y, w);
                if (c.IsBlack()) continue;
                uint32_t key = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
                if (seen.find(key) == seen.end())
                {
                    seen[key] = (uint8_t)palette.size();
                    palette.push_back(c);
                }
            }
    }
    return palette;
}

// Build O(1) lookup from color to palette index.
std::unordered_map<uint32_t, uint8_t> BuildPaletteLookup(const std::vector<Color> &palette)
{
    std::unordered_map<uint32_t, uint8_t> lookup;
    for (int i = 0; i < (int)palette.size(); i++)
    {
        uint32_t key = (uint32_t)palette[i].r << 16 | (uint32_t)palette[i].g << 8 | palette[i].b;
        lookup[key] = (uint8_t)i;
    }
    return lookup;
}

uint8_t FindIndex(const std::unordered_map<uint32_t, uint8_t> &lookup, Color c)
{
    if (c.IsBlack()) return 0;
    uint32_t key = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
    auto it = lookup.find(key);
    return (it != lookup.end()) ? it->second : 0;
}

int PaletteBits(int paletteSize)
{
    if (paletteSize <= 2) return 1;
    if (paletteSize <= 4) return 2;
    if (paletteSize <= 8) return 3;
    if (paletteSize <= 16) return 4;
    if (paletteSize <= 32) return 5;
    if (paletteSize <= 64) return 6;
    if (paletteSize <= 128) return 7;
    return 8;
}

// Writes pixel indices as uncompressed LZW data.
// Strategy: emit only literal codes (no dictionary entries), resetting with a
// clear code before the implicit next code would exceed the current code size.
// This avoids needing an LZW dictionary while producing valid GIF LZW streams.
void WriteLzwData(GifWriter &w, const std::vector<uint8_t> &indices, int minCodeSize)
{
    int clearCode = 1 << minCodeSize;
    int endCode = clearCode + 1;
    int codeSize = minCodeSize + 1;

    std::vector<uint8_t> buf;
    int bitAccum = 0;
    int bitsInAccum = 0;

    auto EmitBits = [&](int code, int nbits) {
        bitAccum |= (code << bitsInAccum);
        bitsInAccum += nbits;
        while (bitsInAccum >= 8)
        {
            buf.push_back((uint8_t)(bitAccum & 0xFF));
            bitAccum >>= 8;
            bitsInAccum -= 8;
        }
    };

    // Max literals before a reset: available codes minus clear and end codes.
    // After this many literals, the next implicit code would require a larger
    // code size, so we emit a clear code to reset instead.
    int maxLiterals = (1 << codeSize) - clearCode - 2;
    if (maxLiterals < 1) maxLiterals = 1;

    EmitBits(clearCode, codeSize);

    int count = 0;
    for (int i = 0; i < (int)indices.size(); i++)
    {
        EmitBits(indices[i], codeSize);
        count++;
        if (count >= maxLiterals && i < (int)indices.size() - 1)
        {
            EmitBits(clearCode, codeSize);
            count = 0;
        }
    }

    EmitBits(endCode, codeSize);

    if (bitsInAccum > 0)
        buf.push_back((uint8_t)(bitAccum & 0xFF));

    // Write as GIF sub-blocks: length byte followed by up to 255 data bytes
    w.WriteByte((uint8_t)minCodeSize);
    int offset = 0;
    int total = (int)buf.size();
    while (offset < total)
    {
        int chunk = std::min(255, total - offset);
        w.WriteByte((uint8_t)chunk);
        w.Write(buf.data() + offset, chunk);
        offset += chunk;
    }
    w.WriteByte(0); // Block terminator
}

// Shared GIF writing logic used by both ExportGif and ExportGifToMemory.
bool WriteGifData(GifWriter &gw, const Canvas &canvas, const std::vector<Color> &palette,
                  const std::unordered_map<uint32_t, uint8_t> &lookup, std::string &)
{
    int w = canvas.Width();
    int h = canvas.Height();
    int palBits = PaletteBits((int)palette.size());
    int palSize = 1 << palBits;

    // Header
    gw.Write("GIF89a", 6);
    gw.WriteU16((uint16_t)w);
    gw.WriteU16((uint16_t)h);
    uint8_t packed = 0x80 | ((palBits - 1) << 4) | (palBits - 1);
    gw.WriteByte(packed);
    gw.WriteByte(0);
    gw.WriteByte(0);

    // Global Color Table
    for (int i = 0; i < palSize; i++)
    {
        if (i < (int)palette.size())
        { gw.WriteByte(palette[i].r); gw.WriteByte(palette[i].g); gw.WriteByte(palette[i].b); }
        else
        { gw.WriteByte(0); gw.WriteByte(0); gw.WriteByte(0); }
    }

    // Netscape Looping Extension
    gw.WriteByte(0x21); gw.WriteByte(0xFF); gw.WriteByte(11);
    gw.Write("NETSCAPE2.0", 11);
    gw.WriteByte(3); gw.WriteByte(1); gw.WriteU16(0); gw.WriteByte(0);

    // Frames
    for (int f = 0; f < (int)canvas.frames.size(); f++)
    {
        const auto &frame = canvas.frames[f];
        int cs = (int)(frame.duration * 100.0f + 0.5f);
        if (cs < 1) cs = 1;

        // Graphics Control Extension
        gw.WriteByte(0x21); gw.WriteByte(0xF9); gw.WriteByte(4);
        gw.WriteByte(0x00); gw.WriteU16((uint16_t)cs); gw.WriteByte(0); gw.WriteByte(0);

        // Image Descriptor
        gw.WriteByte(0x2C); gw.WriteU16(0); gw.WriteU16(0);
        gw.WriteU16((uint16_t)w); gw.WriteU16((uint16_t)h); gw.WriteByte(0);

        // Build indexed pixel data
        std::vector<uint8_t> indices(w * h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                indices[y * w + x] = FindIndex(lookup, frame.GetPixel(x, y, w));

        // Encode loop marker: write a white pixel at bottom-left (0, h-1) on the
        // designated loop frame. The SDK detects R>=128 at this position to know
        // which frame to loop back to during playback.
        if (f == canvas.loopFrame && canvas.loopFrame > 0)
            indices[(h - 1) * w + 0] = FindIndex(lookup, Color{255, 255, 255});

        // Encode the loop-end marker at (1, h-1): the last frame of the loop
        // region; later frames form a release outro. This is a deadsync
        // host-playback extension; SMX firmware and the official SDK ignore it.
        if (f == canvas.loopEndFrame && canvas.loopEndFrame >= 0)
            indices[(h - 1) * w + 1] = FindIndex(lookup, Color{255, 255, 255});

        int minCodeSize = palBits;
        if (minCodeSize < 2) minCodeSize = 2;
        WriteLzwData(gw, indices, minCodeSize);
    }

    // Trailer
    gw.WriteByte(0x3B);
    return true;
}

} // namespace

bool ExportGif(const Canvas &canvas, const std::string &path, std::string &outError)
{
    if (canvas.frames.empty())
    { outError = "No frames to export."; return false; }

    auto palette = BuildGlobalPalette(canvas);

    // Ensure white for the loop / loop-end markers
    if (canvas.loopFrame > 0 || canvas.loopEndFrame >= 0)
    {
        bool hasWhite = false;
        for (const auto &c : palette)
            if (c.r == 255 && c.g == 255 && c.b == 255) { hasWhite = true; break; }
        if (!hasWhite)
            palette.push_back(Color{255, 255, 255});
    }

    if (palette.size() > 256)
    { outError = "Too many unique colors across all frames (max 256 for GIF)."; return false; }

    auto lookup = BuildPaletteLookup(palette);

    GifWriter gw;
    if (!gw.Open(path))
    { outError = "Could not open file for writing: " + path; return false; }

    bool ok = WriteGifData(gw, canvas, palette, lookup, outError);
    gw.Close();
    return ok;
}

bool ExportGifToMemory(const Canvas &canvas, std::vector<char> &outData, std::string &outError)
{
    if (canvas.frames.empty())
    { outError = "No frames to export."; return false; }

    auto palette = BuildGlobalPalette(canvas);

    if (canvas.loopFrame > 0 || canvas.loopEndFrame >= 0)
    {
        bool hasWhite = false;
        for (const auto &c : palette)
            if (c.r == 255 && c.g == 255 && c.b == 255) { hasWhite = true; break; }
        if (!hasWhite)
            palette.push_back(Color{255, 255, 255});
    }

    if (palette.size() > 256)
    { outError = "Too many unique colors across all frames (max 256 for GIF)."; return false; }

    auto lookup = BuildPaletteLookup(palette);

    GifWriter gw;
    gw.OpenMemory(&outData);
    return WriteGifData(gw, canvas, palette, lookup, outError);
}
