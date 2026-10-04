#!/usr/bin/env bash
# Builds a self-contained, ad-hoc signed Leap.app + Leap.dmg.
#   ./scripts/package.sh            build only
#   ./scripts/package.sh --install  build and install into /Applications
set -euo pipefail
cd "$(dirname "$0")/.."

QT="$(brew --prefix qt)"
BREW="$(brew --prefix)"
OUT=build-release
APP="$OUT/Leap.app"

# 1. App icon: .icns from the 1024px PNG with Apple's own tools.
if [ ! -f resources/AppIcon.icns ] || [ resources/AppIcon.png -nt resources/AppIcon.icns ]; then
    ICONSET="$(mktemp -d)/AppIcon.iconset"
    mkdir -p "$ICONSET"
    for s in 16 32 128 256 512; do
        sips -z $s $s resources/AppIcon.png --out "$ICONSET/icon_${s}x${s}.png" >/dev/null
        sips -z $((s*2)) $((s*2)) resources/AppIcon.png --out "$ICONSET/icon_${s}x${s}@2x.png" >/dev/null
    done
    iconutil -c icns "$ICONSET" -o resources/AppIcon.icns
fi

# 2. Clean bundle BEFORE configuring: CMake writes Contents/Info.plist at
#    configure time, and macdeployqt is not idempotent.
rm -rf "$APP" "$OUT/Leap.dmg"
cmake -S . -B "$OUT" -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$QT" -DLEAP_BUILD_TESTS=OFF
cmake --build "$OUT"
test -f "$APP/Contents/Info.plist" || { echo "Info.plist missing"; exit 1; }

# 3. Bundle Qt. Homebrew splits Qt into kegs linked via @rpath.
"$QT/bin/macdeployqt" "$APP" -qmldir=qml -libpath="$QT/lib" -libpath="$BREW/lib" -no-strip

# --- Make the bundle independent of Homebrew -------------------------------
# 1) macdeployqt copies plugins (virtual keyboard, Qt3D, PDF…) whose Qt
#    frameworks it does not bundle. They can only load from Homebrew, so drop
#    every plugin that needs a framework missing from Contents/Frameworks.
find "$APP/Contents/PlugIns" "$APP/Contents/Resources/qml" -name '*.dylib' 2>/dev/null |
    while read -r plugin; do
        for fw in $(otool -L "$plugin" | awk '{print $1}' | sed -n 's#^@rpath/\([^/]*\.framework\)/.*#\1#p'); do
            if [ ! -d "$APP/Contents/Frameworks/$fw" ]; then
                echo "  drop ${plugin#"$APP/Contents/"} (needs $fw)"
                rm -f "$plugin"
                break
            fi
        done
    done
# 2) The linker leaves Homebrew's Qt dir as an LC_RPATH. dyld then resolves
#    @rpath/Qt* to /opt/homebrew, two QtCores get loaded, and after any
#    `brew upgrade qt` the app segfaults on launch. Strip absolute rpaths.
find "$APP/Contents" -type f \( -perm -u+x -o -name '*.dylib' \) | while read -r bin; do
    otool -l "$bin" 2>/dev/null | awk '/LC_RPATH/{f=1} f&&/path /{print $2; f=0}' |
        { grep '^/' || true; } | while read -r rp; do
            install_name_tool -delete_rpath "$rp" "$bin"
        done
done
# 3) Fail loudly instead of shipping a bundle that depends on Homebrew.
if find "$APP/Contents" -type f \( -perm -u+x -o -name '*.dylib' \) -exec otool -l {} \; 2>/dev/null |
        grep -A2 LC_RPATH | grep -q 'path /'; then
    echo "absolute rpath left in bundle"; exit 1
fi
# ---------------------------------------------------------------------------

# 4. Re-sign after install_name_tool rewrote the libraries.
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

# 5. DMG with the usual "drag Leap to Applications" layout.
STAGE="$(mktemp -d)/Leap"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
VERSION="$(/usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' "$APP/Contents/Info.plist")"
DMG="$OUT/Leap-$VERSION.dmg"
rm -f "$DMG"
hdiutil create -volname "Leap $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null
cp "$DMG" "$OUT/Leap.dmg"
echo "Built: $APP and $DMG"
open -R "$DMG" 2>/dev/null || true

if [ "${1:-}" = "--install" ]; then
    pkill -x Leap || true
    rm -rf /Applications/Leap.app
    cp -R "$APP" /Applications/
    touch /Applications/Leap.app
    /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f /Applications/Leap.app
    open /Applications/Leap.app
    echo "Installed to /Applications/Leap.app"
    echo "Ad-hoc signature changed: if keys don't work, remove Leap from"
    echo "System Settings > Privacy & Security > Accessibility and add it again."
fi
