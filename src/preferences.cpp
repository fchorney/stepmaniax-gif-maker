#include "preferences.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

using json = nlohmann::json;

static void MakeDirRecursive(const std::string &path)
{
#ifdef _WIN32
    CreateDirectoryA(path.c_str(), nullptr);
#else
    mkdir(path.c_str(), 0755);
#endif
}

std::string GetConfigDir()
{
    std::string dir;

#ifdef _WIN32
    char *userProfile = std::getenv("UserProfile");
    if (userProfile)
        dir = std::string(userProfile) + "\\Documents\\stepmaniax-gif-maker";
#elif defined(__APPLE__)
    char *home = std::getenv("HOME");
    if (home)
        dir = std::string(home) + "/Library/Application Support/stepmaniax-gif-maker";
#else
    char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        dir = std::string(xdg) + "/stepmaniax-gif-maker";
    } else {
        char *home = std::getenv("HOME");
        if (home)
            dir = std::string(home) + "/.config/stepmaniax-gif-maker";
    }
#endif

    if (!dir.empty())
        MakeDirRecursive(dir);

    return dir;
}

void Preferences::Load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return;

    try {
        json j = json::parse(f);
        if (j.contains("mode")) mode = j["mode"];
        if (j.contains("recent_files")) recentFiles = j["recent_files"].get<std::vector<std::string>>();
        if (j.contains("max_undo_history")) maxUndoHistory = j["max_undo_history"];
        if (j.contains("prompt_on_unsaved")) promptOnUnsaved = j["prompt_on_unsaved"];
        if (j.contains("keybindings")) {
            auto &k = j["keybindings"];
            if (k.contains("draw")) keys.draw = k["draw"];
            if (k.contains("erase")) keys.erase = k["erase"];
            if (k.contains("fill")) keys.fill = k["fill"];
            if (k.contains("replace")) keys.replace = k["replace"];
            if (k.contains("pick")) keys.pick = k["pick"];
            if (k.contains("play_pause")) keys.playPause = k["play_pause"];
            if (k.contains("prev_frame")) keys.prevFrame = k["prev_frame"];
            if (k.contains("next_frame")) keys.nextFrame = k["next_frame"];
            if (k.contains("first_frame")) keys.firstFrame = k["first_frame"];
            if (k.contains("last_frame")) keys.lastFrame = k["last_frame"];
            if (k.contains("add_frame")) keys.addFrame = k["add_frame"];
            if (k.contains("dup_frame")) keys.dupFrame = k["dup_frame"];
            if (k.contains("delete_frame")) keys.deleteFrame = k["delete_frame"];
            if (k.contains("shift_left")) keys.shiftLeft = k["shift_left"];
            if (k.contains("shift_right")) keys.shiftRight = k["shift_right"];
        }
    } catch (...) {
        // Ignore malformed config, use defaults
    }
}

void Preferences::Save(const std::string &path) const
{
    json j;
    j["mode"] = mode;
    j["recent_files"] = recentFiles;
    j["max_undo_history"] = maxUndoHistory;
    j["prompt_on_unsaved"] = promptOnUnsaved;
    j["keybindings"] = {
        {"draw", keys.draw},
        {"erase", keys.erase},
        {"fill", keys.fill},
        {"replace", keys.replace},
        {"pick", keys.pick},
        {"play_pause", keys.playPause},
        {"prev_frame", keys.prevFrame},
        {"next_frame", keys.nextFrame},
        {"first_frame", keys.firstFrame},
        {"last_frame", keys.lastFrame},
        {"add_frame", keys.addFrame},
        {"dup_frame", keys.dupFrame},
        {"delete_frame", keys.deleteFrame},
        {"shift_left", keys.shiftLeft},
        {"shift_right", keys.shiftRight},
    };

    std::ofstream f(path);
    if (f.is_open())
        f << j.dump(2) << "\n";
}
