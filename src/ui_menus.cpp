/// Menu bar rendering and all popup/modal dialogs.
#include "ui_menus.h"
#include "gif_export.h"
#include "gif_import.h"
#include "imgui_internal.h" // g.NavCursorVisible for the modal Enter-confirm fallback
#include <SMX.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>

#define ICON_EXTERNAL_LINK "\xc2\xbb" // Unicode right-pointing double angle quotation mark (>>)

using namespace std;

static void ExportDialogCallback(void *userdata, const char * const *filelist, int filter)
{
    (void)userdata; (void)filter;
    if (filelist && filelist[0] && filelist[0][0] != '\0')
    {
        g_exportPath = filelist[0];
        g_exportRequested = true;
    }
}

static void ImportDialogCallback(void *userdata, const char * const *filelist, int filter)
{
    (void)userdata; (void)filter;
    if (filelist && filelist[0] && filelist[0][0] != '\0')
    {
        g_importPath = filelist[0];
        g_importRequested = true;
    }
}

static void CompositeRelCallback(void *userdata, const char * const *filelist, int filter)
{
    (void)userdata; (void)filter;
    if (filelist && filelist[0] && filelist[0][0] != '\0')
    { g_compositeRelPath = filelist[0]; g_compositeRelRequested = true; }
}

static void CompositePrsCallback(void *userdata, const char * const *filelist, int filter)
{
    (void)userdata; (void)filter;
    if (filelist && filelist[0] && filelist[0][0] != '\0')
    { g_compositePrsPath = filelist[0]; g_compositePrsRequested = true; }
}

// Enter (or keypad Enter) confirms a modal's default action, but only while
// the nav cursor is hidden: once arrow keys move the visible focus to some
// button, Enter and Space go to that button through ImGui's own navigation.
// (ImGui hides the nav cursor again on any mouse click, so a mouse-opened
// dialog otherwise ignores Enter entirely.)
static bool ModalDefaultConfirm()
{
    ImGuiContext &g = *ImGui::GetCurrentContext();
    if (ImGui::GetIO().WantTextInput)
        return false;
    // Stand down only when ImGui's own navigation will handle the activation,
    // which requires BOTH a focused item and a visible nav cursor (the exact
    // precondition of its activation path). Anything less (e.g. the cursor
    // flagged visible with no item focused, which happens a few frames after
    // a modal takes focus) would swallow the key with no visible effect.
    if (g.NavId != 0 && g.NavCursorVisible && g.NavWindow
        && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        return false;
    return ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)
           || ImGui::IsKeyPressed(ImGuiKey_Space, false);
}

// Escape cancels/dismisses a modal. ImGui itself deliberately never closes
// modal popups on Escape, so each dialog wires this into its cancel action.
// Skipped while a text field is active: Escape then reverts the field's edit.
static bool ModalCancelPressed()
{
    return !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

static void UploadProgressCb(int progress, void *pUser)
{
    (void)pUser;
    g_uploadProgress.store(progress);
}

void RenderMenus(AppState &app, SDL_Window *window)
{
    ImGuiIO &io = ImGui::GetIO();
    static bool showAbout = false;

    // Deferred file open dialog (needs to happen after popups close)
    if (app.deferOpenDialog)
    {
        app.deferOpenDialogFrames++;
        if (app.deferOpenDialogFrames >= 2)
        {
            app.deferOpenDialog = false;
            app.deferOpenDialogFrames = 0;
            SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
            SDL_ShowOpenFileDialog(ImportDialogCallback, nullptr, window, filters, 1, nullptr, false);
        }
    }

    // --- Menu Bar ---
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New...", SHORTCUT_MOD "+N"))
            {
                if (app.prefs.promptOnUnsaved && app.dirty)
                    { app.pendingAction = Pending_New; app.showUnsavedDialog = true; }
                else
                    app.showNewDialog = true;
            }
            if (ImGui::MenuItem("Open...", SHORTCUT_MOD "+O"))
            {
                if (app.prefs.promptOnUnsaved && app.dirty)
                    { g_importPath.clear(); app.pendingAction = Pending_Import; app.showUnsavedDialog = true; }
                else
                {
                    SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                    SDL_ShowOpenFileDialog(ImportDialogCallback, nullptr, window, filters, 1, nullptr, false);
                }
            }
            if (ImGui::BeginMenu("Open Recent", !app.prefs.recentFiles.empty()))
            {
                // Cache file existence on menu open (avoid stat() every frame)
                static std::vector<bool> recentExists;
                if (ImGui::IsWindowAppearing() || recentExists.size() != app.prefs.recentFiles.size())
                {
                    recentExists.resize(app.prefs.recentFiles.size());
                    for (int i = 0; i < (int)app.prefs.recentFiles.size(); i++)
                    {
                        struct stat st;
                        recentExists[i] = (stat(app.prefs.recentFiles[i].c_str(), &st) == 0);
                    }
                }

                for (int idx = 0; idx < (int)app.prefs.recentFiles.size(); idx++)
                {
                    const auto &recent = app.prefs.recentFiles[idx];
                    string display;
                    size_t sep = recent.find_last_of("/\\");
                    if (sep != string::npos)
                    {
                        size_t sep2 = recent.find_last_of("/\\", sep - 1);
                        display = (sep2 != string::npos) ? recent.substr(sep2 + 1) : recent.substr(0, sep) + "/" + recent.substr(sep + 1);
                    }
                    else
                        display = recent;
                    bool exists = (idx < (int)recentExists.size()) ? recentExists[idx] : false;
                    if (ImGui::MenuItem(display.c_str(), nullptr, false, exists))
                    {
                        if (app.prefs.promptOnUnsaved && app.dirty)
                        {
                            g_importPath = recent;
                            app.pendingAction = Pending_Import;
                            app.showUnsavedDialog = true;
                        }
                        else
                        {
                            g_importPath = recent;
                            g_importRequested = true;
                        }
                    }
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::Text("%s", recent.c_str());
                        if (!exists) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "(file not found)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Remove Missing Files"))
                {
                    app.prefs.recentFiles.erase(
                        remove_if(app.prefs.recentFiles.begin(), app.prefs.recentFiles.end(),
                            [](const string &f) { struct stat st; return stat(f.c_str(), &st) != 0; }),
                        app.prefs.recentFiles.end());
                }
                if (ImGui::MenuItem("Clear Recent Files"))
                    app.prefs.recentFiles.clear();
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", SHORTCUT_MOD "+S"))
            {
                bool overLimit = false;
                if (app.canvas.target == CanvasTarget::Firmware)
                    for (int p = 0; p < app.canvas.PanelCount(); p++)
                        if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                            overLimit = true;
                if (overLimit)
                    app.showExportWarning = true;
                else if (app.currentFilePath.empty())
                {
                    SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                    SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
                }
                else
                {
                    string err;
                    if (ExportGif(app.canvas, app.currentFilePath, err))
                    {
                        app.dirty = false;
                        app.undo.MarkSaved();
                    }
                }
            }
            if (ImGui::MenuItem("Save As...", SHORTCUT_MOD "+Shift+S"))
            {
                bool overLimit = false;
                if (app.canvas.target == CanvasTarget::Firmware)
                    for (int p = 0; p < app.canvas.PanelCount(); p++)
                        if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                            overLimit = true;
                if (overLimit)
                    app.showExportWarning = true;
                else
                {
                    SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                    SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", SHORTCUT_MOD "+Q"))
            {
                if (app.prefs.promptOnUnsaved && app.dirty)
                    { app.pendingAction = Pending_Quit; app.showUnsavedDialog = true; }
                else
                    app.running = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", SHORTCUT_MOD "+Z", false, app.undo.CanUndo())) { app.undo.Undo(app.canvas); app.ClearFrameSelection(); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
            if (ImGui::MenuItem("Redo", SHORTCUT_MOD "+Y", false, app.undo.CanRedo())) { app.undo.Redo(app.canvas); app.ClearFrameSelection(); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
            ImGui::Separator();
            // Single-frame edits apply to every selected frame when the
            // timeline has a multi-selection.
            bool multiSel = app.HasMultiSelection();
            if (ImGui::MenuItem(multiSel ? "Copy Frames" : "Copy Frame", SHORTCUT_MOD "+C"))
            {
                app.frameClipboard.clear();
                for (int fi : app.SelectionOrCurrent())
                    app.frameClipboard.push_back(app.canvas.frames[fi]);
            }
            if (ImGui::MenuItem(app.frameClipboard.size() > 1 ? "Paste Frames" : "Paste Frame", SHORTCUT_MOD "+V", false, app.CanPasteFrames()))
            {
                int first = app.canvas.currentFrame + 1;
                int count = app.canvas.InsertFrames(first, app.frameClipboard);
                app.ClearFrameSelection();
                for (int i = first; i < first + count && count > 1; i++)
                    app.selectedFrames.push_back(i);
                app.selectAnchor = first;
                app.dirty = true; app.colorCountsDirty = true;
                app.undo.SaveState(app.canvas, count > 1 ? "Paste Frames" : "Paste Frame");
            }
            // Reverse the selected frames, or the whole animation without a
            // multi-selection.
            if (ImGui::MenuItem(multiSel ? "Reverse Frames (Selected)" : "Reverse Frames (All)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.reverseFrames), false, multiSel || (int)app.canvas.frames.size() > 1))
            {
                std::vector<int> rev;
                if (multiSel)
                    rev = app.SelectionOrCurrent();
                else
                    for (int i = 0; i < (int)app.canvas.frames.size(); i++) rev.push_back(i);
                app.canvas.ReverseFrames(rev);
                app.dirty = true; app.colorCountsDirty = true;
                app.undo.SaveState(app.canvas, multiSel ? "Reverse Frames (Selected)" : "Reverse Frames");
            }
            ImGui::Separator();
            if (ImGui::BeginMenu(multiSel ? "Clear Panel (Selected Frames)" : "Clear Panel"))
            {
                for (int p = 0; p < app.canvas.PanelCount(); p++)
                {
                    char label[16];
                    snprintf(label, sizeof(label), "Panel %d", p);
                    if (ImGui::MenuItem(label))
                    {
                        for (int fi : app.SelectionOrCurrent()) app.canvas.ClearPanel(p, fi);
                        app.dirty = true; app.colorCountsDirty = true;
                        app.undo.SaveState(app.canvas, multiSel ? "Clear Panel (Selected Frames)" : "Clear Panel");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Clear Panel (All Frames)"))
            {
                for (int p = 0; p < app.canvas.PanelCount(); p++)
                {
                    char label[16];
                    snprintf(label, sizeof(label), "Panel %d", p);
                    if (ImGui::MenuItem(label)) { app.canvas.ClearPanelAllFrames(p); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Clear Panel (All Frames)"); }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(multiSel ? "Clear All Panels (Selected Frames)" : "Clear All Panels"))
            {
                for (int fi : app.SelectionOrCurrent())
                    for (int p = 0; p < app.canvas.PanelCount(); p++)
                        app.canvas.ClearPanel(p, fi);
                app.dirty = true; app.colorCountsDirty = true;
                app.undo.SaveState(app.canvas, multiSel ? "Clear All (Selected Frames)" : "Clear All");
            }
            if (ImGui::BeginMenu("Quantize Panel", app.canvas.target == CanvasTarget::Firmware))
            {
                for (int p = 0; p < app.canvas.PanelCount(); p++)
                {
                    char label[32];
                    snprintf(label, sizeof(label), "Panel %d", p);
                    if (ImGui::MenuItem(label)) { app.canvas.QuantizePanel(p); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Quantize Panel"); }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Quantize All Panels", nullptr, false, app.canvas.target == CanvasTarget::Firmware))
            {
                for (int p = 0; p < app.canvas.PanelCount(); p++) app.canvas.QuantizePanel(p);
                app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Quantize All");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Adjust HSV...", SHORTCUT_MOD "+E")) { app.hsvDialogPanelMask = 0x1FF; app.showHsvDialog = true; }
            ImGui::Separator();
            // Convert between LED densities. Modern packs 25 LEDs/panel (outer 4x4
            // ring + inner 3x3); Legacy has only the outer 16. Modern->Legacy drops
            // the inner ring; Legacy->Modern adds a blank one.
            {
                bool isModern = app.canvas.mode == CanvasMode::Modern;
                const char *convLabel = isModern ? "Convert to Legacy (16 LED/panel)"
                                                  : "Convert to Modern (25 LED/panel)";
                if (ImGui::MenuItem(convLabel))
                {
                    app.canvas.ConvertMode(isModern ? CanvasMode::Legacy : CanvasMode::Modern);
                    app.dirty = true;
                    app.colorCountsDirty = true;
                    app.undo.SaveState(app.canvas, "Convert Mode");
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(isModern
                        ? "Drop each panel's inner 3x3 ring, keeping the outer 4x4 (16 LEDs).\nLegacy is host-playback only; it cannot be uploaded to firmware."
                        : "Add a blank inner 3x3 ring to each panel (25 LEDs).\nModern is required for firmware upload.");
            }
            ImGui::Separator();
            // Convert the open canvas between targets. Opening a gif guesses its
            // target from the firmware caps, which mislabels a host-authored gif
            // that happens to fit them; this lets the author correct it.
            if (ImGui::BeginMenu("Target"))
            {
                bool isFirmware = app.canvas.target == CanvasTarget::Firmware;
                bool shapeOk = app.canvas.mode == CanvasMode::Modern
                               && app.canvas.extent == CanvasExtent::FullPad;
                bool framesOk = (int)app.canvas.frames.size() <= 32;
                bool colorsOk = true;
                if (shapeOk && framesOk)
                    for (int p = 0; p < app.canvas.PanelCount(); p++)
                        if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                        {
                            colorsOk = false;
                            break;
                        }
                bool firmwareOk = shapeOk && framesOk && colorsOk;
                if (ImGui::MenuItem("Firmware (EEPROM upload; 32-frame, 15-color caps)", nullptr,
                                    isFirmware, firmwareOk)
                    && !isFirmware)
                {
                    app.canvas.target = CanvasTarget::Firmware;
                    app.undo.SaveState(app.canvas, "Target: Firmware");
                }
                if (!firmwareOk && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    if (!shapeOk)
                        ImGui::SetTooltip("Firmware upload needs a Modern full-pad canvas.");
                    else if (!framesOk)
                        ImGui::SetTooltip("Firmware upload caps animations at 32 frames;\ndelete frames first.");
                    else
                        ImGui::SetTooltip("A panel uses more than 15 colors;\nquantize panels first (Edit > Quantize).");
                }
                if (ImGui::MenuItem("Host (deadsync playback; uncapped, no upload)", nullptr,
                                    !isFirmware)
                    && isFirmware)
                {
                    app.canvas.target = CanvasTarget::Host;
                    app.undo.SaveState(app.canvas, "Target: Host");
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Host playback unlocks the loop-end / outro marker\nand removes the frame and color caps.");
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Settings...")) app.showSettings = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware"))
        {
            SMXInfo info0, info1;
            SMX_GetInfo(0, &info0);
            SMX_GetInfo(1, &info1);
            if (info0.m_bConnected)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Pad 1: Connected");
            else
                ImGui::TextDisabled("Pad 1: Not connected");
            if (info1.m_bConnected)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Pad 2: Connected");
            else
                ImGui::TextDisabled("Pad 2: Not connected");
            ImGui::Separator();
            if (ImGui::MenuItem("Preview on Pad", nullptr, app.livePreview, info0.m_bConnected || info1.m_bConnected))
            {
                app.livePreview = !app.livePreview;
                if (app.livePreview)
                {
                    app.livePreviewLastSend = 0;
                    app.livePreviewFrameTime = 0;
                    app.livePreviewFrame = 0;
                }
                else
                    SMX_ReenableAutoLights();
            }
            if (app.livePreview)
            {
                if (ImGui::MenuItem("  Sync to Editor", nullptr, app.livePreviewSync))
                    app.livePreviewSync = true;
                if (ImGui::MenuItem("  Play Animation", nullptr, !app.livePreviewSync))
                {
                    app.livePreviewSync = false;
                    app.livePreviewFrame = 0;
                    app.livePreviewFrameTime = 0;
                }
            }
            if (ImGui::BeginMenu("Upload to Firmware", (info0.m_bConnected || info1.m_bConnected) && app.canvas.target == CanvasTarget::Firmware))
            {
                auto doUpload = [&](bool pad0, bool pad1, SMX_LightsType type, bool fillBlack = false) {
                    if (app.canvas.mode != CanvasMode::Modern)
                        app.uploadError = "Upload requires Modern (23x24) mode.";
                    else if ((int)app.canvas.frames.size() > 32)
                        app.uploadError = "Too many frames (max 32).";
                    else
                    {
                        bool overLimit = false;
                        for (int p = 0; p < 9; p++)
                            if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                                overLimit = true;
                        if (overLimit)
                            app.uploadError = "One or more panels exceed 15 colors.";
                        else
                            app.uploadError.clear();
                    }

                    if (!app.uploadError.empty())
                    { app.showUploadDialog = true; return; }

                    vector<char> gifData;
                    string err;
                    if (!ExportGifToMemory(app.canvas, gifData, err))
                    { app.uploadError = "Failed to encode GIF: " + err; app.showUploadDialog = true; return; }

                    if (fillBlack)
                    {
                        Canvas tempCanvas = app.canvas;
                        int w = tempCanvas.Width(), h = tempCanvas.Height();
                        for (auto &f : tempCanvas.frames)
                            for (int y = 0; y < h; y++)
                                for (int x = 0; x < w; x++)
                                    if (tempCanvas.IsLedPosition(x, y) && f.GetPixel(x, y, w).IsBlack())
                                        f.SetPixel(x, y, w, Color{1, 1, 1});
                        gifData.clear();
                        if (!ExportGifToMemory(tempCanvas, gifData, err))
                        { app.uploadError = "Failed to encode GIF: " + err; app.showUploadDialog = true; return; }
                    }

                    const char *prepErr = nullptr;
                    bool ok = true;
                    if (pad0)
                    {
                        if (!SMX_LightsUpload_PrepareUpload(gifData.data(), (int)gifData.size(), 0, type, &prepErr))
                        { app.uploadError = prepErr ? prepErr : "Prepare failed for pad 1."; ok = false; }
                    }
                    if (ok && pad1)
                    {
                        if (!SMX_LightsUpload_PrepareUpload(gifData.data(), (int)gifData.size(), 1, type, &prepErr))
                        { app.uploadError = prepErr ? prepErr : "Prepare failed for pad 2."; ok = false; }
                    }
                    if (ok)
                    {
                        app.uploadInProgress = true;
                        g_uploadProgress.store(0);
                        if (pad0) SMX_LightsUpload_BeginUpload(0, UploadProgressCb, nullptr);
                        if (pad1) SMX_LightsUpload_BeginUpload(1, UploadProgressCb, nullptr);
                    }
                    else
                        app.showUploadDialog = true;
                };

                if (ImGui::BeginMenu("Released"))
                {
                    if (ImGui::MenuItem("Pad 1", nullptr, false, info0.m_bConnected))
                        doUpload(true, false, SMX_LightsType_Released);
                    if (ImGui::MenuItem("Pad 2", nullptr, false, info1.m_bConnected))
                        doUpload(false, true, SMX_LightsType_Released);
                    if (ImGui::MenuItem("Both Pads", nullptr, false, info0.m_bConnected && info1.m_bConnected))
                        doUpload(true, true, SMX_LightsType_Released);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Pressed"))
                {
                    ImGui::TextDisabled("Black pixels are transparent (overlay only).");
                    ImGui::TextDisabled("Enable 'Fill black' to fully replace released.");
                    static bool fillBlackOnPressed = false;
                    ImGui::Checkbox("Fill black pixels with (1,1,1)", &fillBlackOnPressed);
                    if (ImGui::BeginItemTooltip()) { ImGui::Text("Replace black (transparent) pixels with near-black\nso pressed animation fully replaces released"); ImGui::EndTooltip(); }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Pad 1", nullptr, false, info0.m_bConnected))
                        doUpload(true, false, SMX_LightsType_Pressed, fillBlackOnPressed);
                    if (ImGui::MenuItem("Pad 2", nullptr, false, info1.m_bConnected))
                        doUpload(false, true, SMX_LightsType_Pressed, fillBlackOnPressed);
                    if (ImGui::MenuItem("Both Pads", nullptr, false, info0.m_bConnected && info1.m_bConnected))
                        doUpload(true, true, SMX_LightsType_Pressed, fillBlackOnPressed);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            // Composite preview plays full-pad GIFs across the whole stage, so it
            // is disabled for single-panel canvases like the other full-pad-only
            // hardware actions.
            if (ImGui::MenuItem("Composite Preview...", nullptr, false,
                                (info0.m_bConnected || info1.m_bConnected) &&
                                    app.canvas.extent == CanvasExtent::FullPad))
                app.showCompositeDialog = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Re-enable Auto Lights", nullptr, false, info0.m_bConnected || info1.m_bConnected))
                SMX_ReenableAutoLights();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Guide " ICON_EXTERNAL_LINK))
                SDL_OpenURL("https://github.com/fchorney/stepmaniax-gif-maker/blob/main/docs/guide.md");
            if (ImGui::MenuItem("Report Issue " ICON_EXTERNAL_LINK))
                SDL_OpenURL("https://github.com/fchorney/stepmaniax-gif-maker/issues/new");
            ImGui::Separator();
            if (ImGui::MenuItem("About"))
                showAbout = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // --- Settings Popup ---
    if (app.showSettings)
    {
        ImGui::OpenPopup("Settings");
        app.showSettings = false;
    }

    static int *rebindTarget = nullptr;
    if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Keybindings");
        ImGui::Separator();

        // Whether a key capture was armed at the top of the frame: a key that
        // completes or cancels the capture (inside KeybindRow, which renders
        // before the Close button) must not also confirm/close the dialog.
        bool captureWasArmed = (rebindTarget != nullptr);

        auto KeybindRow = [&](const char *label, int *key) {
            ImGui::Text("%s:", label);
            ImGui::SameLine(120);
            if (rebindTarget == key)
            {
                ImGui::Button("... press a key ...", ImVec2(150, 0));
                for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++)
                {
                    if (k == ImGuiKey_Escape) continue;
                    if (ImGui::IsKeyPressed((ImGuiKey)k))
                    {
                        *key = k;
                        rebindTarget = nullptr;
                        break;
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                    rebindTarget = nullptr;
            }
            else
            {
                char btnLabel[64];
                snprintf(btnLabel, sizeof(btnLabel), "%s##%s", ImGui::GetKeyName((ImGuiKey)*key), label);
                if (ImGui::Button(btnLabel, ImVec2(150, 0)))
                    rebindTarget = key;
            }
        };

        ImGui::Text("Tools");
        ImGui::Separator();
        KeybindRow("Draw", &app.prefs.keys.draw);
        KeybindRow("Erase", &app.prefs.keys.erase);
        KeybindRow("Fill", &app.prefs.keys.fill);
        KeybindRow("Replace", &app.prefs.keys.replace);
        KeybindRow("Pick Color", &app.prefs.keys.pick);

        ImGui::Spacing();
        ImGui::Text("Timeline");
        ImGui::Separator();
        KeybindRow("Play/Pause", &app.prefs.keys.playPause);
        KeybindRow("Prev Frame", &app.prefs.keys.prevFrame);
        KeybindRow("Next Frame", &app.prefs.keys.nextFrame);
        KeybindRow("First Frame", &app.prefs.keys.firstFrame);
        KeybindRow("Last Frame", &app.prefs.keys.lastFrame);
        KeybindRow("Add Frame", &app.prefs.keys.addFrame);
        KeybindRow("Dup Frame", &app.prefs.keys.dupFrame);
        KeybindRow("Delete Frame", &app.prefs.keys.deleteFrame);
        KeybindRow("Shift Left", &app.prefs.keys.shiftLeft);
        KeybindRow("Shift Right", &app.prefs.keys.shiftRight);
        KeybindRow("Hold Sim", &app.prefs.keys.holdSim);
        KeybindRow("Reverse Frames", &app.prefs.keys.reverseFrames);

        ImGui::Separator();
        if (ImGui::Button("Reset Keybinds to Defaults"))
        {
            app.prefs.keys = Keybindings{};
            rebindTarget = nullptr;
        }

        ImGui::Spacing();
        ImGui::Text("General");
        ImGui::Separator();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Max Undo History", &app.prefs.maxUndoHistory))
        {
            if (app.prefs.maxUndoHistory < 10) app.prefs.maxUndoHistory = 10;
            if (app.prefs.maxUndoHistory > 10000) app.prefs.maxUndoHistory = 10000;
            app.undo.SetMaxHistory(app.prefs.maxUndoHistory);
        }
        ImGui::Checkbox("Prompt on unsaved changes", &app.prefs.promptOnUnsaved);

        static const char *modeLabels[] = { "Modern (23x24)", "Legacy (14x15)" };
        int modeIdx = (app.prefs.mode == "legacy") ? 1 : 0;
        ImGui::SetNextItemWidth(160);
        if (ImGui::Combo("Default Mode", &modeIdx, modeLabels, 2))
            app.prefs.mode = (modeIdx == 1) ? "legacy" : "modern";

        ImGui::Separator();
        ImGui::SameLine();
        if (ImGui::Button("Close") || (!captureWasArmed && rebindTarget == nullptr && (ModalDefaultConfirm() || ModalCancelPressed())))
        {
            rebindTarget = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Adjust HSV Popup ---
    // Live-previews on the canvas: each frame the popup recomputes the affected
    // frames from a snapshot taken on open, so the sliders are non-destructive
    // until Apply (and fully reverted on Cancel).
    if (app.showHsvDialog)
    {
        ImGui::OpenPopup("Adjust HSV");
        app.showHsvDialog = false;
    }
    static bool hsvInit = false;
    static std::vector<CanvasFrame> hsvSnapshot;
    static std::vector<int> hsvSelection;
    static int hsvFrame = 0;
    static HsvAdjust hsvAdj;
    enum { HsvScope_Current = 0, HsvScope_Selected, HsvScope_All };
    static int hsvScope = HsvScope_Current;
    static uint16_t hsvMask = 0x1FF;
    if (ImGui::BeginPopupModal("Adjust HSV", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!hsvInit)
        {
            hsvSnapshot = app.canvas.frames;
            hsvSelection = app.SelectionOrCurrent();
            hsvFrame = app.canvas.currentFrame;
            hsvAdj = HsvAdjust{};
            // Default the scope to the timeline selection; never leave a stale
            // "selected" scope active without one.
            if (app.HasMultiSelection())
                hsvScope = HsvScope_Selected;
            else if (hsvScope == HsvScope_Selected)
                hsvScope = HsvScope_Current;
            hsvMask = (app.canvas.PanelCount() == 1) ? 0x1 : app.hsvDialogPanelMask;
            hsvInit = true;
        }

        // Saturation and value each have a gain (x) and a bias (+/-). The bias
        // is what lets dim or black pixels reach the full range; gain alone is
        // relative to each pixel's original level.
        ImGui::SetNextItemWidth(220);
        ImGui::SliderFloat("Hue shift", &hsvAdj.hue_deg, -180.0f, 180.0f, "%.0f deg");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(220);
        ImGui::SliderFloat("Saturation x", &hsvAdj.sat_mul, 0.0f, 4.0f, "%.2fx");
        ImGui::SetNextItemWidth(220);
        ImGui::SliderFloat("Saturation +/-", &hsvAdj.sat_add, -1.0f, 1.0f, "%+.2f");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(220);
        ImGui::SliderFloat("Value x", &hsvAdj.val_mul, 0.0f, 4.0f, "%.2fx");
        ImGui::SetNextItemWidth(220);
        ImGui::SliderFloat("Value +/-", &hsvAdj.val_add, -1.0f, 1.0f, "%+.2f");
        ImGui::Spacing();
        ImGui::TextDisabled("Apply to:");
        ImGui::SameLine();
        ImGui::RadioButton("Current frame", &hsvScope, HsvScope_Current);
        ImGui::SameLine();
        bool hasSelection = hsvSelection.size() > 1;
        if (!hasSelection) ImGui::BeginDisabled();
        ImGui::RadioButton("Selected frames", &hsvScope, HsvScope_Selected);
        if (!hasSelection) ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::RadioButton("All frames", &hsvScope, HsvScope_All);
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset"))
            hsvAdj = HsvAdjust{};

        // Panel scope: a 3x3 grid of checkboxes mirroring the pad layout.
        if (app.canvas.PanelCount() > 1)
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Panels:");
            ImGui::SameLine();
            if (ImGui::SmallButton("All##hsvpanels")) hsvMask = 0x1FF;
            ImGui::SameLine();
            if (ImGui::SmallButton("None##hsvpanels")) hsvMask = 0;
            for (int row = 0; row < 3; row++)
            {
                for (int col = 0; col < 3; col++)
                {
                    int p = row * 3 + col;
                    if (col > 0) ImGui::SameLine();
                    char lbl[16];
                    snprintf(lbl, sizeof(lbl), "%d##hsvp%d", p, p);
                    bool on = (hsvMask >> p) & 1;
                    if (ImGui::Checkbox(lbl, &on))
                        hsvMask = on ? (uint16_t)(hsvMask | (1u << p))
                                     : (uint16_t)(hsvMask & ~(1u << p));
                }
            }
        }

        // Recompute the preview from the snapshot every frame.
        app.canvas.frames = hsvSnapshot;
        if (hsvScope == HsvScope_All)
            for (int i = 0; i < (int)app.canvas.frames.size(); i++)
                app.canvas.AdjustHsv(i, hsvAdj, hsvMask);
        else if (hsvScope == HsvScope_Selected)
            for (int i : hsvSelection)
                app.canvas.AdjustHsv(i, hsvAdj, hsvMask);
        else
            app.canvas.AdjustHsv(hsvFrame, hsvAdj, hsvMask);
        app.colorCountsDirty = true;

        ImGui::Separator();
        bool noPanels = (hsvMask == 0);
        if (noPanels) ImGui::BeginDisabled();
        if (ImGui::Button("Apply") || (!noPanels && ModalDefaultConfirm()))
        {
            bool allPanels = (app.canvas.PanelCount() == 1) || hsvMask == 0x1FF;
            app.dirty = true;
            app.colorCountsDirty = true;
            const char *label =
                hsvScope == HsvScope_All
                    ? (allPanels ? "Adjust HSV (all frames)" : "Adjust HSV (panels, all frames)")
                : hsvScope == HsvScope_Selected
                    ? (allPanels ? "Adjust HSV (selected frames)" : "Adjust HSV (panels, selected frames)")
                    : (allPanels ? "Adjust HSV" : "Adjust HSV (panels)");
            app.undo.SaveState(app.canvas, label);
            hsvInit = false;
            ImGui::CloseCurrentPopup();
        }
        if (noPanels) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ModalCancelPressed())
        {
            app.canvas.frames = hsvSnapshot;
            app.colorCountsDirty = true;
            hsvInit = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    else if (hsvInit)
    {
        // Dismissed without Apply or Cancel (e.g. Escape): revert the live
        // preview exactly like Cancel, or it stays baked into the canvas
        // without an undo entry.
        app.canvas.frames = hsvSnapshot;
        app.colorCountsDirty = true;
        hsvInit = false;
    }

    // --- GIF Export handling ---
    static string exportError;
    static bool showExportResult = false;
    static bool exportSuccess = false;

    if (g_exportRequested)
    {
        g_exportRequested = false;
        string path = g_exportPath;
        if (!path.empty())
        {
            if (path.size() < 4 || path.substr(path.size() - 4) != ".gif")
                path += ".gif";
            exportSuccess = ExportGif(app.canvas, path, exportError);
            if (exportSuccess)
            {
                app.currentFilePath = path;
                app.prefs.AddRecentFile(path);
                app.dirty = false; app.undo.MarkSaved();
            }
            showExportResult = true;
        }
    }

    if (app.showExportWarning)
    {
        bool stillOverLimit = false;
        for (int p = 0; p < 9; p++)
            if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                stillOverLimit = true;
        if (stillOverLimit)
            ImGui::OpenPopup("Save Warning");
        app.showExportWarning = false;
    }
    if (ImGui::BeginPopupModal("Save Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "Some panels exceed the 15-color limit:");
        ImGui::BeginChild("##colorwarn", ImVec2(300, 150), true);
        for (int p = 0; p < 9; p++)
        {
            int count = app.canvas.ColorCountForPanelAllFrames(p);
            if (count > 15)
                ImGui::Text("Panel %d: %d colors (across all frames)", p, count);
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Text("The GIF will work for host playback but cannot be\nuploaded to firmware. Save anyway?");
        ImGui::Separator();
        if (ImGui::Button("Quantize & Save"))
        {
            for (int p = 0; p < 9; p++) app.canvas.QuantizePanel(p);
            app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Quantize All");
            SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
            SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Anyway"))
        {
            SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
            SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ModalDefaultConfirm() || ModalCancelPressed())
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    if (showExportResult)
    {
        ImGui::OpenPopup("Save Result");
        showExportResult = false;
    }
    if (ImGui::BeginPopupModal("Save Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (exportSuccess)
            ImGui::Text("GIF saved successfully!");
        else
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Save failed: %s", exportError.c_str());
        if (ImGui::Button("OK") || ModalDefaultConfirm() || ModalCancelPressed())
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    // --- Unsaved Changes Dialog ---
    if (app.showUnsavedDialog)
    {
        ImGui::OpenPopup("Unsaved Changes");
        app.showUnsavedDialog = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You have unsaved changes. Discard them?");
        ImGui::Separator();
        if (ImGui::Button("Discard"))
        {
            int action = app.pendingAction;
            app.pendingAction = Pending_None;
            ImGui::CloseCurrentPopup();
            if (action == Pending_New)
                app.showNewDialog = true;
            else if (action == Pending_Import)
            {
                if (!g_importPath.empty())
                    g_importRequested = true;
                else
                    app.deferOpenDialog = true;
            }
            else if (action == Pending_Quit)
                app.running = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ModalDefaultConfirm() || ModalCancelPressed())
        {
            app.pendingAction = Pending_None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    // --- GIF Import handling ---
    static string importError;
    static bool showImportResult = false;
    static bool importSuccess = false;

    if (g_importRequested)
    {
        g_importRequested = false;
        bool pixelsModified = false;
        importSuccess = ImportGif(g_importPath, app.canvas, importError, &pixelsModified);
        if (importSuccess)
        {
            app.currentFilePath = g_importPath;
            app.prefs.AddRecentFile(g_importPath);
            app.showExportWarning = false;
            app.ClearFrameSelection();
            app.undo.Clear();
            app.undo.SaveState(app.canvas, "Open");
            app.colorCountsDirty = true;
            if (pixelsModified)
            {
                app.dirty = true;
            }
            else
            {
                app.dirty = false; app.undo.MarkSaved();
            }
        }
        else
            showImportResult = true;
    }

    if (showImportResult)
    {
        ImGui::OpenPopup("Import Result");
        showImportResult = false;
    }
    if (ImGui::BeginPopupModal("Import Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Import failed: %s", importError.c_str());
        if (ImGui::Button("OK") || ModalDefaultConfirm() || ModalCancelPressed())
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    // --- Composite Preview Dialog ---
    if (app.showCompositeDialog)
    {
        ImGui::OpenPopup("Composite Preview");
        app.showCompositeDialog = false;
    }
    if (ImGui::BeginPopupModal("Composite Preview", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {

        ImGui::Text("Released Animation:");
        ImGui::SameLine();
        if (app.compositeReleasedLoaded)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded (%d frames)", (int)app.compositeReleased.frames.size());
        else
            ImGui::TextDisabled("Not loaded");
        if (ImGui::Button("Load from File##rel"))
        {
            SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
            SDL_ShowOpenFileDialog(CompositeRelCallback, nullptr, window, filters, 1, nullptr, false);
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Load released animation from a GIF file"); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button("Use Current GIF##rel"))
        {
            app.compositeReleased = app.canvas;
            app.compositeReleasedLoaded = true;
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Use the GIF currently open in the editor"); ImGui::EndTooltip(); }

        ImGui::Spacing();
        ImGui::Text("Pressed Animation:");
        ImGui::SameLine();
        if (app.compositePressedLoaded)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded (%d frames)", (int)app.compositePressed.frames.size());
        else
            ImGui::TextDisabled("Not loaded (optional)");
        if (ImGui::Button("Load from File##prs"))
        {
            SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
            SDL_ShowOpenFileDialog(CompositePrsCallback, nullptr, window, filters, 1, nullptr, false);
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Load pressed animation from a GIF file"); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button("Use Current GIF##prs"))
        {
            app.compositePressed = app.canvas;
            app.compositePressedLoaded = true;
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Use the GIF currently open in the editor"); ImGui::EndTooltip(); }

        if (app.compositePressedLoaded)
        {
            ImGui::Checkbox("Fill black pixels (fully replace released)", &app.compositeFillBlack);
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Black pixels become opaque instead of transparent\nso pressed animation fully covers released"); ImGui::EndTooltip(); }
        }

        ImGui::Separator();
        if (!app.compositePreview)
        {
            if (ImGui::Button("Start", ImVec2(80, 0)))
            {
                if (app.compositeReleasedLoaded)
                {
                    app.compositePreview = true;
                    app.compositeLastSend = 0;
                    app.compositeFrameTime = 0;
                    app.compositeRelFrame = 0;
                    app.compositePrsFrame = 0;
                    app.compositePrsFrameTime = 0;
                    app.livePreview = false;
                }
            }
        }
        else
        {
            if (ImGui::Button("Stop", ImVec2(80, 0)))
            {
                app.compositePreview = false;
                SMX_ReenableAutoLights();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Playing...");
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(80, 0)) || ModalDefaultConfirm() || ModalCancelPressed())
        {
            if (app.compositePreview)
            {
                app.compositePreview = false;
                SMX_ReenableAutoLights();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Upload Dialog ---
    if (app.showUploadDialog)
    {
        ImGui::OpenPopup("Upload");
        app.showUploadDialog = false;
    }
    if (app.uploadInProgress)
    {
        ImGui::OpenPopup("Upload");
        app.uploadProgress = g_uploadProgress.load();
        if (app.uploadProgress >= 100)
            app.uploadInProgress = false;
    }
    if (ImGui::BeginPopupModal("Upload", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!app.uploadError.empty())
        {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Upload failed: %s", app.uploadError.c_str());
            if (ImGui::Button("OK") || ModalDefaultConfirm() || ModalCancelPressed())
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();
        }
        else if (app.uploadProgress >= 100)
        {
            ImGui::Text("Upload complete!");
            if (ImGui::Button("OK") || ModalDefaultConfirm() || ModalCancelPressed())
                ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("Uploading to firmware...");
            ImGui::ProgressBar(app.uploadProgress / 100.0f);
        }
        ImGui::EndPopup();
    }

    // --- New Dialog ---
    if (app.showNewDialog)
    {
        ImGui::OpenPopup("New Animation");
        app.showNewDialog = false;
    }
    if (ImGui::BeginPopupModal("New Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create a new animation. This will discard current work.");
        ImGui::Separator();
        static int newMode = 1;   // 0 = Legacy, 1 = Modern
        static int newExtent = 0; // 0 = Full pad, 1 = Single panel
        static int newTarget = 0; // 0 = Firmware, 1 = Host
        ImGui::Text("LED format:");
        ImGui::RadioButton("Legacy (4x4, 16 LEDs)", &newMode, 0);
        ImGui::RadioButton("Modern (4x4 + 3x3, 25 LEDs)", &newMode, 1);
        ImGui::Separator();
        ImGui::Text("Canvas:");
        ImGui::RadioButton("Full pad (9 panels)", &newExtent, 0);
        ImGui::RadioButton("Single panel (per-panel judgement GIF)", &newExtent, 1);
        ImGui::Separator();
        // Firmware upload is only possible for a Modern full pad; everything else is host-only.
        bool firmwareAllowed = (newMode == 1 && newExtent == 0);
        if (!firmwareAllowed) newTarget = 1;
        ImGui::Text("Target:");
        ImGui::BeginDisabled(!firmwareAllowed);
        ImGui::RadioButton("Firmware (EEPROM upload; 32-frame, 15-color caps)", &newTarget, 0);
        ImGui::EndDisabled();
        ImGui::RadioButton("Host (deadsync playback; uncapped, no upload)", &newTarget, 1);
        if (!firmwareAllowed)
            ImGui::TextDisabled("Only a Modern full pad can upload to firmware.");
        ImGui::Separator();
        if (ImGui::Button("Create") || ModalDefaultConfirm())
        {
            CanvasMode newCanvasMode = newMode == 1 ? CanvasMode::Modern : CanvasMode::Legacy;
            CanvasExtent newCanvasExtent = newExtent == 1 ? CanvasExtent::SinglePanel : CanvasExtent::FullPad;
            CanvasTarget newCanvasTarget = newTarget == 1 ? CanvasTarget::Host : CanvasTarget::Firmware;
            if (newCanvasMode != app.canvas.mode || newCanvasExtent != app.canvas.extent)
            {
                app.panelClipboard.clear();
                app.panelClipboardValid = false;
            }
            app.canvas.Init(newCanvasMode, newCanvasExtent, newCanvasTarget);
            app.currentFilePath.clear();
            app.dirty = false; app.undo.MarkSaved();
            app.ClearFrameSelection();
            app.undo.Clear();
            app.undo.SaveState(app.canvas, "Initial");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ModalCancelPressed())
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // --- Composite GIF Loading ---
    if (g_compositeRelRequested)
    {
        g_compositeRelRequested = false;
        string err;
        if (ImportGif(g_compositeRelPath, app.compositeReleased, err))
            app.compositeReleasedLoaded = true;
    }
    if (g_compositePrsRequested)
    {
        g_compositePrsRequested = false;
        string err;
        if (ImportGif(g_compositePrsPath, app.compositePressed, err))
            app.compositePressedLoaded = true;
    }

    // --- Keyboard Shortcuts (only when not typing and no popup open) ---
    if (!io.WantTextInput && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.draw)) app.tool = Tool_Draw;
        if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.erase)) app.tool = Tool_Erase;
        if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.fill)) app.tool = Tool_Fill;
        if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.replace)) app.tool = Tool_Replace;
        if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.pick)) app.tool = Tool_Pick;

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) { app.undo.Undo(app.canvas); app.ClearFrameSelection(); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) { app.undo.Redo(app.canvas); app.ClearFrameSelection(); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_E)) { app.hsvDialogPanelMask = 0x1FF; app.showHsvDialog = true; }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            app.frameClipboard.clear();
            for (int fi : app.SelectionOrCurrent())
                app.frameClipboard.push_back(app.canvas.frames[fi]);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && app.CanPasteFrames())
        {
            int first = app.canvas.currentFrame + 1;
            int count = app.canvas.InsertFrames(first, app.frameClipboard);
            app.ClearFrameSelection();
            for (int i = first; i < first + count && count > 1; i++)
                app.selectedFrames.push_back(i);
            app.selectAnchor = first;
            app.dirty = true; app.colorCountsDirty = true;
            app.undo.SaveState(app.canvas, count > 1 ? "Paste Frames" : "Paste Frame");
        }

        if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            bool overLimit = false;
            if (app.canvas.target == CanvasTarget::Firmware)
                for (int p = 0; p < app.canvas.PanelCount(); p++)
                    if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                        overLimit = true;
            if (overLimit)
                app.showExportWarning = true;
            else if (app.currentFilePath.empty())
            {
                SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
            }
            else
            {
                string err;
                if (ExportGif(app.canvas, app.currentFilePath, err))
                {
                    app.dirty = false;
                    app.undo.MarkSaved();
                }
            }
        }
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            bool overLimit = false;
            if (app.canvas.target == CanvasTarget::Firmware)
                for (int p = 0; p < app.canvas.PanelCount(); p++)
                    if (app.canvas.ColorCountForPanelAllFrames(p) > 15)
                        overLimit = true;
            if (overLimit)
                app.showExportWarning = true;
            else
            {
                SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
            }
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
        {
            if (app.prefs.promptOnUnsaved && app.dirty)
                { g_importPath.clear(); app.pendingAction = Pending_Import; app.showUnsavedDialog = true; }
            else
            {
                SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                SDL_ShowOpenFileDialog(ImportDialogCallback, nullptr, window, filters, 1, nullptr, false);
            }
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
        {
            if (app.prefs.promptOnUnsaved && app.dirty)
                { app.pendingAction = Pending_New; app.showUnsavedDialog = true; }
            else
                app.showNewDialog = true;
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            if (app.prefs.promptOnUnsaved && app.dirty)
                { app.pendingAction = Pending_Quit; app.showUnsavedDialog = true; }
            else
                app.running = false;
        }
    }

    // --- About Window ---
    if (showAbout)
        ImGui::OpenPopup("About##modal");
    if (ImGui::BeginPopupModal("About##modal", &showAbout, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("StepManiaX GIF Maker");
        ImGui::Text("Version %s", SMX_GIF_MAKER_VERSION);
        ImGui::Separator();
        ImGui::Text("Created by Fernando Chorney");
        ImGui::Spacing();
        ImGui::Text("Acknowledgments:");
        ImGui::BulletText("StepRevolution - creators of StepManiaX");
        ImGui::BulletText("stepmaniax-sdk - original SDK by Step Revolution");
        ImGui::BulletText("stepmaniax-sdk-mp - cross-platform SDK rewrite");
        ImGui::Spacing();
        ImGui::Text("Built with:");
        ImGui::BulletText("Dear ImGui - immediate-mode GUI framework");
        ImGui::BulletText("SDL3 - cross-platform windowing");
        ImGui::BulletText("hidapi - USB HID communication");
        ImGui::BulletText("nlohmann/json - JSON for Modern C++");
        ImGui::BulletText("gif_load - single-header GIF decoder");
        ImGui::Spacing();
        ImGui::Text("License: MIT");
        ImGui::Spacing();
        if (ImGui::TextLink("github.com/fchorney/stepmaniax-gif-maker"))
            SDL_OpenURL("https://github.com/fchorney/stepmaniax-gif-maker");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0)) || ModalDefaultConfirm() || ModalCancelPressed())
        {
            showAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
