#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "preferences.h"
#include "default_layout.h"
#include "canvas.h"
#include "gif_export.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <fstream>
#include <vector>

enum Tool { Tool_Draw = 0, Tool_Erase, Tool_Fill, Tool_Pick };

// File dialog state
static std::string g_exportPath;
static bool g_exportRequested = false;

static void ExportDialogCallback(void *userdata, const char * const *filelist, int filter)
{
    (void)userdata; (void)filter;
    if (filelist && filelist[0])
    {
        g_exportPath = filelist[0];
        g_exportRequested = true;
    }
}

// Flood fill that operates on LED positions within a single panel
static void FloodFill(CanvasFrame &frame, int w, int h, int x, int y, Color target, Color replacement, const Canvas &canvas)
{
    if (target == replacement) return;
    if (!canvas.IsLedPosition(x, y)) return;

    int panel = canvas.PanelAt(x, y);
    if (panel < 0) return;

    // Collect all LED positions in this panel
    std::vector<std::pair<int,int>> leds;
    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++)
            if (canvas.PanelAt(px, py) == panel && canvas.IsLedPosition(px, py))
                leds.push_back({px, py});

    // Fill all LEDs in the panel that match the target color
    for (auto [lx, ly] : leds)
        if (frame.GetPixel(lx, ly, w) == target)
            frame.SetPixel(lx, ly, w, replacement);
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "StepManiaX GIF Maker", 1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
    {
        printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        printf("SDL_CreateRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Set up config directory for imgui.ini and preferences
    std::string configDir = GetConfigDir();
    static std::string iniPath = configDir + "/imgui.ini";
    std::string prefsPath = configDir + "/preferences.json";
    if (!configDir.empty())
    {
        std::ifstream test(iniPath);
        if (!test.good())
        {
            std::ofstream out(iniPath);
            if (out.is_open())
                out << kDefaultImGuiIni;
        }
        io.IniFilename = iniPath.c_str();
    }

    Preferences prefs;
    prefs.Load(prefsPath);

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // App state
    Canvas canvas;
    canvas.Init(prefs.mode == "legacy" ? CanvasMode::Legacy : CanvasMode::Modern);

    int tool = Tool_Draw;
    ImVec4 currentColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    float cellSize = 20.0f;

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // Settings state (declared before menu bar which references it)
        static bool showSettings = false;
        static int *rebindTarget = nullptr;

        // --- Menu Bar ---
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New")) canvas.Init(canvas.mode);
                if (ImGui::MenuItem("Open .smxgifs")) {}
                if (ImGui::MenuItem("Save")) {}
                if (ImGui::MenuItem("Export GIF"))
                {
                    SDL_DialogFileFilter filters[] = { {"GIF files", "gif"} };
                    SDL_ShowSaveFileDialog(ExportDialogCallback, nullptr, window, filters, 1, nullptr);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Clear All")) canvas.ClearAll();
                if (ImGui::BeginMenu("Clear Panel"))
                {
                    for (int p = 0; p < 9; p++)
                    {
                        char label[16];
                        snprintf(label, sizeof(label), "Panel %d", p);
                        if (ImGui::MenuItem(label)) canvas.ClearPanel(p);
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Settings...")) showSettings = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Animation"))
            {
                if (ImGui::MenuItem("Add Frame"))
                    canvas.AddFrame();
                if (ImGui::MenuItem("Delete Frame"))
                    canvas.DeleteFrame(canvas.currentFrame);
                if (ImGui::MenuItem("Set Loop Point")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Hardware"))
            {
                if (ImGui::MenuItem("Preview on Pad")) {}
                if (ImGui::MenuItem("Upload to Firmware")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Re-enable Auto Lights")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // --- Settings Popup ---
        if (showSettings)
        {
            ImGui::OpenPopup("Settings");
            showSettings = false;
        }

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

            KeybindRow("Draw", &prefs.keys.draw);
            KeybindRow("Erase", &prefs.keys.erase);
            KeybindRow("Fill", &prefs.keys.fill);
            KeybindRow("Pick Color", &prefs.keys.pick);

            ImGui::Separator();
            if (ImGui::Button("Reset to Defaults"))
            {
                prefs.keys = Keybindings{};
                rebindTarget = nullptr;
            }
            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                rebindTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // --- GIF Export handling ---
        static std::string exportError;
        static bool showExportResult = false;
        static bool exportSuccess = false;

        if (g_exportRequested)
        {
            g_exportRequested = false;
            std::string path = g_exportPath;
            // Ensure .gif extension
            if (path.size() < 4 || path.substr(path.size() - 4) != ".gif")
                path += ".gif";
            exportSuccess = ExportGif(canvas, path, exportError);
            showExportResult = true;
        }

        if (showExportResult)
        {
            ImGui::OpenPopup("Export Result");
            showExportResult = false;
        }
        if (ImGui::BeginPopupModal("Export Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (exportSuccess)
                ImGui::Text("GIF exported successfully!");
            else
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Export failed: %s", exportError.c_str());
            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- Tool Panel ---
        ImGui::Begin("Tools");
        ImGui::Text("Drawing Tools");
        ImGui::Separator();
        ImGui::RadioButton("Draw", &tool, Tool_Draw);
        ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)prefs.keys.draw));
        ImGui::RadioButton("Erase", &tool, Tool_Erase);
        ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)prefs.keys.erase));
        ImGui::RadioButton("Fill", &tool, Tool_Fill);
        ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)prefs.keys.fill));
        ImGui::RadioButton("Pick Color", &tool, Tool_Pick);
        ImGui::SameLine(); ImGui::TextDisabled("(%s)", ImGui::GetKeyName((ImGuiKey)prefs.keys.pick));

        // Keyboard shortcuts (only when not typing in a text field)
        if (!io.WantTextInput)
        {
            if (ImGui::IsKeyPressed((ImGuiKey)prefs.keys.draw)) tool = Tool_Draw;
            if (ImGui::IsKeyPressed((ImGuiKey)prefs.keys.erase)) tool = Tool_Erase;
            if (ImGui::IsKeyPressed((ImGuiKey)prefs.keys.fill)) tool = Tool_Fill;
            if (ImGui::IsKeyPressed((ImGuiKey)prefs.keys.pick)) tool = Tool_Pick;
        }
        ImGui::Separator();
        ImGui::Text("Mode");
        int modeInt = (canvas.mode == CanvasMode::Modern) ? 1 : 0;
        if (ImGui::RadioButton("Legacy (14x15)", &modeInt, 0))
            canvas.Init(CanvasMode::Legacy);
        if (ImGui::RadioButton("Modern (23x24)", &modeInt, 1))
            canvas.Init(CanvasMode::Modern);
        ImGui::Separator();
        ImGui::SliderFloat("Zoom", &cellSize, 8.0f, 40.0f, "%.0f px");
        ImGui::End();

        // --- Color Palette ---
        ImGui::Begin("Palette");
        {
            Color drawColor = {
                (uint8_t)(currentColor.x * 255),
                (uint8_t)(currentColor.y * 255),
                (uint8_t)(currentColor.z * 255)
            };
            ImGui::ColorButton("Current", currentColor, 0, ImVec2(40, 40));
            ImGui::SameLine();
            ImGui::Text("R:%d G:%d B:%d", drawColor.r, drawColor.g, drawColor.b);
            ImGui::Separator();
            ImGui::ColorPicker3("##color", (float *)&currentColor,
                ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            ImGui::Separator();
            ImGui::Text("Panel Colors:");
            for (int p = 0; p < 9; p++)
            {
                int count = canvas.ColorCountForPanel(p);
                if (count > 15)
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "  Panel %d: %d/15 (!)", p, count);
                else
                    ImGui::Text("  Panel %d: %d/15", p, count);
            }
        }
        ImGui::End();

        // --- Canvas ---
        ImGui::Begin("Canvas");
        {
            const char *modeStr = (canvas.mode == CanvasMode::Modern) ? "Modern (23x24)" : "Legacy (14x15)";
            ImGui::Text("Mode: %s | Frame %d/%d", modeStr,
                canvas.currentFrame + 1, (int)canvas.frames.size());
            ImGui::Separator();

            ImDrawList *draw = ImGui::GetWindowDrawList();
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            int gridW = canvas.Width();
            int gridH = canvas.Height();
            ImVec2 canvasSize(gridW * cellSize, gridH * cellSize);

            // Invisible button for mouse interaction
            ImGui::InvisibleButton("##canvas", canvasSize);
            bool canvasHovered = ImGui::IsItemHovered();
            bool canvasActive = ImGui::IsItemActive();

            // Draw pixels
            const auto &frame = canvas.CurrentFrame();
            for (int y = 0; y < gridH; y++)
            {
                for (int x = 0; x < gridW; x++)
                {
                    ImVec2 tl(canvasPos.x + x * cellSize, canvasPos.y + y * cellSize);
                    ImVec2 br(tl.x + cellSize, tl.y + cellSize);

                    Color c = frame.GetPixel(x, y, gridW);
                    ImU32 fillColor;

                    if (canvas.IsFlagRow(x, y))
                        fillColor = IM_COL32(20, 20, 40, 255);
                    else if (canvas.IsGutter(x, y))
                        fillColor = IM_COL32(50, 50, 50, 255);
                    else if (c.IsBlack())
                        fillColor = IM_COL32(15, 15, 15, 255);
                    else
                        fillColor = IM_COL32(c.r, c.g, c.b, 255);

                    draw->AddRectFilled(tl, br, fillColor);

                    // Grid lines
                    ImU32 gridColor;
                    if (canvas.IsGutter(x, y))
                        gridColor = IM_COL32(100, 100, 100, 255);
                    else
                        gridColor = IM_COL32(40, 40, 40, 255);
                    draw->AddRect(tl, br, gridColor);

                    // LED position indicator (small dot)
                    if (canvas.IsLedPosition(x, y) && cellSize >= 12.0f)
                    {
                        float cx = tl.x + cellSize * 0.5f;
                        float cy = tl.y + cellSize * 0.5f;
                        float r = cellSize * 0.1f;
                        ImU32 dotColor = c.IsBlack() ? IM_COL32(80, 80, 80, 150) : IM_COL32(255, 255, 255, 80);
                        draw->AddCircleFilled(ImVec2(cx, cy), r, dotColor);
                    }
                }
            }

            // Mouse interaction
            if (canvasHovered || canvasActive)
            {
                ImVec2 mouse = ImGui::GetMousePos();
                int mx = (int)((mouse.x - canvasPos.x) / cellSize);
                int my = (int)((mouse.y - canvasPos.y) / cellSize);

                if (mx >= 0 && mx < gridW && my >= 0 && my < gridH &&
                    !canvas.IsGutter(mx, my) && !canvas.IsFlagRow(mx, my) &&
                    canvas.IsLedPosition(mx, my))
                {
                    // Hover highlight
                    ImVec2 htl(canvasPos.x + mx * cellSize, canvasPos.y + my * cellSize);
                    ImVec2 hbr(htl.x + cellSize, htl.y + cellSize);
                    draw->AddRect(htl, hbr, IM_COL32(255, 255, 0, 180), 0, 0, 2.0f);

                    // Tool actions on click/drag
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && canvasActive)
                    {
                        auto &editFrame = canvas.CurrentFrame();
                        Color drawColor = {
                            (uint8_t)(currentColor.x * 255),
                            (uint8_t)(currentColor.y * 255),
                            (uint8_t)(currentColor.z * 255)
                        };

                        switch (tool)
                        {
                        case Tool_Draw:
                            editFrame.SetPixel(mx, my, gridW, drawColor);
                            break;
                        case Tool_Erase:
                            editFrame.SetPixel(mx, my, gridW, Color{0, 0, 0});
                            break;
                        case Tool_Fill:
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            {
                                Color target = editFrame.GetPixel(mx, my, gridW);
                                FloodFill(editFrame, gridW, gridH, mx, my, target, drawColor, canvas);
                            }
                            break;
                        case Tool_Pick:
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            {
                                Color picked = editFrame.GetPixel(mx, my, gridW);
                                currentColor.x = picked.r / 255.0f;
                                currentColor.y = picked.g / 255.0f;
                                currentColor.z = picked.b / 255.0f;
                            }
                            break;
                        }
                    }
                }

                // Right-click context menu for clearing panels
                if (mx >= 0 && mx < gridW && my >= 0 && my < gridH)
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        ImGui::OpenPopup("##canvas_ctx");
                }
            }

            if (ImGui::BeginPopup("##canvas_ctx"))
            {
                if (ImGui::MenuItem("Clear This Panel"))
                {
                    ImVec2 mouse = ImGui::GetMousePos();
                    int mx = (int)((mouse.x - canvasPos.x) / cellSize);
                    int my = (int)((mouse.y - canvasPos.y) / cellSize);
                    int panel = canvas.PanelAt(mx, my);
                    canvas.ClearPanel(panel);
                }
                if (ImGui::MenuItem("Clear All Panels"))
                    canvas.ClearAll();
                ImGui::EndPopup();
            }
        }
        ImGui::End();

        // --- Preview ---
        ImGui::Begin("Preview");
        {
            static bool hwColors = true;
            ImGui::Text("Pad Preview");
            ImGui::Checkbox("Hardware colors (66%%)", &hwColors);
            ImGui::Separator();

            const auto &frame = canvas.CurrentFrame();
            int w = canvas.Width();
            int h = canvas.Height();
            float previewScale = 4.0f;
            ImDrawList *draw = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();

            for (int y = 0; y < h - 1; y++) // skip flag row
            {
                for (int x = 0; x < w; x++)
                {
                    Color c = frame.GetPixel(x, y, w);
                    if (canvas.IsGutter(x, y)) continue;

                    ImVec2 tl(pos.x + x * previewScale, pos.y + y * previewScale);
                    ImVec2 br(tl.x + previewScale, tl.y + previewScale);
                    if (!c.IsBlack())
                    {
                        uint8_t r = hwColors ? (uint8_t)(c.r * 0.6666f) : c.r;
                        uint8_t g = hwColors ? (uint8_t)(c.g * 0.6666f) : c.g;
                        uint8_t b = hwColors ? (uint8_t)(c.b * 0.6666f) : c.b;
                        draw->AddRectFilled(tl, br, IM_COL32(r, g, b, 255));
                    }
                    else
                        draw->AddRectFilled(tl, br, IM_COL32(5, 5, 5, 255));
                }
            }
            ImGui::Dummy(ImVec2(w * previewScale, (h - 1) * previewScale));
        }
        ImGui::End();

        // --- Timeline ---
        ImGui::Begin("Timeline");
        {
            static bool playing = false;
            static double lastFrameTime = 0;

            int totalFrames = (int)canvas.frames.size();

            // Playback logic
            if (playing && totalFrames > 1)
            {
                double now = ImGui::GetTime();
                float frameDur = canvas.CurrentFrame().duration;
                if (now - lastFrameTime >= frameDur)
                {
                    lastFrameTime = now;
                    canvas.currentFrame = (canvas.currentFrame + 1) % totalFrames;
                }
            }

            // Controls
            if (!playing)
            {
                if (ImGui::Button("Play"))
                {
                    playing = true;
                    lastFrameTime = ImGui::GetTime();
                }
            }
            else
            {
                if (ImGui::Button("Pause"))
                    playing = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("|<"))
                canvas.currentFrame = 0;
            ImGui::SameLine();
            if (ImGui::Button("<") && canvas.currentFrame > 0)
                canvas.currentFrame--;
            ImGui::SameLine();
            if (ImGui::Button(">") && canvas.currentFrame < totalFrames - 1)
                canvas.currentFrame++;
            ImGui::SameLine();
            if (ImGui::Button(">|"))
                canvas.currentFrame = totalFrames - 1;
            ImGui::SameLine();
            ImGui::Text("Frame %d / %d", canvas.currentFrame + 1, totalFrames);

            // Frame operations
            ImGui::SameLine();
            ImGui::Spacing(); ImGui::SameLine();
            if (ImGui::Button("-") && totalFrames > 1) canvas.DeleteFrame(canvas.currentFrame);
            ImGui::SameLine();
            if (ImGui::Button("+")) { canvas.AddFrame(); canvas.currentFrame = (int)canvas.frames.size() - 1; }
            ImGui::SameLine();
            if (ImGui::Button("Dup")) canvas.DuplicateFrame(canvas.currentFrame);
            ImGui::SameLine();
            if (ImGui::Button("<<") && canvas.currentFrame > 0)
            {
                std::swap(canvas.frames[canvas.currentFrame], canvas.frames[canvas.currentFrame - 1]);
                canvas.currentFrame--;
            }
            ImGui::SameLine();
            if (ImGui::Button(">>") && canvas.currentFrame < totalFrames - 1)
            {
                std::swap(canvas.frames[canvas.currentFrame], canvas.frames[canvas.currentFrame + 1]);
                canvas.currentFrame++;
            }
            ImGui::SameLine();
            ImGui::Text("(max 32)");

            // Arrow key navigation (when not typing)
            if (!io.WantTextInput && !playing)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && canvas.currentFrame > 0)
                    canvas.currentFrame--;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && canvas.currentFrame < totalFrames - 1)
                    canvas.currentFrame++;
            }

            ImGui::Separator();

            // Frame thumbnails
            int w = canvas.Width();
            int h = canvas.Height() - 1; // skip flag row
            float thumbScale = 2.0f;
            float thumbW = w * thumbScale;
            float thumbH = h * thumbScale;

            ImGui::BeginChild("##thumbs", ImVec2(0, thumbH + 20), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (int f = 0; f < totalFrames; f++)
            {
                ImGui::PushID(f);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList *draw = ImGui::GetWindowDrawList();

                // Background for thumbnail
                draw->AddRectFilled(pos, ImVec2(pos.x + thumbW, pos.y + thumbH), IM_COL32(15, 15, 15, 255));

                // Highlight current frame with outline
                if (f == canvas.currentFrame)
                    draw->AddRect(ImVec2(pos.x - 2, pos.y - 2),
                        ImVec2(pos.x + thumbW + 2, pos.y + thumbH + 2),
                        IM_COL32(255, 200, 0, 255), 0, 0, 2.0f);

                // Draw thumbnail
                const auto &frame = canvas.frames[f];
                for (int y = 0; y < h; y++)
                {
                    for (int x = 0; x < w; x++)
                    {
                        if (canvas.IsGutter(x, y)) continue;
                        Color c = frame.GetPixel(x, y, w);
                        if (c.IsBlack()) continue;
                        ImVec2 tl(pos.x + x * thumbScale, pos.y + y * thumbScale);
                        ImVec2 br(tl.x + thumbScale, tl.y + thumbScale);
                        draw->AddRectFilled(tl, br, IM_COL32(c.r, c.g, c.b, 255));
                    }
                }

                // Clickable area
                if (ImGui::InvisibleButton("##thumb", ImVec2(thumbW, thumbH)))
                    canvas.currentFrame = f;

                ImGui::SameLine();
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::End();

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Save preferences
    prefs.mode = (canvas.mode == CanvasMode::Modern) ? "modern" : "legacy";
    prefs.Save(prefsPath);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
