/// Menu bar rendering and all popup/modal dialogs.
#include "ui_menus.h"
#include "gif_export.h"
#include "gif_import.h"
#include <SMX.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>

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

static void UploadProgressCb(int progress, void *pUser)
{
    (void)pUser;
    g_uploadProgress.store(progress);
}

void RenderMenus(AppState &app, SDL_Window *window)
{
    ImGuiIO &io = ImGui::GetIO();

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
                for (int p = 0; p < 9; p++)
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
                for (int p = 0; p < 9; p++)
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
            if (ImGui::MenuItem("Undo", SHORTCUT_MOD "+Z", false, app.undo.CanUndo())) { app.undo.Undo(app.canvas); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
            if (ImGui::MenuItem("Redo", SHORTCUT_MOD "+Y", false, app.undo.CanRedo())) { app.undo.Redo(app.canvas); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Frame", SHORTCUT_MOD "+C"))
            {
                app.frameClipboard = app.canvas.CurrentFrame();
                app.frameClipboardValid = true;
            }
            if (ImGui::MenuItem("Paste Frame", SHORTCUT_MOD "+V", false, app.frameClipboardValid && (int)app.canvas.frames.size() < Canvas::MaxFrames))
            {
                app.canvas.frames.insert(app.canvas.frames.begin() + app.canvas.currentFrame + 1, app.frameClipboard);
                app.canvas.currentFrame++;
                app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Paste Frame");
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Clear Panel"))
            {
                for (int p = 0; p < 9; p++)
                {
                    char label[16];
                    snprintf(label, sizeof(label), "Panel %d", p);
                    if (ImGui::MenuItem(label)) { app.canvas.ClearPanel(p); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Clear Panel"); }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Clear All Panels")) { app.canvas.ClearAll(); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Clear All"); }
            if (ImGui::BeginMenu("Quantize Panel"))
            {
                for (int p = 0; p < 9; p++)
                {
                    char label[32];
                    snprintf(label, sizeof(label), "Panel %d", p);
                    if (ImGui::MenuItem(label)) { app.canvas.QuantizePanel(p); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Quantize Panel"); }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Quantize All Panels"))
            {
                for (int p = 0; p < 9; p++) app.canvas.QuantizePanel(p);
                app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Quantize All");
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
            if (ImGui::BeginMenu("Upload to Firmware", info0.m_bConnected || info1.m_bConnected))
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
            if (ImGui::MenuItem("Composite Preview...", nullptr, false, info0.m_bConnected || info1.m_bConnected))
                app.showCompositeDialog = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Re-enable Auto Lights", nullptr, false, info0.m_bConnected || info1.m_bConnected))
                SMX_ReenableAutoLights();
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

        ImGui::Separator();
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            rebindTarget = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
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
        if (ImGui::Button("Cancel"))
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
        if (exportSuccess)
            ImGui::Text("GIF saved successfully!");
        else
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Save failed: %s", exportError.c_str());
        if (ImGui::Button("OK"))
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
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
        if (ImGui::Button("Cancel"))
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
            app.undo.Clear();
            app.undo.SaveState(app.canvas, "Open");
            if (pixelsModified)
            {
                app.dirty = true;
                app.colorCountsDirty = true;
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Import failed: %s", importError.c_str());
        if (ImGui::Button("OK"))
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);

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
        if (ImGui::Button("Close", ImVec2(80, 0)))
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
        if (!app.uploadError.empty())
        {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Upload failed: %s", app.uploadError.c_str());
            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();
        }
        else if (app.uploadProgress >= 100)
        {
            static bool uploadFocusSet = false;
            ImGui::Text("Upload complete!");
            if (ImGui::Button("OK"))
            {
                uploadFocusSet = false;
                ImGui::CloseCurrentPopup();
            }
            if (!uploadFocusSet)
            {
                ImGui::SetKeyboardFocusHere(-1);
                ImGui::SetNavCursorVisible(true);
                uploadFocusSet = true;
            }
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
        if (ImGui::IsWindowAppearing()) ImGui::SetNavCursorVisible(true);
        ImGui::Text("Create a new animation. This will discard current work.");
        ImGui::Separator();
        static int newMode = 1;
        ImGui::RadioButton("Legacy (14x15) - host playback only", &newMode, 0);
        ImGui::RadioButton("Modern (23x24) - host playback + firmware upload", &newMode, 1);
        ImGui::Separator();
        if (ImGui::Button("Create"))
        {
            CanvasMode newCanvasMode = newMode == 1 ? CanvasMode::Modern : CanvasMode::Legacy;
            if (newCanvasMode != app.canvas.mode)
            {
                app.panelClipboard.clear();
                app.panelClipboardValid = false;
            }
            app.canvas.Init(newCanvasMode);
            app.currentFilePath.clear();
            app.dirty = false; app.undo.MarkSaved();
            app.undo.Clear();
            app.undo.SaveState(app.canvas, "Initial");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
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

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) { app.undo.Undo(app.canvas); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) { app.undo.Redo(app.canvas); app.dirty = app.undo.HasUnsavedChanges(); app.colorCountsDirty = true; }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            app.frameClipboard = app.canvas.CurrentFrame();
            app.frameClipboardValid = true;
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && app.frameClipboardValid && (int)app.canvas.frames.size() < Canvas::MaxFrames)
        {
            app.canvas.frames.insert(app.canvas.frames.begin() + app.canvas.currentFrame + 1, app.frameClipboard);
            app.canvas.currentFrame++;
            app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Paste Frame");
        }

        if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            bool overLimit = false;
            for (int p = 0; p < 9; p++)
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
            for (int p = 0; p < 9; p++)
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
}
