/// Application entry point, SDL/ImGui initialization, and main event loop.
#include "app_state.h"
#include "ui_menus.h"
#include "ui_canvas.h"
#include "ui_preview.h"
#include "ui_timeline.h"
#include "hardware.h"
#include "default_layout.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SMX.h>
#include <SDL3/SDL.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>

// Global file dialog state (defined here, declared extern in app_state.h)
std::string g_exportPath;
bool g_exportRequested = false;
std::string g_importPath;
bool g_importRequested = false;
std::string g_compositeRelPath;
std::string g_compositePrsPath;
bool g_compositeRelRequested = false;
bool g_compositePrsRequested = false;
std::atomic<int> g_uploadProgress{0};

// --- Logging ---
static FILE *g_logFile = nullptr;
static std::mutex g_logMutex;

static void LogWrite(const char *msg)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile) return;
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    fprintf(g_logFile, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec, msg);
    fflush(g_logFile);
}

static void SMXLogCb(const char *log)
{
    LogWrite(log);
}

// SMX SDK callback (unused, but required)
static void SMXUpdateCb(int pad, SMXUpdateCallbackReason reason, void *pUser)
{
    (void)pad; (void)reason; (void)pUser;
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SMX_SetLogCallback(SMXLogCb);
    SMX_Start(SMXUpdateCb, nullptr);

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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

    // Open log file
    if (!configDir.empty())
    {
        std::string logPath = configDir + "/stepmaniax-gif-maker.log";
        g_logFile = fopen(logPath.c_str(), "a");
        if (g_logFile)
            LogWrite("--- Application started ---");
    }

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.HoverDelayNormal = 2.0f;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // App state
    AppState app;
    app.prefs.Load(prefsPath);
    app.canvas.Init(app.prefs.mode == "legacy" ? CanvasMode::Legacy : CanvasMode::Modern);
    app.undo.SetMaxHistory(app.prefs.maxUndoHistory);
    app.undo.SaveState(app.canvas, "Initial");

    while (app.running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                if (app.prefs.promptOnUnsaved && app.dirty)
                    { app.pendingAction = Pending_Quit; app.showUnsavedDialog = true; }
                else
                    app.running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
            {
                if (app.prefs.promptOnUnsaved && app.dirty)
                    { app.pendingAction = Pending_Quit; app.showUnsavedDialog = true; }
                else
                    app.running = false;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        RenderMenus(app, window);
        RenderCanvas(app);
        RenderPreview(app);
        RenderTimeline(app);
        UpdateHardware(app);

        // Update window title (only when changed)
        {
            static std::string lastTitle;
            std::string title = "StepManiaX GIF Maker";
            if (!app.currentFilePath.empty())
            {
                size_t sep = app.currentFilePath.find_last_of("/\\");
                std::string filename = (sep != std::string::npos) ? app.currentFilePath.substr(sep + 1) : app.currentFilePath;
                title = filename + " - " + title;
            }
            if (app.dirty) title = "* " + title;
            if (title != lastTitle)
            {
                SDL_SetWindowTitle(window, title.c_str());
                lastTitle = title;
            }
        }

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Save preferences
    app.prefs.mode = (app.canvas.mode == CanvasMode::Modern) ? "modern" : "legacy";
    app.prefs.Save(prefsPath);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SMX_Stop();
    if (g_logFile)
    {
        LogWrite("--- Application stopped ---");
        fclose(g_logFile);
    }
    SDL_Quit();
    return 0;
}
