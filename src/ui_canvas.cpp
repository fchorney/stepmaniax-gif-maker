/// Canvas window rendering, drawing tools, and mouse interaction.
#include "ui_canvas.h"
#include <cstdio>

using namespace std;

// Fill all LEDs in a panel with the given color
static void FillPanel(CanvasFrame &frame, int w, int h, int x, int y, Color replacement, const Canvas &canvas)
{
    int panel = canvas.PanelAt(x, y);
    if (panel < 0) return;

    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++)
            if (canvas.PanelAt(px, py) == panel && canvas.IsLedPosition(px, py))
                frame.SetPixel(px, py, w, replacement);
}

// Replace all LEDs in a panel that match the target color with the replacement color
static void ReplaceColor(CanvasFrame &frame, int w, int h, int x, int y, Color target, Color replacement, const Canvas &canvas)
{
    if (target == replacement) return;
    int panel = canvas.PanelAt(x, y);
    if (panel < 0) return;

    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++)
            if (canvas.PanelAt(px, py) == panel && canvas.IsLedPosition(px, py))
                if (frame.GetPixel(px, py, w) == target)
                    frame.SetPixel(px, py, w, replacement);
}

void RenderCanvas(AppState &app)
{
    ImGuiIO &io = ImGui::GetIO();

    // --- Tool Panel ---
    ImGui::Begin("Tools");
    ImGui::Text("Drawing Tools");
    ImGui::Separator();
    ImGui::RadioButton("Draw", &app.tool, Tool_Draw);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Draw pixels with the selected color (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.draw)); ImGui::EndTooltip(); }
    ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.draw));
    ImGui::RadioButton("Erase", &app.tool, Tool_Erase);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Erase pixels (set to black/off) (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.erase)); ImGui::EndTooltip(); }
    ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.erase));
    ImGui::RadioButton("Fill", &app.tool, Tool_Fill);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Fill all LEDs in a panel with the selected color (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.fill)); ImGui::EndTooltip(); }
    ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.fill));
    ImGui::RadioButton("Replace", &app.tool, Tool_Replace);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Replace all matching-color LEDs in a panel (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.replace)); ImGui::EndTooltip(); }
    ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.replace));
    ImGui::RadioButton("Pick Color", &app.tool, Tool_Pick);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Pick a color from the canvas (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.pick)); ImGui::EndTooltip(); }
    ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.pick));

    ImGui::Separator();
    const char *modeLabel = (app.canvas.mode == CanvasMode::Modern) ? "Modern (23x24)" : "Legacy (14x15)";
    ImGui::Text("Mode: %s", modeLabel);
    ImGui::Separator();
    ImGui::SliderFloat("Zoom", &app.cellSize, 8.0f, 40.0f, "%.0f px");
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Canvas zoom level (Ctrl+Scroll)"); ImGui::EndTooltip(); }
    ImGui::Separator();
    ImGui::Checkbox("Onion Skin", &app.onionSkin);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Show previous/next frame as faded overlay"); ImGui::EndTooltip(); }
    if (app.onionSkin)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Prev", &app.onionPrev);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Show previous frame ghost"); ImGui::EndTooltip(); }
        ImGui::SameLine();
        ImGui::Checkbox("Next", &app.onionNext);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Show next frame ghost"); ImGui::EndTooltip(); }
    }
    ImGui::End();

    // --- Color Palette ---
    ImGui::Begin("Palette");
    {
        Color drawColor = {
            (uint8_t)(app.currentColor.x * 255),
            (uint8_t)(app.currentColor.y * 255),
            (uint8_t)(app.currentColor.z * 255)
        };
        ImGui::ColorButton("Current", app.currentColor, 0, ImVec2(40, 40));
        ImGui::SameLine();
        ImGui::Text("R:%d G:%d B:%d", drawColor.r, drawColor.g, drawColor.b);
        ImGui::Separator();
        ImGui::ColorPicker3("##color", (float *)&app.currentColor,
            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::Separator();
        ImGui::Text("Panel Colors (all frames):");
        for (int p = 0; p < 9; p++)
        {
            int count = app.canvas.ColorCountForPanelAllFrames(p);
            if (count > 15)
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "  Panel %d: %d/15 (!)", p, count);
            else
                ImGui::Text("  Panel %d: %d/15", p, count);
        }
    }
    ImGui::End();

    // --- Canvas ---
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
    {
        const char *modeStr = (app.canvas.mode == CanvasMode::Modern) ? "Modern (23x24)" : "Legacy (14x15)";
        ImGui::Text("Mode: %s | Frame %d/%d", modeStr,
            app.canvas.currentFrame + 1, (int)app.canvas.frames.size());
        ImGui::Separator();

        ImDrawList *draw = ImGui::GetWindowDrawList();
        int gridW = app.canvas.Width();
        int gridH = app.canvas.Height();
        ImVec2 canvasSize(gridW * app.cellSize, gridH * app.cellSize);

        // Center the grid in available space
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float offsetX = (avail.x > canvasSize.x) ? (avail.x - canvasSize.x) * 0.5f : 0;
        float offsetY = (avail.y > canvasSize.y) ? (avail.y - canvasSize.y) * 0.5f : 0;

        if (offsetX > 0 || offsetY > 0)
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();

        // Invisible button for mouse interaction
        ImGui::InvisibleButton("##canvas", canvasSize);
        bool canvasHovered = ImGui::IsItemHovered();
        bool canvasActive = ImGui::IsItemActive();

        // Ctrl+scroll to zoom canvas
        if (canvasHovered && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && io.MouseWheel != 0)
        {
            app.cellSize += io.MouseWheel * 2.0f;
            if (app.cellSize < 8.0f) app.cellSize = 8.0f;
            if (app.cellSize > 40.0f) app.cellSize = 40.0f;
        }

        // Draw pixels
        const auto &frame = app.canvas.CurrentFrame();
        for (int y = 0; y < gridH; y++)
        {
            for (int x = 0; x < gridW; x++)
            {
                ImVec2 tl(canvasPos.x + x * app.cellSize, canvasPos.y + y * app.cellSize);
                ImVec2 br(tl.x + app.cellSize, tl.y + app.cellSize);

                Color c = frame.GetPixel(x, y, gridW);
                ImU32 fillColor;

                if (app.canvas.IsFlagRow(x, y))
                    fillColor = IM_COL32(20, 20, 40, 255);
                else if (app.canvas.IsGutter(x, y))
                    fillColor = IM_COL32(50, 50, 50, 255);
                else if (c.IsBlack())
                {
                    if (app.onionSkin && app.canvas.IsLedPosition(x, y))
                    {
                        Color prev = {}, next = {};
                        if (app.onionPrev && app.canvas.currentFrame > 0)
                            prev = app.canvas.frames[app.canvas.currentFrame - 1].GetPixel(x, y, gridW);
                        if (app.onionNext && app.canvas.currentFrame < (int)app.canvas.frames.size() - 1)
                            next = app.canvas.frames[app.canvas.currentFrame + 1].GetPixel(x, y, gridW);

                        if (!prev.IsBlack() && !next.IsBlack())
                            fillColor = IM_COL32((prev.r + next.r) / 4, (prev.g + next.g) / 4, (prev.b + next.b) / 4, 255);
                        else if (!prev.IsBlack())
                            fillColor = IM_COL32(prev.r / 2, prev.g / 2, prev.b / 2, 255);
                        else if (!next.IsBlack())
                            fillColor = IM_COL32(next.r / 2, next.g / 2, next.b / 2, 255);
                        else
                            fillColor = IM_COL32(15, 15, 15, 255);
                    }
                    else
                        fillColor = IM_COL32(15, 15, 15, 255);
                }
                else
                    fillColor = IM_COL32(c.r, c.g, c.b, 255);

                draw->AddRectFilled(tl, br, fillColor);

                // Grid lines
                ImU32 gridColor;
                if (app.canvas.IsGutter(x, y))
                    gridColor = IM_COL32(100, 100, 100, 255);
                else
                    gridColor = IM_COL32(40, 40, 40, 255);
                draw->AddRect(tl, br, gridColor);

                // LED position indicator (small dot)
                if (app.canvas.IsLedPosition(x, y) && app.cellSize >= 12.0f)
                {
                    float cx = tl.x + app.cellSize * 0.5f;
                    float cy = tl.y + app.cellSize * 0.5f;
                    float r = app.cellSize * 0.1f;
                    ImU32 dotColor = c.IsBlack() ? IM_COL32(80, 80, 80, 150) : IM_COL32(255, 255, 255, 80);
                    draw->AddCircleFilled(ImVec2(cx, cy), r, dotColor);
                }

                // Onion skin: corner triangles for prev/next frame
                if (app.onionSkin && app.canvas.IsLedPosition(x, y) && app.cellSize >= 10.0f)
                {
                    float ts = app.cellSize * 0.35f;
                    bool hasPrev = false, hasNext = false;
                    if (app.onionPrev && app.canvas.currentFrame > 0)
                        hasPrev = !app.canvas.frames[app.canvas.currentFrame - 1].GetPixel(x, y, gridW).IsBlack();
                    if (app.onionNext && app.canvas.currentFrame < (int)app.canvas.frames.size() - 1)
                        hasNext = !app.canvas.frames[app.canvas.currentFrame + 1].GetPixel(x, y, gridW).IsBlack();

                    if (hasPrev)
                        draw->AddTriangleFilled(
                            tl, ImVec2(tl.x + ts, tl.y), ImVec2(tl.x, tl.y + ts),
                            IM_COL32(0, 255, 255, 180));
                    if (hasNext)
                        draw->AddTriangleFilled(
                            br, ImVec2(br.x - ts, br.y), ImVec2(br.x, br.y - ts),
                            IM_COL32(255, 0, 255, 180));
                }
            }
        }

        // Mouse interaction
        if (canvasHovered || canvasActive)
        {
            ImVec2 mouse = ImGui::GetMousePos();
            int mx = (int)((mouse.x - canvasPos.x) / app.cellSize);
            int my = (int)((mouse.y - canvasPos.y) / app.cellSize);

            if (mx >= 0 && mx < gridW && my >= 0 && my < gridH &&
                !app.canvas.IsGutter(mx, my) && !app.canvas.IsFlagRow(mx, my) &&
                app.canvas.IsLedPosition(mx, my))
            {
                // Hover highlight
                ImVec2 htl(canvasPos.x + mx * app.cellSize, canvasPos.y + my * app.cellSize);
                ImVec2 hbr(htl.x + app.cellSize, htl.y + app.cellSize);
                draw->AddRect(htl, hbr, IM_COL32(255, 255, 0, 180), 0, 0, 2.0f);

                // Tool actions on click/drag
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && canvasActive)
                {
                    if (!app.strokeActive)
                        app.strokeActive = true;

                    auto &editFrame = app.canvas.CurrentFrame();
                    Color drawColor = {
                        (uint8_t)(app.currentColor.x * 255),
                        (uint8_t)(app.currentColor.y * 255),
                        (uint8_t)(app.currentColor.z * 255)
                    };

                    switch (app.tool)
                    {
                    case Tool_Draw:
                        editFrame.SetPixel(mx, my, gridW, drawColor);
                        break;
                    case Tool_Erase:
                        editFrame.SetPixel(mx, my, gridW, Color{0, 0, 0});
                        break;
                    case Tool_Fill:
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            FillPanel(editFrame, gridW, gridH, mx, my, drawColor, app.canvas);
                        break;
                    case Tool_Replace:
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            Color target = editFrame.GetPixel(mx, my, gridW);
                            ReplaceColor(editFrame, gridW, gridH, mx, my, target, drawColor, app.canvas);
                        }
                        break;
                    case Tool_Pick:
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            Color picked = editFrame.GetPixel(mx, my, gridW);
                            app.currentColor.x = picked.r / 255.0f;
                            app.currentColor.y = picked.g / 255.0f;
                            app.currentColor.z = picked.b / 255.0f;
                        }
                        break;
                    }
                }
            }

            // Right-click context menu for clearing panels
            if (mx >= 0 && mx < gridW && my >= 0 && my < gridH)
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    app.rightClickPanel = app.canvas.PanelAt(mx, my);
                    if (app.rightClickPanel < 0)
                    {
                        int col = (app.canvas.mode == CanvasMode::Modern) ?
                            (mx < 8 ? 0 : mx < 16 ? 1 : 2) :
                            (mx < 5 ? 0 : mx < 10 ? 1 : 2);
                        int row = (app.canvas.mode == CanvasMode::Modern) ?
                            (my < 8 ? 0 : my < 16 ? 1 : 2) :
                            (my < 5 ? 0 : my < 10 ? 1 : 2);
                        app.rightClickPanel = row * 3 + col;
                    }
                    ImGui::OpenPopup("##canvas_ctx");
                }
            }
        }

        // Save undo state when stroke ends
        if (app.strokeActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            app.strokeActive = false;
            const char *toolLabel = "Draw";
            if (app.tool == Tool_Erase) toolLabel = "Erase";
            else if (app.tool == Tool_Fill) toolLabel = "Fill";
            else if (app.tool == Tool_Replace) toolLabel = "Replace";
            app.dirty = true; app.undo.SaveState(app.canvas, toolLabel);
        }

        if (ImGui::BeginPopup("##canvas_ctx"))
        {
            if (app.rightClickPanel >= 0)
                ImGui::Text("Panel %d", app.rightClickPanel);
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Panel") && app.rightClickPanel >= 0)
            {
                app.panelClipboard.clear();
                int w = app.canvas.Width(), h = app.canvas.Height();
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++)
                        if (app.canvas.PanelAt(x, y) == app.rightClickPanel && app.canvas.IsLedPosition(x, y))
                            app.panelClipboard.push_back(app.canvas.CurrentFrame().GetPixel(x, y, w));
                app.panelClipboardValid = true;
            }
            if (ImGui::MenuItem("Paste Panel", nullptr, false, app.panelClipboardValid) && app.rightClickPanel >= 0)
            {
                int w = app.canvas.Width(), h = app.canvas.Height();
                int idx = 0;
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++)
                        if (app.canvas.PanelAt(x, y) == app.rightClickPanel && app.canvas.IsLedPosition(x, y))
                        {
                            if (idx < (int)app.panelClipboard.size())
                                app.canvas.CurrentFrame().SetPixel(x, y, w, app.panelClipboard[idx]);
                            idx++;
                        }
                app.dirty = true; app.undo.SaveState(app.canvas, "Paste Panel");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear This Panel") && app.rightClickPanel >= 0)
            {
                app.canvas.ClearPanel(app.rightClickPanel);
                app.dirty = true; app.undo.SaveState(app.canvas, "Clear Panel");
            }
            if (ImGui::MenuItem("Quantize This Panel") && app.rightClickPanel >= 0)
            {
                app.canvas.QuantizePanel(app.rightClickPanel);
                app.dirty = true; app.undo.SaveState(app.canvas, "Quantize Panel");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear All Panels"))
            {
                app.canvas.ClearAll();
                app.dirty = true; app.undo.SaveState(app.canvas, "Clear All");
            }
            if (ImGui::MenuItem("Quantize All Panels"))
            {
                for (int p = 0; p < 9; p++) app.canvas.QuantizePanel(p);
                app.dirty = true; app.undo.SaveState(app.canvas, "Quantize All");
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
