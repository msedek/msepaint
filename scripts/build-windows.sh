#!/usr/bin/env bash
# Cross-builds the Windows version from Linux: msepaint.exe, a portable .zip and an NSIS installer.
# Needs: llvm-mingw (UCRT) for Linux, Qt for Windows built with llvm-mingw, a host Qt of the same version, makensis.
# Usage: scripts/build-windows.sh <llvm-mingw-dir> <qt-windows-dir> <qt-host-dir>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MINGW="$1"; QTW="$2"; QTHOST="$3"
VERSION="$(grep -oP 'project\(msepaint VERSION \K[0-9.]+' "$ROOT/CMakeLists.txt")"
OUT="$ROOT/build/windows"
cmake -S "$ROOT" -B "$OUT/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64-llvm.cmake" -DLLVM_MINGW="$MINGW" -DQT_WIN="$QTW" -DQT_HOST_PATH="$QTHOST"
cmake --build "$OUT/build" --target msepaint
"$ROOT/scripts/deploy-windows.sh" "$OUT/build" "$QTW" "$MINGW" "$OUT/msepaint"
cp "$ROOT/dist/msepaint.ico" "$OUT/"
( cd "$OUT" && rm -f "msepaint-$VERSION-windows-x64-portable.zip" && python3 -m zipfile -c "msepaint-$VERSION-windows-x64-portable.zip" msepaint )
makensis -V2 -DVERSION="$VERSION" -DSRCDIR="$OUT/msepaint" -DOUTFILE="$OUT/msepaint-$VERSION-windows-x64-setup.exe" \
    "$ROOT/dist/windows/msepaint.nsi"
echo "Built $OUT/msepaint-$VERSION-windows-x64-setup.exe and the portable .zip"
