/// GIF export -> import round-trip tests, including single-panel canvases and
/// the import target inference (firmware vs host).
#include "doctest/doctest.h"
#include "canvas.h"
#include "gif_export.h"
#include "gif_import.h"
#include <filesystem>
#include <string>

static std::string TempPath(const char *name)
{
    return (std::filesystem::temp_directory_path() / name).string();
}

TEST_CASE("single-panel Modern export/import round-trip")
{
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    int w = src.Width();
    src.AddFrame(); // 2 frames total
    src.frames[0].SetPixel(0, 0, w, Color{255, 0, 0}); // outer LED
    src.frames[0].SetPixel(1, 1, w, Color{0, 255, 0}); // inner LED
    src.frames[1].SetPixel(6, 6, w, Color{0, 0, 255}); // outer corner LED
    for (auto &f : src.frames) f.duration = 0.1f;      // 100ms round-trips cleanly

    std::string path = TempPath("gifmaker_rt_sp.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    bool modified = false;
    REQUIRE(ImportGif(path, dst, ierr, &modified));

    CHECK(dst.extent == CanvasExtent::SinglePanel);
    CHECK(dst.mode == CanvasMode::Modern);
    CHECK(dst.target == CanvasTarget::Host);
    CHECK(dst.Width() == 7);
    CHECK(dst.Height() == 8);
    REQUIRE((int)dst.frames.size() == 2);

    Color a = dst.frames[0].GetPixel(0, 0, w);
    CHECK(a.r == 255); CHECK(a.g == 0); CHECK(a.b == 0);
    Color b = dst.frames[0].GetPixel(1, 1, w);
    CHECK(b.r == 0); CHECK(b.g == 255); CHECK(b.b == 0);
    Color cc = dst.frames[1].GetPixel(6, 6, w);
    CHECK(cc.r == 0); CHECK(cc.g == 0); CHECK(cc.b == 255);
    CHECK(modified == false); // only LED positions were painted

    std::filesystem::remove(path);
}

TEST_CASE("loop frame marker round-trips on a single panel")
{
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    src.AddFrame();
    src.AddFrame(); // 3 frames
    src.frames[1].SetPixel(0, 0, src.Width(), Color{200, 100, 0});
    src.loopFrame = 2;

    std::string path = TempPath("gifmaker_rt_loop.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    REQUIRE(ImportGif(path, dst, ierr, nullptr));
    CHECK(dst.loopFrame == 2);

    std::filesystem::remove(path);
}

TEST_CASE("loop-end marker round-trips and clears its flag pixels")
{
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    src.AddFrame();
    src.AddFrame();
    src.AddFrame(); // 4 frames: intro 0, loop 1..2, outro 3
    src.frames[0].SetPixel(0, 0, src.Width(), Color{200, 100, 0});
    src.loopFrame = 1;
    src.loopEndFrame = 2;

    std::string path = TempPath("gifmaker_rt_loopend.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    bool modified = false;
    REQUIRE(ImportGif(path, dst, ierr, &modified));
    CHECK(dst.loopFrame == 1);
    CHECK(dst.loopEndFrame == 2);
    // Both marker pixels are cleared so they don't show in the editor, and
    // they don't count as modified pixels.
    int h = dst.Height(), w = dst.Width();
    CHECK(dst.frames[1].GetPixel(0, h - 1, w).IsBlack());
    CHECK(dst.frames[2].GetPixel(1, h - 1, w).IsBlack());
    CHECK(modified == false);

    std::filesystem::remove(path);
}

TEST_CASE("a loop end before the loop start is dropped on import")
{
    // The UI can't create this, but a hand-authored GIF could.
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::SinglePanel, CanvasTarget::Host);
    src.AddFrame();
    src.AddFrame(); // 3 frames
    src.loopFrame = 2;
    src.loopEndFrame = 0;

    std::string path = TempPath("gifmaker_rt_badend.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    REQUIRE(ImportGif(path, dst, ierr, nullptr));
    CHECK(dst.loopFrame == 2);
    CHECK(dst.loopEndFrame == -1);

    std::filesystem::remove(path);
}

TEST_CASE("full-pad Modern within caps imports as Firmware target")
{
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    src.frames[0].SetPixel(0, 0, src.Width(), Color{10, 20, 30});

    std::string path = TempPath("gifmaker_rt_full.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    REQUIRE(ImportGif(path, dst, ierr, nullptr));
    CHECK(dst.extent == CanvasExtent::FullPad);
    CHECK(dst.mode == CanvasMode::Modern);
    CHECK(dst.target == CanvasTarget::Firmware);

    std::filesystem::remove(path);
}

TEST_CASE("full-pad Modern with a loop-end marker imports as Host")
{
    // The loop-end marker is a host-playback extension, so it overrides the
    // firmware-caps guess even when the gif would fit the caps.
    Canvas src;
    src.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    src.AddFrame();
    src.AddFrame();
    src.frames[0].SetPixel(0, 0, src.Width(), Color{10, 20, 30});
    src.loopEndFrame = 1;

    std::string path = TempPath("gifmaker_rt_full_loopend.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    REQUIRE(ImportGif(path, dst, ierr, nullptr));
    CHECK(dst.extent == CanvasExtent::FullPad);
    CHECK(dst.loopEndFrame == 1);
    CHECK(dst.target == CanvasTarget::Host);

    std::filesystem::remove(path);
}

TEST_CASE("full-pad Legacy imports as Host (legacy cannot upload)")
{
    Canvas src;
    src.Init(CanvasMode::Legacy, CanvasExtent::FullPad, CanvasTarget::Firmware);
    src.frames[0].SetPixel(0, 0, src.Width(), Color{10, 20, 30});

    std::string path = TempPath("gifmaker_rt_leg.gif");
    std::string err;
    REQUIRE(ExportGif(src, path, err));

    Canvas dst;
    std::string ierr;
    REQUIRE(ImportGif(path, dst, ierr, nullptr));
    CHECK(dst.extent == CanvasExtent::FullPad);
    CHECK(dst.mode == CanvasMode::Legacy);
    CHECK(dst.target == CanvasTarget::Host);

    std::filesystem::remove(path);
}
