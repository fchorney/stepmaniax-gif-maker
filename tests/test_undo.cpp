/// Undo history tests: snapshot completeness and saved-point tracking.
#include "doctest/doctest.h"
#include "undo.h"

TEST_CASE("undo restores the canvas target")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    UndoHistory u;
    u.SaveState(c, "Initial");

    c.target = CanvasTarget::Host;
    u.SaveState(c, "Target: Host");

    // Undoing the target switch must bring Firmware back.
    CHECK(u.Undo(c));
    CHECK(c.target == CanvasTarget::Firmware);
    CHECK(u.Redo(c));
    CHECK(c.target == CanvasTarget::Host);

    // Undoing a mode conversion restores the pre-conversion target too
    // (Convert to Legacy forces Host).
    Canvas m;
    m.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Firmware);
    UndoHistory mu;
    mu.SaveState(m, "Initial");
    m.ConvertMode(CanvasMode::Legacy);
    mu.SaveState(m, "Convert Mode");
    CHECK(m.target == CanvasTarget::Host);
    CHECK(mu.Undo(m));
    CHECK(m.mode == CanvasMode::Modern);
    CHECK(m.target == CanvasTarget::Firmware);
}

TEST_CASE("saved-point tracking survives history trimming")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    UndoHistory u;
    u.SetMaxHistory(10);

    u.SaveState(c, "Initial");
    u.MarkSaved();
    CHECK(!u.HasUnsavedChanges());

    // Push enough edits that the saved snapshot is trimmed out of history.
    for (int i = 0; i < 15; i++)
    {
        c.AddFrame();
        u.SaveState(c, "Add Frame");
    }
    CHECK(u.HasUnsavedChanges());

    // The saved state is gone: no reachable position may compare as saved.
    while (u.CanUndo())
    {
        u.Undo(c);
        CHECK(u.HasUnsavedChanges());
    }

    // A fresh save point works normally again.
    u.MarkSaved();
    CHECK(!u.HasUnsavedChanges());
    c.AddFrame();
    u.SaveState(c, "Add Frame");
    CHECK(u.HasUnsavedChanges());
    u.Undo(c);
    CHECK(!u.HasUnsavedChanges());
}

TEST_CASE("saved point shifts with trimming while still reachable")
{
    Canvas c;
    c.Init(CanvasMode::Modern, CanvasExtent::FullPad, CanvasTarget::Host);
    UndoHistory u;
    u.SetMaxHistory(10);

    // Fill history, save in the middle, then push a few more so the head
    // trims but the saved snapshot survives.
    for (int i = 0; i < 8; i++)
    {
        c.AddFrame();
        u.SaveState(c, "Add Frame");
    }
    u.MarkSaved();
    for (int i = 0; i < 4; i++)
    {
        c.AddFrame();
        u.SaveState(c, "Add Frame");
    }
    CHECK(u.HasUnsavedChanges());

    // Undo back exactly to the save point: must read as saved again.
    for (int i = 0; i < 4; i++)
        CHECK(u.Undo(c));
    CHECK(!u.HasUnsavedChanges());
}
