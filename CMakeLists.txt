cmake_minimum_required(VERSION 3.20)
project(stepmaniax-gif-maker VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- hidapi (built from source, static) ---
# Build hidapi as a static library so end users don't need it installed.
set(HIDAPI_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(HIDAPI_WITH_HIDRAW ON CACHE BOOL "" FORCE)
set(HIDAPI_WITH_LIBUSB OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi.git
    GIT_TAG hidapi-0.15.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(hidapi)

# Linux hidraw backend needs libudev
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
endif()

# Make hidapi discoverable by the SMX SDK's find_package/pkg_check_modules.
# Variables are set right before add_subdirectory below.

# --- SDL3 ---
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.14
    GIT_SHALLOW TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# --- Dear ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# --- nlohmann/json ---
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json)

# Build ImGui as a static library with SDL3 + SDL3_Renderer backends
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3-static)

# --- stepmaniax-sdk-mp ---
# Vendored as a git submodule in src/vendor/stepmaniax-sdk-mp
set(SMX_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/stepmaniax-sdk-mp" CACHE PATH "Path to stepmaniax-sdk-mp")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_INTEGRATION_TESTS OFF CACHE BOOL "" FORCE)
# Prevent the SDK from searching for hidapi — we already built it above.
# Set as normal variables so they aren't shadowed by pkg_check_modules.
set(HIDAPI_FOUND TRUE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HIDAPI_LIBRARIES hidapi_hidraw)
elseif(APPLE)
    set(HIDAPI_LIBRARIES hidapi)
elseif(WIN32)
    set(HIDAPI_LIBRARIES hidapi_winapi)
else()
    set(HIDAPI_LIBRARIES hidapi)
endif()
# The SDK does #include <hidapi/hidapi.h>, so it needs the parent of hidapi/.
# We can't use hidapi_SOURCE_DIR directly because it contains a VERSION file
# that conflicts with the C++ <version> header on case-insensitive filesystems.
# Create a clean include wrapper directory with just the hidapi/ headers.
set(HIDAPI_INCLUDE_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/hidapi-include")
file(MAKE_DIRECTORY "${HIDAPI_INCLUDE_WRAPPER}/hidapi")
file(GLOB HIDAPI_HEADERS "${hidapi_SOURCE_DIR}/hidapi/*.h")
file(COPY ${HIDAPI_HEADERS} DESTINATION "${HIDAPI_INCLUDE_WRAPPER}/hidapi")
set(HIDAPI_INCLUDE_DIRS "${HIDAPI_INCLUDE_WRAPPER}")
set(HIDAPI_LIBRARY_DIRS "")
# Skip pkg-config search in the SDK (HIDAPI_FOUND is already set)
add_subdirectory(${SMX_SDK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/smx-sdk-mp)

# --- Main executable ---
set(APP_SOURCES
    src/main.cpp
    src/ui_menus.cpp
    src/ui_canvas.cpp
    src/ui_preview.cpp
    src/ui_timeline.cpp
    src/hardware.cpp
    src/preferences.cpp
    src/canvas.cpp
    src/gif_export.cpp
    src/gif_import.cpp
    src/undo.cpp
)

# Windows app icon
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")
    list(APPEND APP_SOURCES resources/app.rc)
endif()

add_executable(stepmaniax-gif-maker ${APP_SOURCES})
target_include_directories(stepmaniax-gif-maker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(stepmaniax-gif-maker PRIVATE SMX_GIF_MAKER_VERSION="${PROJECT_VERSION}")
target_link_libraries(stepmaniax-gif-maker PRIVATE imgui_lib smx-mp nlohmann_json::nlohmann_json)

# When linking smx-mp statically on Windows, prevent dllimport decoration
if(WIN32)
    target_compile_definitions(stepmaniax-gif-maker PRIVATE SMX_EXPORTS)
    # Hide console window in Release builds (keep main() as entry point)
    target_link_options(stepmaniax-gif-maker PRIVATE
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>)
endif()
