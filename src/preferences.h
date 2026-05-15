#pragma once
/// User preferences: keybindings, recent files, and settings (JSON persistence).

#include <string>
#include <vector>

// Key values stored as int (ImGuiKey enum values).
// ImGuiKey_0=536, 1=537, ..., A=546, ..., Z=571, F1=572, ..., F24=595,
// Apostrophe=596, Comma=597, Minus=598, Period=599, Slash=600,
// Semicolon=601, Equal=602, LeftBracket=603
// Space=524, Delete=522, Home=519, End=520, LeftArrow=513, RightArrow=514
struct Keybindings
{
    int draw = 537;  // ImGuiKey_1
    int erase = 538; // ImGuiKey_2
    int fill = 539;  // ImGuiKey_3
    int replace = 540; // ImGuiKey_4
    int pick = 541;  // ImGuiKey_5

    // Timeline
    int playPause = 524;    // ImGuiKey_Space
    int prevFrame = 513;    // ImGuiKey_LeftArrow
    int nextFrame = 514;    // ImGuiKey_RightArrow
    int firstFrame = 519;   // ImGuiKey_Home
    int lastFrame = 520;    // ImGuiKey_End
    int addFrame = 546;     // ImGuiKey_A
    int dupFrame = 549;     // ImGuiKey_D
    int deleteFrame = 522;  // ImGuiKey_Delete
    int shiftLeft = 597;    // ImGuiKey_Comma
    int shiftRight = 599;   // ImGuiKey_Period
};

struct Preferences
{
    std::string mode = "modern"; // "legacy" or "modern"
    std::vector<std::string> recentFiles;
    Keybindings keys;
    int maxUndoHistory = 100;
    bool promptOnUnsaved = true;

    void Load(const std::string &path);
    void Save(const std::string &path) const;
    void AddRecentFile(const std::string &filePath);
};

// Returns the platform-specific config directory, creating it if needed.
// Windows: %UserProfile%\Documents\stepmaniax-gif-maker
// macOS:   ~/Library/Application Support/stepmaniax-gif-maker
// Linux:   $XDG_CONFIG_HOME/stepmaniax-gif-maker (fallback ~/.config/stepmaniax-gif-maker)
std::string GetConfigDir();
