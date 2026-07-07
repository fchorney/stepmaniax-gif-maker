#pragma once
/// Application-wide shared state passed to all UI and hardware modules.

#include "canvas.h"
#include "undo.h"
#include "preferences.h"
#include "imgui.h"
#include <algorithm>
#include <string>
#include <vector>
#include <atomic>

enum Tool { Tool_Draw = 0, Tool_Erase, Tool_Fill, Tool_Replace, Tool_Pick };
enum PendingAction { Pending_None = 0, Pending_New, Pending_Import, Pending_Quit };

#ifdef __APPLE__
    #define SHORTCUT_MOD "Cmd"
#else
    #define SHORTCUT_MOD "Ctrl"
#endif

/// Shared application state passed to all UI/hardware modules.
struct AppState
{
    Canvas canvas;
    UndoHistory undo;
    Preferences prefs;

    int tool = Tool_Draw;
    ImVec4 currentColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    float cellSize = 28.0f;
    float previewZoom = 15.0f;
    float previewSpeed = 1.0f; // playback/preview speed multiplier; never affects saved timing
    int singlePanelPreviewSlot = 1; // which physical pad panel (0..8) lights a single-panel canvas
    bool loopPlayback = true; // when false, playback runs once and stops on the last frame
    bool strokeActive = false;
    int rightClickPanel = -1;
    std::vector<Color> panelClipboard;
    bool panelClipboardValid = false;
    CanvasFrame frameClipboard;
    bool frameClipboardValid = false;

    // File state
    std::string currentFilePath;
    bool dirty = false;

    // Unsaved changes
    int pendingAction = Pending_None;
    bool showUnsavedDialog = false;
    bool deferOpenDialog = false;
    int deferOpenDialogFrames = 0;

    // Live hardware preview
    bool livePreview = false;
    bool livePreviewSync = true;
    double livePreviewLastSend = 0;
    double livePreviewFrameTime = 0;
    int livePreviewFrame = 0;

    // Firmware upload
    bool uploadInProgress = false;
    int uploadProgress = 0;
    bool showUploadDialog = false;
    std::string uploadError;

    // Composite preview
    bool compositePreview = false;
    bool showCompositeDialog = false;
    Canvas compositeReleased;
    Canvas compositePressed;
    bool compositeReleasedLoaded = false;
    bool compositePressedLoaded = false;
    double compositeLastSend = 0;
    double compositeFrameTime = 0;
    int compositeRelFrame = 0;
    int compositePrsFrame = 0;
    double compositePrsFrameTime = 0;
    bool compositeFillBlack = false;

    // Dialog flags
    bool showSettings = false;
    bool showNewDialog = false;
    bool showExportWarning = false;
    bool showHsvDialog = false;
    // Panels preselected when the HSV dialog opens (bit p = panel p). Set to a
    // single bit by "Adjust HSV This Panel..."; all-panels everywhere else.
    uint16_t hsvDialogPanelMask = 0x1FF;

    // Onion skin
    bool onionSkin = false;
    bool onionPrev = true;
    bool onionNext = true;

    // Multi-frame selection on the timeline (sorted, unique). Empty means
    // "just the current frame"; single-frame edits apply to every selected
    // frame when two or more are selected.
    std::vector<int> selectedFrames;
    int selectAnchor = 0; // range start for shift-click

    bool IsFrameSelected(int f) const
    {
        return std::find(selectedFrames.begin(), selectedFrames.end(), f) != selectedFrames.end();
    }

    // The frames an edit applies to: the multi-selection when two or more
    // frames are selected, otherwise just the current frame. A lone selected
    // frame never diverges from the current frame.
    std::vector<int> SelectionOrCurrent() const
    {
        std::vector<int> out;
        int n = (int)canvas.frames.size();
        for (int f : selectedFrames)
            if (f >= 0 && f < n)
                out.push_back(f);
        if (out.size() < 2)
        {
            out.clear();
            out.push_back(canvas.currentFrame);
        }
        return out;
    }

    bool HasMultiSelection() const { return SelectionOrCurrent().size() > 1; }

    void ClearFrameSelection()
    {
        selectedFrames.clear();
        selectAnchor = canvas.currentFrame;
    }

    // Cached color counts (invalidated on canvas change)
    int cachedColorCounts[9] = {};
    bool colorCountsDirty = true;

    void RefreshColorCounts()
    {
        if (!colorCountsDirty) return;
        for (int p = 0; p < 9; p++)
            cachedColorCounts[p] = canvas.ColorCountForPanelAllFrames(p);
        colorCountsDirty = false;
    }

    // App lifecycle
    bool running = true;
};

// Global file dialog state (set from SDL callbacks)
extern std::string g_exportPath;
extern bool g_exportRequested;
extern std::string g_importPath;
extern bool g_importRequested;
extern std::string g_compositeRelPath;
extern std::string g_compositePrsPath;
extern bool g_compositeRelRequested;
extern bool g_compositePrsRequested;
extern std::atomic<int> g_uploadProgress;
