/// Timeline controls, frame thumbnails, and playback.
#include "ui_timeline.h"
#include <cstdio>
#include <algorithm>

void RenderTimeline(AppState &app)
{
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Begin("Timeline");
    {
        static bool playing = false;
        static double lastFrameTime = 0;

        int totalFrames = (int)app.canvas.frames.size();

        // Playback logic
        if (playing && totalFrames > 1)
        {
            double now = ImGui::GetTime();
            float frameDur = app.canvas.CurrentFrame().duration;
            if ((now - lastFrameTime) * app.previewSpeed >= frameDur)
            {
                lastFrameTime = now;
                app.canvas.currentFrame++;
                if (app.canvas.currentFrame >= totalFrames)
                {
                    if (app.loopPlayback)
                        app.canvas.currentFrame = app.canvas.loopFrame;
                    else
                    {
                        app.canvas.currentFrame = totalFrames - 1; // hold on the last frame
                        playing = false;                            // one-shot: stop
                    }
                }
            }
        }

        // Press-and-hold simulation of deadsync segment playback: intro once,
        // then the loop region repeats while the Hold button is held down;
        // releasing snaps straight to the outro (no finishing the loop pass),
        // which plays out and parks on the last frame.
        static bool holdSim = false;
        static bool holdEngaged = false;
        if (app.canvas.loopEndFrame < 0)
            holdSim = false;
        if (holdSim && totalFrames > 1)
        {
            double now = ImGui::GetTime();
            float frameDur = app.canvas.CurrentFrame().duration;
            if ((now - lastFrameTime) * app.previewSpeed >= frameDur)
            {
                lastFrameTime = now;
                int loopEnd = std::min(app.canvas.loopEndFrame, totalFrames - 1);
                if (app.canvas.currentFrame == loopEnd && holdEngaged)
                    app.canvas.currentFrame = std::min(app.canvas.loopFrame, loopEnd);
                else if (app.canvas.currentFrame >= totalFrames - 1)
                    holdSim = false; // outro finished
                else
                    app.canvas.currentFrame++;
            }
        }

        // --- Playback ---
        ImGui::TextDisabled("Playback:");
        ImGui::SameLine();
        if (!playing)
        {
            if (ImGui::Button("Play"))
            {
                playing = true;
                holdSim = false;
                lastFrameTime = ImGui::GetTime();
                if (app.canvas.currentFrame >= totalFrames - 1)
                    app.canvas.currentFrame = 0; // replay from the start if parked at the end
            }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Play animation (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.playPause)); ImGui::EndTooltip(); }
        }
        else
        {
            if (ImGui::Button("Pause"))
                playing = false;
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Pause animation (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.playPause)); ImGui::EndTooltip(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {
            app.canvas.currentFrame = 0;
            playing = true;
            holdSim = false;
            lastFrameTime = ImGui::GetTime();
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Play from frame 0"); ImGui::EndTooltip(); }
        ImGui::SameLine();
        ImGui::Checkbox("Loop", &app.loopPlayback);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("When off, playback plays once and stops on the last frame");
        if (app.canvas.loopEndFrame >= 0)
        {
            ImGui::SameLine();
            ImGui::Button("Hold");
            if (ImGui::IsItemActivated())
            {
                playing = false;
                holdEngaged = true;
                if (!holdSim)
                {
                    holdSim = true;
                    app.canvas.currentFrame = 0;
                    lastFrameTime = ImGui::GetTime();
                }
                else if (app.canvas.currentFrame > app.canvas.loopEndFrame)
                {
                    // Re-press during the outro: back into the loop region,
                    // mirroring a freeze/roll re-engage in deadsync.
                    app.canvas.currentFrame = std::min(app.canvas.loopFrame, app.canvas.loopEndFrame);
                }
            }
            if (ImGui::IsItemDeactivated())
            {
                // Release: snap from the loop region straight to the outro,
                // mirroring deadsync (which does not finish the loop pass).
                holdEngaged = false;
                int loopEnd = std::min(app.canvas.loopEndFrame, totalFrames - 1);
                if (app.canvas.currentFrame >= app.canvas.loopFrame
                    && app.canvas.currentFrame <= loopEnd && loopEnd + 1 < totalFrames)
                {
                    app.canvas.currentFrame = loopEnd + 1;
                    lastFrameTime = ImGui::GetTime();
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Press and hold to simulate deadsync playback:\nintro, then the loop region repeats while held;\nreleasing snaps to the outro. Press again during\nthe outro to jump back into the loop.");
        }

        // --- Move ---
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        ImGui::TextDisabled("Move:");
        ImGui::SameLine();
        if (ImGui::Button("|<"))
            app.canvas.currentFrame = 0;
        if (ImGui::BeginItemTooltip()) { ImGui::Text("First frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.firstFrame)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button("<") && app.canvas.currentFrame > 0)
            app.canvas.currentFrame--;
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Previous frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.prevFrame)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button(">") && app.canvas.currentFrame < totalFrames - 1)
            app.canvas.currentFrame++;
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Next frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.nextFrame)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button(">|"))
            app.canvas.currentFrame = totalFrames - 1;
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Last frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.lastFrame)); ImGui::EndTooltip(); }

        // --- Frames ---
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        ImGui::TextDisabled("Frames:");
        ImGui::SameLine();
        if (ImGui::Button("-") && totalFrames > 1) { app.canvas.DeleteFrame(app.canvas.currentFrame); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Delete Frame"); }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Delete frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.deleteFrame)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        bool atFrameLimit = (totalFrames >= app.canvas.MaxFrames());
        if (atFrameLimit) ImGui::BeginDisabled();
        if (ImGui::Button("Dup")) { app.canvas.DuplicateFrame(app.canvas.currentFrame); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Duplicate Frame"); }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Duplicate frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.dupFrame)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button("+")) { app.canvas.AddFrame(); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Add Frame"); }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Add blank frame (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.addFrame)); ImGui::EndTooltip(); }
        if (atFrameLimit) ImGui::EndDisabled();

        // --- Shift ---
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        ImGui::TextDisabled("Shift:");
        ImGui::SameLine();
        if (ImGui::Button("<<") && app.canvas.currentFrame > 0)
        {
            std::swap(app.canvas.frames[app.canvas.currentFrame], app.canvas.frames[app.canvas.currentFrame - 1]);
            app.canvas.currentFrame--;
            app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Shift Left");
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Shift frame left (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.shiftLeft)); ImGui::EndTooltip(); }
        ImGui::SameLine();
        if (ImGui::Button(">>") && app.canvas.currentFrame < totalFrames - 1)
        {
            std::swap(app.canvas.frames[app.canvas.currentFrame], app.canvas.frames[app.canvas.currentFrame + 1]);
            app.canvas.currentFrame++;
            app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Shift Right");
        }
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Shift frame right (%s)", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.shiftRight)); ImGui::EndTooltip(); }

        // --- Timing ---
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        ImGui::TextDisabled("Timing:");
        ImGui::SameLine();
        static int editingDurFrame = -1;
        if (editingDurFrame < 0 || editingDurFrame >= (int)app.canvas.frames.size())
            editingDurFrame = app.canvas.currentFrame;
        float durMs = app.canvas.frames[editingDurFrame].duration * 1000.0f;
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputFloat("ms##dur", &durMs, 0, 0, "%.0f"))
        {
            if (durMs < 10) durMs = 10;
            if (durMs > 2550) durMs = 2550;
            app.canvas.frames[editingDurFrame].duration = durMs / 1000.0f;
        }
        if (!ImGui::IsItemActive())
            editingDurFrame = app.canvas.currentFrame;
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            app.dirty = true; app.colorCountsDirty = true;
            app.undo.SaveState(app.canvas, "Duration");
        }
        ImGui::SameLine();
        if (ImGui::Button("All##dur"))
        {
            float d = app.canvas.frames[editingDurFrame].duration;
            for (auto &f : app.canvas.frames) f.duration = d;
            app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Duration (all)");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Set all frames to this duration");

        // Arrow key navigation (when not typing and no popup open)
        bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        if (!io.WantTextInput && !playing && !popupOpen)
        {
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.prevFrame) && app.canvas.currentFrame > 0)
                app.canvas.currentFrame--;
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.nextFrame) && app.canvas.currentFrame < totalFrames - 1)
                app.canvas.currentFrame++;
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.firstFrame))
                app.canvas.currentFrame = 0;
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.lastFrame))
                app.canvas.currentFrame = totalFrames - 1;
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.addFrame) && totalFrames < app.canvas.MaxFrames())
                { app.canvas.AddFrame(); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Add Frame"); }
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.dupFrame) && totalFrames < app.canvas.MaxFrames())
                { app.canvas.DuplicateFrame(app.canvas.currentFrame); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Duplicate Frame"); }
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.deleteFrame) && totalFrames > 1)
                { app.canvas.DeleteFrame(app.canvas.currentFrame); app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Delete Frame"); }
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.shiftLeft) && app.canvas.currentFrame > 0)
            {
                std::swap(app.canvas.frames[app.canvas.currentFrame], app.canvas.frames[app.canvas.currentFrame - 1]);
                app.canvas.currentFrame--;
                app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Shift Left");
            }
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.shiftRight) && app.canvas.currentFrame < totalFrames - 1)
            {
                std::swap(app.canvas.frames[app.canvas.currentFrame], app.canvas.frames[app.canvas.currentFrame + 1]);
                app.canvas.currentFrame++;
                app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Shift Right");
            }
        }
        if (!io.WantTextInput)
        {
            if (ImGui::IsKeyPressed((ImGuiKey)app.prefs.keys.playPause))
            {
                playing = !playing;
                if (playing)
                {
                    holdSim = false;
                    lastFrameTime = ImGui::GetTime();
                    if (app.canvas.currentFrame >= totalFrames - 1)
                        app.canvas.currentFrame = 0;
                }
            }
            if (app.canvas.loopEndFrame >= 0)
            {
                ImGuiKey holdKey = (ImGuiKey)app.prefs.keys.holdSim;
                if (ImGui::IsKeyPressed(holdKey, false))
                {
                    playing = false;
                    holdEngaged = true;
                    if (!holdSim)
                    {
                        holdSim = true;
                        app.canvas.currentFrame = 0;
                        lastFrameTime = ImGui::GetTime();
                    }
                    else if (app.canvas.currentFrame > app.canvas.loopEndFrame)
                    {
                        app.canvas.currentFrame = std::min(app.canvas.loopFrame, app.canvas.loopEndFrame);
                    }
                }
                if (ImGui::IsKeyReleased(holdKey))
                {
                    holdEngaged = false;
                    int loopEnd = std::min(app.canvas.loopEndFrame, totalFrames - 1);
                    if (app.canvas.currentFrame >= app.canvas.loopFrame
                        && app.canvas.currentFrame <= loopEnd && loopEnd + 1 < totalFrames)
                    {
                        app.canvas.currentFrame = loopEnd + 1;
                        lastFrameTime = ImGui::GetTime();
                    }
                }
            }
        }

        ImGui::Separator();

        // Frame thumbnails
        int w = app.canvas.Width();
        int h = app.canvas.Height() - 1; // skip flag row
        // Scale each thumbnail to a consistent on-screen size so a single panel renders as
        // large as a full pad rather than a tiny block.
        float thumbScale = 46.0f / (float)std::max(w, h);
        float thumbW = w * thumbScale;
        float thumbH = h * thumbScale;

        ImGui::BeginChild("##thumbs", ImVec2(0, thumbH + 20), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (int f = 0; f < totalFrames; f++)
        {
            ImGui::PushID(f);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList *draw = ImGui::GetWindowDrawList();

            draw->AddRectFilled(pos, ImVec2(pos.x + thumbW, pos.y + thumbH), IM_COL32(15, 15, 15, 255));

            if (f == app.canvas.currentFrame)
                draw->AddRect(ImVec2(pos.x - 2, pos.y - 2),
                    ImVec2(pos.x + thumbW + 2, pos.y + thumbH + 2),
                    IM_COL32(255, 200, 0, 255), 0, 0, 2.0f);

            // Loop point marker (cyan, pointing up); shifted left when the
            // loop-end marker shares the frame.
            if (f == app.canvas.loopFrame)
            {
                float cx = pos.x + thumbW * 0.5f - (f == app.canvas.loopEndFrame ? 7.0f : 0.0f);
                float ty = pos.y + thumbH + 2;
                draw->AddTriangleFilled(
                    ImVec2(cx - 5, ty + 8),
                    ImVec2(cx + 5, ty + 8),
                    ImVec2(cx, ty),
                    IM_COL32(0, 200, 255, 220));
            }

            // Loop-end marker (orange, pointing down): later frames are the outro.
            if (f == app.canvas.loopEndFrame)
            {
                float cx = pos.x + thumbW * 0.5f + (f == app.canvas.loopFrame ? 7.0f : 0.0f);
                float ty = pos.y + thumbH + 2;
                draw->AddTriangleFilled(
                    ImVec2(cx - 5, ty),
                    ImVec2(cx + 5, ty),
                    ImVec2(cx, ty + 8),
                    IM_COL32(255, 160, 0, 220));
            }

            // Draw thumbnail
            const auto &frame = app.canvas.frames[f];
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    if (app.canvas.IsGutter(x, y)) continue;
                    Color c = frame.GetPixel(x, y, w);
                    if (c.IsBlack()) continue;
                    ImVec2 tl(pos.x + x * thumbScale, pos.y + y * thumbScale);
                    ImVec2 br(tl.x + thumbScale, tl.y + thumbScale);
                    draw->AddRectFilled(tl, br, IM_COL32(c.r, c.g, c.b, 255));
                }
            }

            // Clickable area
            if (ImGui::InvisibleButton("##thumb", ImVec2(thumbW, thumbH)))
                app.canvas.currentFrame = f;
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup("##frame_ctx");
            if (ImGui::BeginPopup("##frame_ctx"))
            {
                if (ImGui::MenuItem("Copy Frame", SHORTCUT_MOD "+C"))
                {
                    app.frameClipboard = app.canvas.frames[f];
                    app.frameClipboardValid = true;
                }
                if (ImGui::MenuItem("Paste Frame", SHORTCUT_MOD "+V", false, app.frameClipboardValid && (int)app.canvas.frames.size() < app.canvas.MaxFrames()))
                {
                    app.canvas.frames.insert(app.canvas.frames.begin() + f + 1, app.frameClipboard);
                    app.canvas.currentFrame = f + 1;
                    app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Paste Frame");
                }
                if (ImGui::MenuItem("Delete Frame", ImGui::GetKeyName((ImGuiKey)app.prefs.keys.deleteFrame), false, (int)app.canvas.frames.size() > 1))
                {
                    app.canvas.DeleteFrame(f);
                    app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Delete Frame");
                }
                ImGui::Separator();
                if (app.canvas.loopFrame == f)
                {
                    if (ImGui::MenuItem("Clear Loop Point"))
                    {
                        app.canvas.loopFrame = 0;
                        app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Clear Loop Point");
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Set Loop Point"))
                    {
                        app.canvas.loopFrame = f;
                        // A loop end before the new loop start is meaningless; drop it.
                        if (app.canvas.loopEndFrame >= 0 && app.canvas.loopEndFrame < f)
                            app.canvas.loopEndFrame = -1;
                        app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Set Loop Point");
                    }
                }
                if (app.canvas.loopEndFrame == f)
                {
                    if (ImGui::MenuItem("Clear Loop End"))
                    {
                        app.canvas.loopEndFrame = -1;
                        app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Clear Loop End");
                    }
                }
                else if (app.canvas.target == CanvasTarget::Host && f >= app.canvas.loopFrame)
                {
                    if (ImGui::MenuItem("Set Loop End"))
                    {
                        app.canvas.loopEndFrame = f;
                        app.dirty = true; app.colorCountsDirty = true; app.undo.SaveState(app.canvas, "Set Loop End");
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Last frame of the loop region; later frames become a\nrelease outro. Only used by deadsync host playback\n(per-panel judgement GIFs); SMX firmware and the\nofficial SDK ignore this marker.");
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::EndChild();

        // Frame info, total duration, and preview-speed control at the bottom.
        ImGui::Text("Frame %d / %d", app.canvas.currentFrame + 1, (int)app.canvas.frames.size());
        ImGui::SameLine();
        if (app.canvas.target == CanvasTarget::Host)
            ImGui::TextDisabled("(uncapped)");
        else if ((int)app.canvas.frames.size() >= app.canvas.MaxFrames())
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "(%d max)", app.canvas.MaxFrames());
        else
            ImGui::TextDisabled("(%d max)", app.canvas.MaxFrames());

        // Total animation time = sum of frame durations (the real exported loop length).
        float totalSecs = 0.0f;
        for (const auto &f : app.canvas.frames) totalSecs += f.duration;
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        if (app.canvas.loopEndFrame >= 0)
        {
            int loopEnd = std::min(app.canvas.loopEndFrame, (int)app.canvas.frames.size() - 1);
            float introSecs = 0.0f, loopSecs = 0.0f, outroSecs = 0.0f;
            for (int i = 0; i < app.canvas.loopFrame; i++)
                introSecs += app.canvas.frames[i].duration;
            for (int i = app.canvas.loopFrame; i <= loopEnd; i++)
                loopSecs += app.canvas.frames[i].duration;
            for (int i = loopEnd + 1; i < (int)app.canvas.frames.size(); i++)
                outroSecs += app.canvas.frames[i].duration;
            ImGui::Text("Total: %.2fs (intro %.2fs, loop %.2fs, outro %.2fs)", totalSecs, introSecs, loopSecs, outroSecs);
        }
        else if (app.canvas.loopFrame > 0)
        {
            float loopSecs = 0.0f;
            for (int i = app.canvas.loopFrame; i < (int)app.canvas.frames.size(); i++)
                loopSecs += app.canvas.frames[i].duration;
            ImGui::Text("Total: %.2fs (loop %.2fs)", totalSecs, loopSecs);
        }
        else
            ImGui::Text("Total: %.2fs", totalSecs);

        // Preview/playback speed. Authoring aid only: never changes saved timing or export.
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
        ImGui::TextDisabled("Speed:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        ImGui::SliderFloat("##speed", &app.previewSpeed, 0.05f, 4.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Preview and playback speed only.\nDoes not change saved frame timing or the exported GIF.");
        ImGui::SameLine();
        if (ImGui::SmallButton("1x")) app.previewSpeed = 1.0f;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset speed to 1.00x");
    }
    ImGui::End();
}
