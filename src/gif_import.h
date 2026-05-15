#pragma once
/// GIF decoder for importing SMX-compatible animations using gif_load.h.

#include "canvas.h"
#include <string>

// Import a GIF file into the canvas.
// Validates dimensions (must be 14x15 or 23x24).
// Returns true on success, false on error (with message in outError).
// If pixelsModified is non-null, set to true if non-LED pixels were stripped.
bool ImportGif(const std::string &path, Canvas &canvas, std::string &outError, bool *pixelsModified = nullptr);
