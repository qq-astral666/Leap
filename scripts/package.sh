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
