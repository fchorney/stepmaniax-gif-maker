/// Canvas geometry tests: dimensions, panel mapping, LED sampling, frame caps.
#include "doctest/doctest.h"
#include "canvas.h"

// Count LED positions across the whole canvas.
static int CountLeds(const Canvas &c)
{
    int n = 0;
    for (int y = 0; y < c.Height(); y++)
        for (int x = 0; x < c.Width(); x++)
            if (c.IsLedPosition(x, y)) n++;
    return n;
}

TEST_CASE("canvas dimensions by format and extent")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad);
    CHECK(c.Width() == 23);
    CHECK(c.Height() == 24);
    CHECK(c.PanelCount() == 9);

    c.Init(CanvasMode::Legacy, CanvasExtent::FullPad);
    CHECK(c.Width() == 14);
    CHECK(c.Height() == 15);
    CHECK(c.PanelCount() == 9);

    c.Init(CanvasMode::Modern, CanvasExtent::SinglePanel);
    CHECK(c.Width() == 7);
    CHECK(c.Height() == 8);
    CHECK(c.PanelCount() == 1);

    c.Init(CanvasMode::Legacy, CanvasExtent::SinglePanel);
    CHECK(c.Width() == 4);
    CHECK(c.Height() == 5);
    CHECK(c.PanelCount() == 1);
}

TEST_CASE("single-panel LED counts match the hardware")
{
    Canvas modern;
    modern.Init(CanvasMode::Modern, CanvasExtent::SinglePanel);
    CHECK(CountLeds(modern) == 25); // 16 outer (even/even) + 9 inner (odd/odd)

    Canvas legacy;
    legacy.Init(CanvasMode::Legacy, CanvasExtent::SinglePanel);
    CHECK(CountLeds(legacy) == 16);
}

TEST_CASE("single-panel Modern LED set equals full-pad panel 0 footprint")
{
    // This is the WYSIWYG / deadsync extract_panel parity contract: a single panel
    // must sample exactly the same LEDs as the corresponding full-pad panel.
    Canvas full;
    full.Init(CanvasMode::Modern, CanvasExtent::FullPad);
    Canvas one;
    one.Init(CanvasMode::Modern, CanvasExtent::SinglePanel);

    // Panel 0 of the full pad occupies x,y in 0..6 (gutter at 7).
    for (int y = 0; y <= 6; y++)
        for (int x = 0; x <= 6; x++)
            CHECK(full.IsLedPosition(x, y) == one.IsLedPosition(x, y));
}

TEST_CASE("single-panel PanelAt and gutters")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::SinglePanel);
    for (int y = 0; y < c.Height(); y++)
        for (int x = 0; x < c.Width(); x++)
        {
            if (c.IsFlagRow(x, y))
            {
                CHECK(c.PanelAt(x, y) == -1);
            }
            else
            {
                CHECK(c.PanelAt(x, y) == 0);     // every cell belongs to the one panel
                CHECK(c.IsGutter(x, y) == false); // no internal gutters
            }
        }
}

TEST_CASE("MaxFrames depends on target")
{
    Canvas fw;
    fw.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    CHECK(fw.MaxFrames() == 32);

    Canvas host;
    host.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    CHECK(host.MaxFrames() > 100000); // effectively unbounded

    // Firmware canvas stops adding frames at the cap.
    for (int i = 0; i < 100; i++) fw.AddFrame();
    CHECK((int)fw.frames.size() == 32);

    // Host canvas keeps going (1 from Init + 50 added).
    for (int i = 0; i < 50; i++) host.AddFrame();
    CHECK((int)host.frames.size() == 51);
}

TEST_CASE("AdjustColorHsv shifts hue and applies gain/bias to saturation/value")
{
    Color red{255, 0, 0};

    // Identity returns the same color.
    Color id = AdjustColorHsv(red, HsvAdjust{});
    CHECK(id.r == 255);
    CHECK(id.g == 0);
    CHECK(id.b == 0);

    // +120 degrees of hue turns red into green.
    Color green = AdjustColorHsv(red, HsvAdjust{.hue_deg = 120.0f});
    CHECK(green.r == 0);
    CHECK(green.g == 255);
    CHECK(green.b == 0);

    // Halving value (gain) dims red to half brightness.
    Color dim = AdjustColorHsv(red, HsvAdjust{.val_mul = 0.5f});
    CHECK(dim.r == 128);
    CHECK(dim.g == 0);
    CHECK(dim.b == 0);

    // Zero saturation desaturates to grey at the same value.
    Color grey = AdjustColorHsv(red, HsvAdjust{.sat_mul = 0.0f});
    CHECK(grey.r == 255);
    CHECK(grey.g == 255);
    CHECK(grey.b == 255);

    // A hue shift leaves a grey unchanged (saturation is zero).
    Color stays = AdjustColorHsv(Color{100, 100, 100}, HsvAdjust{.hue_deg = 90.0f});
    CHECK(stays.r == 100);
    CHECK(stays.g == 100);
    CHECK(stays.b == 100);

    // A value bias lifts a dim pixel to full, which gain alone could not do.
    // Dim red (value ~0.2) * 1.0 + 1.0 bias clamps to full value.
    Color lifted = AdjustColorHsv(Color{50, 0, 0}, HsvAdjust{.val_add = 1.0f});
    CHECK(lifted.r == 255);
    CHECK(lifted.g == 0);
    CHECK(lifted.b == 0);

    // A value bias lifts black off zero (to grey, since it has no saturation).
    Color fromBlack = AdjustColorHsv(Color{0, 0, 0}, HsvAdjust{.val_add = 0.5f});
    CHECK(fromBlack.r == 128);
    CHECK(fromBlack.g == 128);
    CHECK(fromBlack.b == 128);

    // Black with no value bias stays black even under gain.
    Color black = AdjustColorHsv(Color{0, 0, 0}, HsvAdjust{.hue_deg = 45.0f, .val_mul = 4.0f});
    CHECK(black.IsBlack());
}

TEST_CASE("ConvertMode preserves the outer ring and remaps geometry")
{
    // Build a full-pad Modern canvas with two frames and paint a known color on
    // an outer-ring LED and an inner-ring LED of panel 0.
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    c.AddFrame(); // 2 frames
    c.loopFrame = 1;
    int mw = c.Width();
    const Color outer{10, 20, 30}; // panel 0 outer cell (dx=1,dy=1) -> (2,2)
    const Color inner{40, 50, 60}; // panel 0 inner cell -> (1,1), dropped on convert
    REQUIRE(c.IsLedPosition(2, 2));
    REQUIRE(c.IsLedPosition(1, 1));
    for (auto &f : c.frames)
    {
        f.SetPixel(2, 2, mw, outer);
        f.SetPixel(1, 1, mw, inner);
    }

    // Modern -> Legacy: geometry shrinks, inner ring is dropped, outer survives.
    c.ConvertMode(CanvasMode::Legacy);
    CHECK(c.mode == CanvasMode::Legacy);
    CHECK(c.Width() == 14);
    CHECK(c.Height() == 15);
    CHECK((int)c.frames.size() == 2);
    CHECK(c.loopFrame == 1);
    int lw = c.Width();
    // Modern (2,2) outer cell maps to Legacy (1,1).
    for (const auto &f : c.frames)
        CHECK(f.GetPixel(1, 1, lw) == outer);

    // Modern -> Legacy must force Host target (Legacy can't upload to firmware).
    Canvas fw;
    fw.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    fw.ConvertMode(CanvasMode::Legacy);
    CHECK(fw.target == CanvasTarget::Host);

    // Legacy -> Modern: outer ring restored at even coords, inner ring is blank.
    c.ConvertMode(CanvasMode::Modern);
    CHECK(c.mode == CanvasMode::Modern);
    CHECK(c.Width() == 23);
    int mw2 = c.Width();
    for (const auto &f : c.frames)
    {
        CHECK(f.GetPixel(2, 2, mw2) == outer); // outer ring round-trips
        CHECK(f.GetPixel(1, 1, mw2).IsBlack()); // inner ring blank, not restored
    }

    // Converting to the current mode is a no-op.
    Canvas same;
    same.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    same.ConvertMode(CanvasMode::Modern);
    CHECK(same.Width() == 7);
    CHECK((int)same.frames.size() == 1);
}

TEST_CASE("ClearPanelAllFrames clears one panel across every frame")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    c.AddFrame(); // 2 frames
    int w = c.Width();
    // An LED in panel 0 and one in panel 8, on both frames (both even/even,
    // so genuine LED positions: panel 0 at (0,0), panel 8 at (16,16)).
    const Color red{255, 0, 0};
    REQUIRE(c.PanelAt(0, 0) == 0);
    REQUIRE(c.IsLedPosition(0, 0));
    REQUIRE(c.PanelAt(16, 16) == 8);
    REQUIRE(c.IsLedPosition(16, 16));
    for (auto &f : c.frames)
    {
        f.SetPixel(0, 0, w, red);
        f.SetPixel(16, 16, w, red);
    }

    c.ClearPanelAllFrames(0);

    // Panel 0 is black on every frame; panel 8 is untouched on every frame.
    for (const auto &f : c.frames)
    {
        CHECK(f.GetPixel(0, 0, w).IsBlack());
        CHECK(f.GetPixel(16, 16, w) == red);
    }
}

// Build a Host canvas with `count` frames, each tagged by a unique duration so
// tests can tell which frame image landed where after structural edits.
static Canvas TaggedCanvas(int count)
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    while ((int)c.frames.size() < count) c.AddFrame();
    for (int i = 0; i < count; i++)
        c.frames[i].duration = 0.01f * (i + 1);
    return c;
}

TEST_CASE("loop markers stay on the same frame image across inserts")
{
    // 4 frames: intro 0, loop 1..2, outro 3.
    Canvas c = TaggedCanvas(4);
    c.loopFrame = 1;
    c.loopEndFrame = 2;

    // Insert before the loop region: both markers shift right.
    c.currentFrame = 0;
    c.AddFrame(); // blank inserted at index 1
    CHECK(c.loopFrame == 2);
    CHECK(c.loopEndFrame == 3);
    CHECK(c.frames[2].duration == doctest::Approx(0.02f));

    // Insert inside the loop region: only the loop end shifts.
    c.DuplicateFrame(2); // copy of the loop-start image at index 3
    CHECK(c.loopFrame == 2);
    CHECK(c.loopEndFrame == 4);

    // Insert after the loop region: no marker moves.
    CanvasFrame tail = c.frames.back();
    c.InsertFrame((int)c.frames.size(), tail);
    CHECK(c.loopFrame == 2);
    CHECK(c.loopEndFrame == 4);
    CHECK(c.currentFrame == (int)c.frames.size() - 1);
}

TEST_CASE("loop markers stay on the same frame image across deletes")
{
    // 5 frames: intro 0, loop 1..3, outro 4.
    Canvas c = TaggedCanvas(5);
    c.loopFrame = 1;
    c.loopEndFrame = 3;

    // Delete before the loop region: both markers shift left.
    c.DeleteFrame(0);
    CHECK(c.loopFrame == 0);
    CHECK(c.loopEndFrame == 2);
    CHECK(c.frames[0].duration == doctest::Approx(0.02f));

    // Delete inside the loop region: only the loop end shifts.
    c.DeleteFrame(1);
    CHECK(c.loopFrame == 0);
    CHECK(c.loopEndFrame == 1);

    // Delete the loop-end frame itself: the region shrinks to the previous frame.
    c.DeleteFrame(1);
    CHECK(c.loopFrame == 0);
    CHECK(c.loopEndFrame == 0);

    // Delete the frame holding both markers: the loop end clears, the loop
    // start moves to the next frame (same index).
    c.DeleteFrame(0);
    CHECK(c.loopFrame == 0);
    CHECK(c.loopEndFrame == -1);
}

TEST_CASE("deleting the loop start keeps the marker on the next frame")
{
    Canvas c = TaggedCanvas(4);
    c.loopFrame = 2;
    c.DeleteFrame(2);
    CHECK(c.loopFrame == 2); // now the image that was frame 3
    CHECK(c.frames[2].duration == doctest::Approx(0.04f));

    // Deleting a loop start that is the last frame clamps it back into range.
    c.loopFrame = 2;
    c.DeleteFrame(2);
    CHECK(c.loopFrame == 1);
}

TEST_CASE("swapping frames moves loop markers with their images")
{
    Canvas c = TaggedCanvas(4);
    c.loopFrame = 1;

    // Shift the loop-start image right: the marker follows.
    c.SwapFrames(1, 2);
    CHECK(c.loopFrame == 2);
    CHECK(c.frames[2].duration == doctest::Approx(0.02f));

    // Swapping two unmarked frames leaves markers alone.
    c.SwapFrames(0, 3);
    CHECK(c.loopFrame == 2);

    // Swapping loop start past loop end keeps the region spanning the same
    // frames instead of inverting.
    c.loopFrame = 1;
    c.loopEndFrame = 2;
    c.SwapFrames(1, 2);
    CHECK(c.loopFrame == 1);
    CHECK(c.loopEndFrame == 2);
}

TEST_CASE("DeleteFrames removes a scattered set in one pass")
{
    Canvas c = TaggedCanvas(6);
    c.loopFrame = 1;
    c.loopEndFrame = 4;

    // Duplicates and out-of-range entries are ignored.
    c.DeleteFrames({4, 0, 2, 2, 99, -1});
    REQUIRE((int)c.frames.size() == 3);
    CHECK(c.frames[0].duration == doctest::Approx(0.02f));
    CHECK(c.frames[1].duration == doctest::Approx(0.04f));
    CHECK(c.frames[2].duration == doctest::Approx(0.06f));
    // Loop start (image .02) slid to index 0; loop end (image .05, deleted)
    // shrank onto the previous surviving frame (.04, index 1).
    CHECK(c.loopFrame == 0);
    CHECK(c.loopEndFrame == 1);
    // Current frame lands where the first deleted frame was.
    CHECK(c.currentFrame == 0);

    // Deleting every frame keeps the first one.
    c.DeleteFrames({0, 1, 2});
    REQUIRE((int)c.frames.size() == 1);
    CHECK(c.frames[0].duration == doctest::Approx(0.02f));
    CHECK(c.currentFrame == 0);
}

TEST_CASE("DuplicateFrames inserts the copies as a block after the selection")
{
    Canvas c = TaggedCanvas(4);
    c.loopFrame = 3;

    int first = c.DuplicateFrames({0, 2});
    CHECK(first == 3);
    REQUIRE((int)c.frames.size() == 6);
    // Block sits after the highest selected index, in selection order.
    CHECK(c.frames[3].duration == doctest::Approx(0.01f));
    CHECK(c.frames[4].duration == doctest::Approx(0.03f));
    // The loop marker (image .04) shifted right past the inserted block.
    CHECK(c.loopFrame == 5);

    // At the firmware cap, duplication stops cleanly.
    Canvas fw;
    fw.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    while ((int)fw.frames.size() < 31) fw.AddFrame();
    std::vector<int> all;
    for (int i = 0; i < 31; i++) all.push_back(i);
    int firstFw = fw.DuplicateFrames(all); // room for only one copy
    CHECK(firstFw == 31);
    CHECK((int)fw.frames.size() == 32);
}

TEST_CASE("ClearPanel on a specific frame leaves other frames alone")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    c.AddFrame();
    c.AddFrame(); // 3 frames
    int w = c.Width();
    const Color red{255, 0, 0};
    for (auto &f : c.frames)
        f.SetPixel(0, 0, w, red); // panel 0 LED

    c.ClearPanel(0, 1);
    CHECK(c.frames[0].GetPixel(0, 0, w) == red);
    CHECK(c.frames[1].GetPixel(0, 0, w).IsBlack());
    CHECK(c.frames[2].GetPixel(0, 0, w) == red);

    // Out-of-range frame index is a no-op, not a crash.
    c.ClearPanel(0, 99);
    c.ClearPanel(0, -1);
    CHECK(c.frames[0].GetPixel(0, 0, w) == red);
}
