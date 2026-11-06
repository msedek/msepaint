#!/usr/bin/env bash
# Builds a portable msepaint AppImage with linuxdeploy + linuxdeploy-plugin-qt.
# Usage: scripts/build-appimage.sh            (uses the Qt found by qmake6/qmake, or $QMAKE)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/appimage"
VERSION="$(grep -oP 'project\(msepaint VERSION \K[0-9.]+' "$ROOT/CMakeLists.txt")"
TOOLS="$BUILD/tools"
mkdir -p "$TOOLS"
for t in linuxdeploy linuxdeploy-plugin-qt; do
    f="$TOOLS/$t-x86_64.AppImage"
    [ -x "$f" ] || { curl -fsSL -o "$f" "https://github.com/linuxdeploy/$t/releases/download/continuous/$t-x86_64.AppImage"; chmod +x "$f"; }
done
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
cmake -S "$ROOT" -B "$BUILD/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD/build"
rm -rf "$BUILD/AppDir"
DESTDIR="$BUILD/AppDir" cmake --install "$BUILD/build"
sed -i 's|^Exec=.*|Exec=msepaint %f|' "$BUILD/AppDir/usr/share/applications/msepaint.desktop"
# libheif plugins of the same version as the bundled libheif (HEIC/AVIF decoding).
EXTRA=()
for d in /usr/lib/x86_64-linux-gnu/libheif/plugins /usr/lib/libheif/plugins; do
    [ -d "$d" ] || continue
    mkdir -p "$BUILD/AppDir/usr/lib/libheif-plugins"
    for p in "$d"/libheif-libde265.so "$d"/libheif-aomdec.so "$d"/libheif-dav1d.so; do
        [ -f "$p" ] || continue
        cp "$p" "$BUILD/AppDir/usr/lib/libheif-plugins/"
        EXTRA+=(--deploy-deps-only "$BUILD/AppDir/usr/lib/libheif-plugins/$(basename "$p")")
    done
    break
done
cd "$BUILD"
export QML_SOURCES_PATHS="$ROOT/qml" EXTRA_PLATFORM_PLUGINS="libqwayland.so" EXTRA_QT_MODULES="waylandclient"
export LDAI_OUTPUT="msepaint-$VERSION-x86_64.AppImage"
"$TOOLS/linuxdeploy-x86_64.AppImage" --appdir AppDir "${EXTRA[@]}" --plugin qt --output appimage
echo "Built $BUILD/$LDAI_OUTPUT"
