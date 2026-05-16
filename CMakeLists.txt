cmake_minimum_required(VERSION 3.20)
project(stepmaniax-gif-maker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

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
target_link_libraries(stepmaniax-gif-maker PRIVATE imgui_lib smx-mp nlohmann_json::nlohmann_json)

# Hide console window on Windows release builds
if(WIN32)
    set_target_properties(stepmaniax-gif-maker PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
