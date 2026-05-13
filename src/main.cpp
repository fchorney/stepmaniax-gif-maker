#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <cstdio>

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

    // Style: dark theme with some polish
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

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

        // Dockspace over the entire window
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // --- Menu Bar ---
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New")) {}
                if (ImGui::MenuItem("Open .smxgifs")) {}
                if (ImGui::MenuItem("Save")) {}
                if (ImGui::MenuItem("Export GIF")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Quit")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Animation"))
            {
                if (ImGui::MenuItem("Add Frame")) {}
                if (ImGui::MenuItem("Delete Frame")) {}
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

        // --- Tool Panel ---
        ImGui::Begin("Tools");
        ImGui::Text("Drawing Tools");
        ImGui::Separator();
        static int tool = 0;
        ImGui::RadioButton("Draw", &tool, 0);
        ImGui::RadioButton("Erase", &tool, 1);
        ImGui::RadioButton("Fill", &tool, 2);
        ImGui::RadioButton("Pick Color", &tool, 3);
        ImGui::End();

        // --- Color Palette ---
        ImGui::Begin("Palette");
        ImGui::Text("Colors (0/15)");
        ImGui::Separator();
        static ImVec4 currentColor = ImVec4(1, 0, 0, 1);
        ImGui::ColorPicker3("##color", (float *)&currentColor,
            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::End();

        // --- Canvas ---
        ImGui::Begin("Canvas");
        ImGui::Text("Mode: Modern (23x24) | Pad 1 | Released");
        ImGui::Separator();
        // Placeholder: draw a grid representing the 3x3 panel layout
        ImDrawList *draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float cellSize = 20.0f;
        int gridW = 23, gridH = 24;
        for (int y = 0; y < gridH; y++)
        {
            for (int x = 0; x < gridW; x++)
            {
                ImVec2 tl(pos.x + x * cellSize, pos.y + y * cellSize);
                ImVec2 br(tl.x + cellSize, tl.y + cellSize);
                draw->AddRectFilled(tl, br, IM_COL32(30, 30, 30, 255));
                draw->AddRect(tl, br, IM_COL32(60, 60, 60, 255));
            }
        }
        ImGui::Dummy(ImVec2(gridW * cellSize, gridH * cellSize));
        ImGui::End();

        // --- Preview ---
        ImGui::Begin("Preview");
        ImGui::Text("Pad Preview");
        ImGui::Separator();
        ImGui::Text("Pad 1: (animating)");
        ImGui::Text("Pad 2: (no data)");
        ImGui::End();

        // --- Timeline ---
        ImGui::Begin("Timeline");
        static int currentFrame = 0;
        static int totalFrames = 1;
        ImGui::SliderInt("Frame", &currentFrame, 0, totalFrames - 1);
        ImGui::SameLine();
        ImGui::Text("/ %d", totalFrames);
        if (ImGui::Button("Play")) {}
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {}
        ImGui::SameLine();
        ImGui::Text("Duration: 33ms (30 FPS)");
        ImGui::End();

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
