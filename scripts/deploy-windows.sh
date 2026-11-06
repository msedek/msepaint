#!/usr/bin/env bash
# Collects msepaint.exe with every DLL, Qt plugin and QML module it needs into a folder that runs on Windows
# without anything installed. Works from Linux on a cross build (no windeployqt needed).
# Usage: scripts/deploy-windows.sh <build-dir> <qt-windows-dir> <llvm-mingw-dir> <out-dir>
set -euo pipefail
BUILD="$1"; QTW="$2"; MINGW="$3"; OUT="$4"
OBJDUMP="$MINGW/bin/llvm-objdump"
rm -rf "$OUT"; mkdir -p "$OUT"
cp "$BUILD/msepaint.exe" "$OUT/"

# Plugins and QML modules used by the app.
mkdir -p "$OUT/plugins"
for d in platforms styles imageformats iconengines generic; do
    [ -d "$QTW/plugins/$d" ] && cp -r "$QTW/plugins/$d" "$OUT/plugins/"
done
mkdir -p "$OUT/qml/Qt/labs"
cp -r "$QTW/qml/QtQuick" "$QTW/qml/QtQml" "$OUT/qml/"
[ -d "$QTW/qml/QtCore" ] && cp -r "$QTW/qml/QtCore" "$OUT/qml/"
for m in folderlistmodel settings platform; do
    [ -d "$QTW/qml/Qt/labs/$m" ] && cp -r "$QTW/qml/Qt/labs/$m" "$OUT/qml/Qt/labs/"
done
# Not needed: debug files, designer metadata, Qt Quick 3D/VirtualKeyboard leftovers, unused control styles.
find "$OUT" \( -name "*.pdb" -o -name "*.debug" -o -name "*.qmltypes" -o -name "designer" \) -prune -exec rm -rf {} +
for s in Material Universal Imagine Fusion FluentWinUI3 iOS macOS Windows; do
    rm -rf "$OUT/qml/QtQuick/Controls/$s" "$OUT/qml/QtQuick/Controls/designer"
done
rm -rf "$OUT/qml/QtQuick/VirtualKeyboard" "$OUT/qml/QtQuick/Scene3D" "$OUT/qml/QtQuick/Scene2D" "$OUT/qml/QtQuick/Particles"

cat > "$OUT/qt.conf" <<'EOF'
[Paths]
Plugins = plugins
QmlImports = qml
EOF

# Copy the DLLs every PE file needs, recursively (Qt DLLs from bin/, runtime from the toolchain).
declare -A seen
queue=()
while IFS= read -r -d '' f; do queue+=("$f"); done < <(find "$OUT" \( -name "*.exe" -o -name "*.dll" \) -print0)
while [ ${#queue[@]} -gt 0 ]; do
    f="${queue[0]}"; queue=("${queue[@]:1}")
    while read -r dll; do
        key="${dll,,}"
        [ -n "${seen[$key]:-}" ] && continue
        seen[$key]=1
        for src in "$QTW/bin/$dll" "$QTW/$dll" "$MINGW/x86_64-w64-mingw32/bin/$dll"; do
            if [ -f "$src" ]; then
                [ -f "$OUT/$dll" ] || { cp "$src" "$OUT/"; queue+=("$OUT/$dll"); }
                break
            fi
        done
    done < <("$OBJDUMP" -p "$f" 2>/dev/null | sed -n 's/^\s*DLL Name: //p')
done
# Software OpenGL fallback for machines without a working GPU driver.
[ -f "$QTW/bin/opengl32sw.dll" ] && cp "$QTW/bin/opengl32sw.dll" "$OUT/"
cp "$(dirname "$0")/../LICENSE" "$(dirname "$0")/../THIRD_PARTY.md" "$OUT/" 2>/dev/null || true
echo "Deployed to $OUT ($(du -sh "$OUT" | cut -f1))"
