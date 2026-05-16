#!/bin/bash
# Package stepmaniax-gif-maker for local testing.
# Usage: ./package.sh [linux|macos]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/stepmaniax-gif-maker"

if [ ! -f "$BINARY" ]; then
    echo "Error: Build first with: mkdir build && cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

case "${1:-$(uname -s)}" in
    linux|Linux)
        echo "=== Packaging for Linux ==="
        mkdir -p dist/linux
        cp "$BINARY" dist/linux/
        # Create .desktop file
        cat > dist/linux/stepmaniax-gif-maker.desktop << EOF
[Desktop Entry]
Name=StepManiaX GIF Maker
Comment=Pixel editor for SMX pad LED animations
Exec=stepmaniax-gif-maker
Icon=stepmaniax-gif-maker
Terminal=false
Type=Application
Categories=Graphics;
EOF
        if [ -f "$SCRIPT_DIR/resources/icon.png" ]; then
            cp "$SCRIPT_DIR/resources/icon.png" dist/linux/stepmaniax-gif-maker.png
        fi
        echo "Done: dist/linux/"
        echo "  Run: ./dist/linux/stepmaniax-gif-maker"
        echo "  Install .desktop file to ~/.local/share/applications/ for menu integration"
        ;;

    macos|Darwin)
        echo "=== Packaging for macOS ==="
        APP="dist/StepManiaX GIF Maker.app"
        rm -rf "$APP"
        mkdir -p "$APP/Contents/MacOS"
        mkdir -p "$APP/Contents/Resources"

        cp "$BINARY" "$APP/Contents/MacOS/stepmaniax-gif-maker"

        # Info.plist
        cat > "$APP/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>StepManiaX GIF Maker</string>
    <key>CFBundleDisplayName</key>
    <string>StepManiaX GIF Maker</string>
    <key>CFBundleIdentifier</key>
    <string>com.fchorney.stepmaniax-gif-maker</string>
    <key>CFBundleVersion</key>
    <string>0.1.1</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1.1</string>
    <key>CFBundleExecutable</key>
    <string>stepmaniax-gif-maker</string>
    <key>CFBundleIconFile</key>
    <string>icon</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
</dict>
</plist>
EOF

        if [ -f "$SCRIPT_DIR/resources/icon.icns" ]; then
            cp "$SCRIPT_DIR/resources/icon.icns" "$APP/Contents/Resources/icon.icns"
        fi

        echo "Done: $APP"
        echo "  Run: open \"$APP\""
        echo "  Or double-click in Finder"
        ;;

    *)
        echo "Usage: $0 [linux|macos]"
        echo "  Defaults to current platform if not specified."
        exit 1
        ;;
esac
