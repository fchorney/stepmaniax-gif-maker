#pragma once

#include "canvas.h"
#include <string>

// Export the canvas as a valid SMX GIF file.
// Returns true on success, false on error (with error message in outError).
bool ExportGif(const Canvas &canvas, const std::string &path, std::string &outError);
