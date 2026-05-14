#include "gif_export.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

// Custom GIF encoder for SMX GIFs.
// Uses uncompressed LZW (clear code + literals) since our images are tiny.
// Produces exact palettes with no quantization or dithering.

namespace {

struct GifWriter {
    FILE *fp = nullptr;

    bool Open(const std::string &path) {
        fp = fopen(path.c_str(), "wb");
        return fp != nullptr;
    }
    void Write(const void *data, size_t size) { fwrite(data, 1, size, fp); }
    void WriteByte(uint8_t b) { fwrite(&b, 1, 1, fp); }
    void WriteU16(uint16_t v) { fwrite(&v, 2, 1, fp); } // little-endian
    void Close() { if (fp) fclose(fp); fp = nullptr; }
};

// Collect all unique colors across all frames, build a global palette.
// Returns palette (index 0 = black/transparent).
std::vector<Color> BuildGlobalPalette(const Canvas &canvas)
{
    std::vector<Color> palette;
    palette.push_back(Color{0, 0, 0}); // index 0 = black (transparent in SMX)

    for (const auto &frame : canvas.frames)
    {
        int w = canvas.Width(), h = canvas.Height();
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                Color c = frame.GetPixel(x, y, w);
                if (c.IsBlack()) continue;
                // Check if already in palette
                bool found = false;
                for (const auto &p : palette)
                    if (p == c) { found = true; break; }
                if (!found)
                    palette.push_back(c);
            }
        }
    }
    return palette;
}

// Find color index in palette (0 if not found / black)
uint8_t FindIndex(const std::vector<Color> &palette, Color c)
{
    if (c.IsBlack()) return 0;
    for (int i = 1; i < (int)palette.size(); i++)
        if (palette[i] == c) return (uint8_t)i;
    return 0;
}

// Determine the minimum number of bits needed for the palette (GIF requires power-of-2 table)
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

// Write uncompressed LZW image data for one frame.
// Uses the "clear code + literals" pattern to avoid actual LZW compression.
void WriteLzwData(GifWriter &w, const std::vector<uint8_t> &indices, int minCodeSize)
{
    int clearCode = 1 << minCodeSize;
    int endCode = clearCode + 1;
    int codeSize = minCodeSize + 1; // bits per output code

    // We accumulate bits into a byte buffer, then flush as sub-blocks.
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

    // In uncompressed LZW, each literal we emit causes the decoder to add one
    // entry to its string table. We must emit a clear code before the table
    // grows large enough to require a bigger code size.
    // After a clear, next table entry = clearCode + 2.
    // Code size bumps when table reaches (1 << codeSize).
    // So max literals before reset = (1 << codeSize) - (clearCode + 2) - 1.
    // The -1 is because the decoder adds the entry AFTER reading the code,
    // so on the Nth literal the table size becomes clearCode+2+N-1.
    // Actually: decoder adds entry after seeing 2nd code onwards.
    // After clear: table size = clearCode+2, next entry = clearCode+2.
    // After 1st literal: no entry added yet (need pair). table = clearCode+2.
    // After 2nd literal: entry added (code clearCode+2). table = clearCode+3.
    // After 3rd literal: entry added (code clearCode+3). table = clearCode+4.
    // ...
    // After Nth literal (N>=2): table = clearCode+2+(N-1) = clearCode+N+1.
    // Bump when table = (1 << codeSize): clearCode+N+1 = (1<<codeSize)
    // N = (1<<codeSize) - clearCode - 1
    // But we must reset BEFORE that, so max = (1<<codeSize) - clearCode - 2.
    int maxLiterals = (1 << codeSize) - clearCode - 2;
    if (maxLiterals < 1) maxLiterals = 1;

    // Emit clear code to start
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

    // Emit end code
    EmitBits(endCode, codeSize);

    // Flush remaining bits
    if (bitsInAccum > 0)
        buf.push_back((uint8_t)(bitAccum & 0xFF));

    // Write LZW minimum code size
    w.WriteByte((uint8_t)minCodeSize);

    // Write as sub-blocks (max 255 bytes each)
    int offset = 0;
    int total = (int)buf.size();
    while (offset < total)
    {
        int chunk = std::min(255, total - offset);
        w.WriteByte((uint8_t)chunk);
        w.Write(buf.data() + offset, chunk);
        offset += chunk;
    }
    w.WriteByte(0); // block terminator
}

} // namespace

bool ExportGif(const Canvas &canvas, const std::string &path, std::string &outError)
{
    if (canvas.frames.empty())
    {
        outError = "No frames to export.";
        return false;
    }

    int w = canvas.Width();
    int h = canvas.Height();

    // Build global palette
    auto palette = BuildGlobalPalette(canvas);

    // Ensure white is in the palette for the loop marker
    Color white = {255, 255, 255};
    if (canvas.loopFrame > 0)
    {
        bool hasWhite = false;
        for (const auto &c : palette)
            if (c == white) { hasWhite = true; break; }
        if (!hasWhite)
            palette.push_back(white);
    }

    if (palette.size() > 256)
    {
        outError = "Too many unique colors across all frames (max 256 for GIF).";
        return false;
    }

    int palBits = PaletteBits((int)palette.size());
    int palSize = 1 << palBits; // padded to power of 2

    GifWriter gw;
    if (!gw.Open(path))
    {
        outError = "Could not open file for writing: " + path;
        return false;
    }

    // --- GIF Header ---
    gw.Write("GIF89a", 6);

    // --- Logical Screen Descriptor ---
    gw.WriteU16((uint16_t)w);
    gw.WriteU16((uint16_t)h);
    uint8_t packed = 0x80 | ((palBits - 1) << 4) | (palBits - 1); // GCT flag + color resolution + GCT size
    gw.WriteByte(packed);
    gw.WriteByte(0); // background color index
    gw.WriteByte(0); // pixel aspect ratio

    // --- Global Color Table ---
    for (int i = 0; i < palSize; i++)
    {
        if (i < (int)palette.size())
        {
            gw.WriteByte(palette[i].r);
            gw.WriteByte(palette[i].g);
            gw.WriteByte(palette[i].b);
        }
        else
        {
            gw.WriteByte(0); gw.WriteByte(0); gw.WriteByte(0);
        }
    }

    // --- Netscape Looping Extension (loop forever) ---
    gw.WriteByte(0x21); // extension introducer
    gw.WriteByte(0xFF); // application extension
    gw.WriteByte(11);   // block size
    gw.Write("NETSCAPE2.0", 11);
    gw.WriteByte(3);    // sub-block size
    gw.WriteByte(1);    // loop sub-block id
    gw.WriteU16(0);     // loop count (0 = infinite)
    gw.WriteByte(0);    // block terminator

    // --- Frames ---
    for (int f = 0; f < (int)canvas.frames.size(); f++)
    {
        const auto &frame = canvas.frames[f];

        // Frame delay in centiseconds
        int cs = (int)(frame.duration * 100.0f + 0.5f);
        if (cs < 1) cs = 1;

        // Graphics Control Extension
        gw.WriteByte(0x21); // extension introducer
        gw.WriteByte(0xF9); // GCE label
        gw.WriteByte(4);    // block size
        gw.WriteByte(0x00); // disposal: none, no transparency
        gw.WriteU16((uint16_t)cs);
        gw.WriteByte(0);    // transparent color index (unused)
        gw.WriteByte(0);    // block terminator

        // Image Descriptor
        gw.WriteByte(0x2C); // image separator
        gw.WriteU16(0);     // left
        gw.WriteU16(0);     // top
        gw.WriteU16((uint16_t)w);
        gw.WriteU16((uint16_t)h);
        gw.WriteByte(0);    // no local color table, not interlaced

        // Build indexed pixel data
        std::vector<uint8_t> indices(w * h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                indices[y * w + x] = FindIndex(palette, frame.GetPixel(x, y, w));

        // Write loop marker: bottom-left pixel = white on the loop frame
        if (f == canvas.loopFrame && canvas.loopFrame > 0)
            indices[(h - 1) * w + 0] = FindIndex(palette, Color{255, 255, 255});

        // Write LZW data
        int minCodeSize = palBits;
        if (minCodeSize < 2) minCodeSize = 2; // GIF spec minimum
        WriteLzwData(gw, indices, minCodeSize);
    }

    // --- Trailer ---
    gw.WriteByte(0x3B);
    gw.Close();
    return true;
}
