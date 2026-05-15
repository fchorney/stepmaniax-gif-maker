#pragma once

#include "canvas.h"
#include <string>
#include <vector>

// Export the canvas as a valid SMX GIF file.
// Returns true on success, false on error (with error message in outError).
bool ExportGif(const Canvas &canvas, const std::string &path, std::string &outError);

// Export the canvas as a GIF to a memory buffer.
bool ExportGifToMemory(const Canvas &canvas, std::vector<char> &outData, std::string &outError);
