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
