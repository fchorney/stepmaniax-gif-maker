#pragma once

#include <string>
#include <vector>

struct Preferences {
    std::string mode = "modern"; // "legacy" or "modern"
    std::vector<std::string> recentFiles;

    void Load(const std::string &path);
    void Save(const std::string &path) const;
};

// Returns the platform-specific config directory, creating it if needed.
// Windows: %UserProfile%\Documents\stepmaniax-gif-maker
// macOS:   ~/Library/Application Support/stepmaniax-gif-maker
// Linux:   $XDG_CONFIG_HOME/stepmaniax-gif-maker (fallback ~/.config/stepmaniax-gif-maker)
std::string GetConfigDir();
