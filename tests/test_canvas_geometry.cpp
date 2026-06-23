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
