#pragma once

#include <string>
#include <vector>

// Key values stored as int (ImGuiKey enum values).
// Defaults: 1=Draw, 2=Erase, 3=Fill, 4=Pick
struct Keybindings {
    int draw = 537;  // ImGuiKey_1
    int erase = 538; // ImGuiKey_2
    int fill = 539;  // ImGuiKey_3
    int pick = 540;  // ImGuiKey_4
};

struct Preferences {
    std::string mode = "modern"; // "legacy" or "modern"
    std::vector<std::string> recentFiles;
    Keybindings keys;

    void Load(const std::string &path);
    void Save(const std::string &path) const;
};

// Returns the platform-specific config directory, creating it if needed.
// Windows: %UserProfile%\Documents\stepmaniax-gif-maker
// macOS:   ~/Library/Application Support/stepmaniax-gif-maker
// Linux:   $XDG_CONFIG_HOME/stepmaniax-gif-maker (fallback ~/.config/stepmaniax-gif-maker)
std::string GetConfigDir();
