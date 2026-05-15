/// LED preview panel and history panel rendering.
#include "ui_preview.h"

void RenderPreview(AppState &app)
{
    ImGuiIO &io = ImGui::GetIO();

    // --- Preview ---
    ImGui::Begin("Preview");
    {
        static bool hwColors = false;
        static float previewZoom = 6.0f;
        static bool previewPlaying = false;
        static double previewLastTime = 0;
        static int previewFrame = 0;

        ImGui::Checkbox("Hardware colors (66%%)", &hwColors);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Preview with the 66%% color scaling applied by hardware"); ImGui::EndTooltip(); }
        ImGui::SliderFloat("Zoom##prev", &previewZoom, 3.0f, 15.0f, "%.0f");
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Preview zoom level (Ctrl+Scroll)"); ImGui::EndTooltip(); }

        if (!previewPlaying)
        {
            if (ImGui::Button("Play##prev")) { previewPlaying = true; previewLastTime = ImGui::GetTime(); previewFrame = app.canvas.currentFrame; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Play animation in preview"); ImGui::EndTooltip(); }
        }
        else
        {
            if (ImGui::Button("Stop##prev")) previewPlaying = false;
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Stop preview playback"); ImGui::EndTooltip(); }
        }
        static bool previewSync = true;
        ImGui::SameLine();
        ImGui::Checkbox("Sync", &previewSync);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Sync preview to editor frame selection"); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (previewPlaying)
            ImGui::Text("Frame %d/%d (playing)", (previewSync ? app.canvas.currentFrame : previewFrame) + 1, (int)app.canvas.frames.size());
        else
            ImGui::Text("Frame %d/%d", (previewSync ? app.canvas.currentFrame : previewFrame) + 1, (int)app.canvas.frames.size());

        // Advance preview animation independently
        int totalFrames = (int)app.canvas.frames.size();
        if (previewPlaying && totalFrames > 1)
        {
            double now = ImGui::GetTime();
            if (previewFrame >= totalFrames) previewFrame = 0;
            float dur = app.canvas.frames[previewFrame].duration;
            if (now - previewLastTime >= dur)
            {
                previewLastTime = now;
                previewFrame++;
                if (previewFrame >= totalFrames)
                    previewFrame = app.canvas.loopFrame;
            }
        }

        int displayFrame = previewSync ? app.canvas.currentFrame : previewFrame;
        if (displayFrame >= totalFrames) displayFrame = 0;

        ImGui::Separator();

        const auto &frame = app.canvas.frames[displayFrame];
        int w = app.canvas.Width();
        ImDrawList *draw = ImGui::GetWindowDrawList();
        float ledSpacing = previewZoom;
        float panelGap = previewZoom * 0.8f;

        int outerGrid = 4;
        float outerSpan = (outerGrid - 1) * ledSpacing;
        float panelSize = outerSpan + ledSpacing * 2;
        float totalSize = panelSize * 3 + panelGap * 2;

        // Center in available space
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float offsetX = (avail.x > totalSize) ? (avail.x - totalSize) * 0.5f : 0;
        float offsetY = (avail.y > totalSize + 30) ? (avail.y - totalSize - 30) * 0.5f : 0;
        if (offsetX > 0 || offsetY > 0)
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));

        ImVec2 origin = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(origin, ImVec2(origin.x + totalSize, origin.y + totalSize), IM_COL32(20, 20, 20, 255));

        float ledRadius = previewZoom * 0.35f;

        for (int panel = 0; panel < 9; panel++)
        {
            int pcol = panel % 3;
            int prow = panel / 3;
            float px = origin.x + pcol * (panelSize + panelGap);
            float py = origin.y + prow * (panelSize + panelGap);

            draw->AddRect(ImVec2(px, py), ImVec2(px + panelSize, py + panelSize),
                IM_COL32(80, 80, 80, 255), 2.0f);
            draw->AddRectFilled(ImVec2(px + 1, py + 1), ImVec2(px + panelSize - 1, py + panelSize - 1),
                IM_COL32(10, 10, 10, 255));

            int col = panel % 3;
            int row = panel / 3;
            float ledPadX = (panelSize - outerSpan) * 0.5f;
            float ledPadY = ledPadX;

            for (int dy = 0; dy < 4; dy++)
            {
                for (int dx = 0; dx < 4; dx++)
                {
                    int pixX, pixY;
                    if (app.canvas.mode == CanvasMode::Modern)
                    {
                        pixX = col * 8 + dx * 2;
                        pixY = row * 8 + dy * 2;
                    }
                    else
                    {
                        pixX = col * 5 + dx;
                        pixY = row * 5 + dy;
                    }

                    Color c = frame.GetPixel(pixX, pixY, w);
                    float cx = px + ledPadX + dx * ledSpacing;
                    float cy = py + ledPadY + dy * ledSpacing;

                    ImU32 color;
                    if (c.IsBlack())
                        color = IM_COL32(30, 30, 30, 255);
                    else
                    {
                        uint8_t r = hwColors ? (uint8_t)(c.r * 0.6666f) : c.r;
                        uint8_t g = hwColors ? (uint8_t)(c.g * 0.6666f) : c.g;
                        uint8_t b = hwColors ? (uint8_t)(c.b * 0.6666f) : c.b;
                        color = IM_COL32(r, g, b, 255);
                    }
                    draw->AddCircleFilled(ImVec2(cx, cy), ledRadius, color);
                }
            }

            // Inner 3x3 grid (modern mode only)
            if (app.canvas.mode == CanvasMode::Modern)
            {
                float innerOffset = ledSpacing * 0.5f;
                for (int dy = 0; dy < 3; dy++)
                {
                    for (int dx = 0; dx < 3; dx++)
                    {
                        int pixX = col * 8 + dx * 2 + 1;
                        int pixY = row * 8 + dy * 2 + 1;

                        Color c = frame.GetPixel(pixX, pixY, w);
                        float cx = px + ledPadX + innerOffset + dx * ledSpacing;
                        float cy = py + ledPadY + innerOffset + dy * ledSpacing;

                        ImU32 color;
                        if (c.IsBlack())
                            color = IM_COL32(30, 30, 30, 200);
                        else
                        {
                            uint8_t r = hwColors ? (uint8_t)(c.r * 0.6666f) : c.r;
                            uint8_t g = hwColors ? (uint8_t)(c.g * 0.6666f) : c.g;
                            uint8_t b = hwColors ? (uint8_t)(c.b * 0.6666f) : c.b;
                            color = IM_COL32(r, g, b, 255);
                        }
                        draw->AddCircleFilled(ImVec2(cx, cy), ledRadius, color);
                    }
                }
            }
        }

        ImGui::Dummy(ImVec2(totalSize, totalSize));

        // Ctrl+scroll to zoom preview
        if (ImGui::IsItemHovered() && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && io.MouseWheel != 0)
        {
            previewZoom += io.MouseWheel * 1.0f;
            if (previewZoom < 3.0f) previewZoom = 3.0f;
            if (previewZoom > 15.0f) previewZoom = 15.0f;
        }
    }
    ImGui::End();

    // --- History Panel ---
    ImGui::Begin("History");
    {
        ImGui::Text("Undo History (%d/%d, max %d)", app.undo.GetPos(), app.undo.GetCount() - 1, app.undo.GetMaxHistory());
        ImGui::Separator();
        ImGui::BeginChild("##history_list", ImVec2(0, 0), false);
        for (int i = app.undo.GetCount() - 1; i >= 0; i--)
        {
            bool isCurrent = (i == app.undo.GetPos());
            const char *lbl = app.undo.GetLabel(i);
            char buf[64];
            snprintf(buf, sizeof(buf), "%s %s###h%d", isCurrent ? ">" : " ", lbl[0] ? lbl : "?", i);

            if (isCurrent)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));

            if (ImGui::Selectable(buf, isCurrent))
                app.undo.GoTo(i, app.canvas);

            if (isCurrent)
                ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
