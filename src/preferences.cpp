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
    } catch (...) {
        // Ignore malformed config, use defaults
    }
}

void Preferences::Save(const std::string &path) const
{
    json j;
    j["mode"] = mode;
    j["recent_files"] = recentFiles;

    std::ofstream f(path);
    if (f.is_open())
        f << j.dump(2) << "\n";
}
