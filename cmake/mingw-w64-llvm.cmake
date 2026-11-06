# Compilación cruzada para Windows x64 con llvm-mingw (UCRT).
# Uso: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-llvm.cmake -DLLVM_MINGW=/ruta/llvm-mingw -DQT_WIN=/ruta/qt-win \
#            -DQT_HOST_PATH=/ruta/qt-linux-misma-version ...
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang)
set(CMAKE_CXX_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang++)
set(CMAKE_RC_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH ${QT_WIN} ${LLVM_MINGW}/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
list(APPEND CMAKE_PREFIX_PATH ${QT_WIN})
