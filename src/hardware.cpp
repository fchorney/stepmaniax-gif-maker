/// Live hardware preview and composite preview via SMX_SetLights2.
#include "hardware.h"
#include <SMX.h>
#include <cstring>

void UpdateHardware(AppState &app)
{
    // --- Composite Hardware Preview ---
    if (app.compositePreview)
    {
        double now = ImGui::GetTime();
        if (now - app.compositeLastSend >= 1.0 / 30.0)
        {
            app.compositeLastSend = now;

            // Advance released animation
            int relTotal = (int)app.compositeReleased.frames.size();
            if (relTotal > 1)
            {
                app.compositeFrameTime += 1.0 / 30.0;
                float dur = app.compositeReleased.frames[app.compositeRelFrame].duration;
                if (app.compositeFrameTime >= dur)
                {
                    app.compositeFrameTime -= dur;
                    app.compositeRelFrame++;
                    if (app.compositeRelFrame >= relTotal)
                        app.compositeRelFrame = app.compositeReleased.loopFrame;
                }
            }

            // Get input state
            uint16_t inputState0 = SMX_GetInputState(0);
            uint16_t inputState1 = SMX_GetInputState(1);

            // Build light buffer
            char lightData[1350] = {};
            int w = app.compositeReleased.Width();

            for (int padIdx = 0; padIdx < 2; padIdx++)
            {
                uint16_t inputState = (padIdx == 0) ? inputState0 : inputState1;
                int padOffset = padIdx * 675;

                for (int panel = 0; panel < 9; panel++)
                {
                    int col = panel % 3;
                    int row = panel / 3;
                    bool pressed = (inputState & (1 << panel)) != 0;

                    const CanvasFrame *relFrame = &app.compositeReleased.frames[app.compositeRelFrame];
                    const CanvasFrame *prsFrame = nullptr;
                    if (pressed && app.compositePressedLoaded && !app.compositePressed.frames.empty())
                        prsFrame = &app.compositePressed.frames[app.compositePrsFrame % (int)app.compositePressed.frames.size()];

                    int ledIdx = 0;
                    // Outer 4x4
                    for (int dy = 0; dy < 4; dy++)
                        for (int dx = 0; dx < 4; dx++)
                        {
                            int px = col * 8 + dx * 2;
                            int py = row * 8 + dy * 2;
                            Color c = relFrame->GetPixel(px, py, w);
                            if (prsFrame)
                            {
                                Color pc = prsFrame->GetPixel(px, py, w);
                                if (!pc.IsBlack() || app.compositeFillBlack) c = pc.IsBlack() ? Color{1,1,1} : pc;
                            }
                            int offset = padOffset + panel * 75 + ledIdx * 3;
                            lightData[offset + 0] = c.r;
                            lightData[offset + 1] = c.g;
                            lightData[offset + 2] = c.b;
                            ledIdx++;
                        }
                    // Inner 3x3
                    for (int dy = 0; dy < 3; dy++)
                        for (int dx = 0; dx < 3; dx++)
                        {
                            int px = col * 8 + dx * 2 + 1;
                            int py = row * 8 + dy * 2 + 1;
                            Color c = relFrame->GetPixel(px, py, w);
                            if (prsFrame)
                            {
                                Color pc = prsFrame->GetPixel(px, py, w);
                                if (!pc.IsBlack() || app.compositeFillBlack) c = pc.IsBlack() ? Color{1,1,1} : pc;
                            }
                            int offset = padOffset + panel * 75 + ledIdx * 3;
                            lightData[offset + 0] = c.r;
                            lightData[offset + 1] = c.g;
                            lightData[offset + 2] = c.b;
                            ledIdx++;
                        }
                }
            }

            SMX_SetLights2(lightData, 1350);

            // Advance pressed animation with proper timing
            if (app.compositePressedLoaded && !app.compositePressed.frames.empty())
            {
                app.compositePrsFrameTime += 1.0 / 30.0;
                float dur = app.compositePressed.frames[app.compositePrsFrame].duration;
                if (app.compositePrsFrameTime >= dur)
                {
                    app.compositePrsFrameTime -= dur;
                    app.compositePrsFrame++;
                    if (app.compositePrsFrame >= (int)app.compositePressed.frames.size())
                        app.compositePrsFrame = app.compositePressed.loopFrame;
                }
            }
        }
    }

    // --- Live Hardware Preview ---
    if (app.livePreview)
    {
        double now = ImGui::GetTime();
        if (now - app.livePreviewLastSend >= 1.0 / 30.0)
        {
            app.livePreviewLastSend = now;

            // Advance animation based on frame duration (play mode only)
            int totalFrames = (int)app.canvas.frames.size();
            if (!app.livePreviewSync && totalFrames > 1)
            {
                app.livePreviewFrameTime += 1.0 / 30.0;
                float dur = app.canvas.frames[app.livePreviewFrame].duration;
                if (app.livePreviewFrameTime >= dur)
                {
                    app.livePreviewFrameTime -= dur;
                    app.livePreviewFrame++;
                    if (app.livePreviewFrame >= totalFrames)
                        app.livePreviewFrame = app.canvas.loopFrame;
                }
            }

            // Determine which frame to display
            int displayFrame = app.livePreviewSync ? app.canvas.currentFrame : app.livePreviewFrame;
            if (displayFrame >= totalFrames) displayFrame = 0;

            // Build light buffer (1350 bytes: 2 pads x 9 panels x 25 LEDs x 3 RGB)
            char lightData[1350] = {};
            const auto &frame = app.canvas.frames[displayFrame];
            int w = app.canvas.Width();

            // Fill pad 0 (first 675 bytes)
            for (int panel = 0; panel < 9; panel++)
            {
                int col = panel % 3;
                int row = panel / 3;
                int ledIdx = 0;

                if (app.canvas.mode == CanvasMode::Modern)
                {
                    // Outer 4x4 at even coords
                    for (int dy = 0; dy < 4; dy++)
                        for (int dx = 0; dx < 4; dx++)
                        {
                            int px = col * 8 + dx * 2;
                            int py = row * 8 + dy * 2;
                            Color c = frame.GetPixel(px, py, w);
                            int offset = panel * 75 + ledIdx * 3;
                            lightData[offset + 0] = c.r;
                            lightData[offset + 1] = c.g;
                            lightData[offset + 2] = c.b;
                            ledIdx++;
                        }
                    // Inner 3x3 at odd coords
                    for (int dy = 0; dy < 3; dy++)
                        for (int dx = 0; dx < 3; dx++)
                        {
                            int px = col * 8 + dx * 2 + 1;
                            int py = row * 8 + dy * 2 + 1;
                            Color c = frame.GetPixel(px, py, w);
                            int offset = panel * 75 + ledIdx * 3;
                            lightData[offset + 0] = c.r;
                            lightData[offset + 1] = c.g;
                            lightData[offset + 2] = c.b;
                            ledIdx++;
                        }
                }
                else
                {
                    // Legacy: 4x4 at (col*5, row*5)
                    for (int dy = 0; dy < 4; dy++)
                        for (int dx = 0; dx < 4; dx++)
                        {
                            int px = col * 5 + dx;
                            int py = row * 5 + dy;
                            Color c = frame.GetPixel(px, py, w);
                            int offset = panel * 48 + ledIdx * 3;
                            lightData[offset + 0] = c.r;
                            lightData[offset + 1] = c.g;
                            lightData[offset + 2] = c.b;
                            ledIdx++;
                        }
                }
            }

            // Send to both pads (pad 1 gets same data as pad 0 for now)
            int size = (app.canvas.mode == CanvasMode::Modern) ? 1350 : 864;
            int padSize = size / 2;
            memcpy(lightData + padSize, lightData, padSize);
            SMX_SetLights2(lightData, size);
        }
    }
}
