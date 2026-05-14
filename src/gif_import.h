#pragma once

#include "canvas.h"
#include <string>

// Import a GIF file into the canvas.
// Validates dimensions (must be 14x15 or 23x24).
// Returns true on success, false on error (with message in outError).
bool ImportGif(const std::string &path, Canvas &canvas, std::string &outError);
