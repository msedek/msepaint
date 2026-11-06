# Third-party components

## Included in this repository

| Component | Files | License |
|---|---|---|
| Selawik font | `assets/fonts/selaw*.ttf` | SIL Open Font License 1.1 (`assets/fonts/LICENSE-Selawik.txt`) |
| Fluent UI System Icons | `assets/fonts/FluentSystemIcons-*` | MIT (`assets/fonts/LICENSE-FluentSystemIcons.txt`) |

All other icons and the application icon were drawn for msepaint (`scripts/gen-icons.py`, `dist/msepaint.svg`).

## Bundled in the release packages

The AppImage and the Windows packages also ship shared libraries that are dynamically linked and can be replaced. Their licenses and
copyright notices are included inside the AppImage (`usr/share/doc`) when the build system provides them.

| Library | License | Source |
|---|---|---|
| Qt 6 | LGPL-3.0 | <https://www.qt.io/download-open-source> |
| libheif | LGPL-3.0 | <https://github.com/strukturag/libheif> |
| libde265 | LGPL-3.0 | <https://github.com/strukturag/libde265> |
| libaom | BSD-2-Clause | <https://aomedia.googlesource.com/aom> |
| ICU | Unicode License | <https://icu.unicode.org> |
| LLVM libc++ / libunwind (Windows) | Apache-2.0 with LLVM exception | <https://github.com/llvm/llvm-project> |
| Other system libraries (freetype, fontconfig, libpng, zlib, ...) | their respective open source licenses | your distribution's source packages |
