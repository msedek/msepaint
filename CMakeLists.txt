cmake_minimum_required(VERSION 3.24)
project(msepaint VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Svg PrintSupport Widgets Test)
qt_standard_project_setup(REQUIRES 6.8)

# Núcleo: documento, capas, historial, herramientas y algoritmos. Sin QML.
add_library(msepaint_core STATIC
    src/core/Document.cpp
    src/core/History.cpp
    src/core/Tool.cpp
    src/core/ImageOps.cpp
    src/core/Brushes.cpp
    src/core/ShapeGeometry.cpp
    src/core/tools/Stroke.cpp
    src/core/tools/Handles.cpp
    src/core/tools/Select.cpp
    src/core/tools/Shape.cpp
    src/core/tools/Text.cpp
    src/core/tools/Fill.cpp
)
target_include_directories(msepaint_core PUBLIC src)
target_link_libraries(msepaint_core PUBLIC Qt6::Core Qt6::Gui)

# --- Archivos e impresión ---
# Formatos de imagen y proyecto .paint (HEIF). libheif es opcional: sin ella no hay .paint ni HEIC/AVIF.
target_sources(msepaint_core PRIVATE src/core/FileFormats.cpp)
find_path(LIBHEIF_INCLUDE_DIR libheif/heif.h PATH_SUFFIXES x86_64-linux-gnu aarch64-linux-gnu)
find_library(LIBHEIF_LIBRARY NAMES heif)
if(LIBHEIF_INCLUDE_DIR AND LIBHEIF_LIBRARY)
    message(STATUS "libheif: ${LIBHEIF_LIBRARY} (proyectos .paint activados)")
    target_include_directories(msepaint_core PRIVATE ${LIBHEIF_INCLUDE_DIR})
    target_link_libraries(msepaint_core PRIVATE ${LIBHEIF_LIBRARY})
    target_compile_definitions(msepaint_core PUBLIC MSEPAINT_HAVE_LIBHEIF)
else()
    message(WARNING "libheif no encontrada: se compila sin proyectos .paint ni HEIC/AVIF")
endif()

qt_add_executable(msepaint
    src/main.cpp
    src/ui/CanvasItem.cpp
    src/ui/AppController.cpp
    src/ui/ImageProviders.cpp
    src/ui/Demo.cpp
    src/ui/Printing.cpp
    src/ui/DesktopBackground.cpp
)
target_include_directories(msepaint PRIVATE src src/ui)
target_compile_definitions(msepaint PRIVATE MSEPAINT_VERSION="${PROJECT_VERSION}")
target_link_libraries(msepaint PRIVATE msepaint_core Qt6::Quick Qt6::QuickControls2 Qt6::Svg Qt6::PrintSupport Qt6::Widgets)

set_source_files_properties(qml/Theme.qml qml/Icons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
file(GLOB MSEPAINT_QML_CONTROLS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} qml/controls/*.qml)
file(GLOB MSEPAINT_QML_DIALOGS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} qml/dialogs/*.qml)
foreach(f ${MSEPAINT_QML_CONTROLS} ${MSEPAINT_QML_DIALOGS})
    get_filename_component(n ${f} NAME)
    set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${n})
endforeach()

file(GLOB MSEPAINT_ICONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} assets/icons/*.svg)

qt_add_qml_module(msepaint
    URI MsePaint
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Icons.qml
        qml/MenuBarRow.qml
        qml/Ribbon.qml
        qml/StatusBar.qml
        qml/LayersPanel.qml
        qml/SideSliders.qml
        qml/Rulers.qml
        qml/FileFlow.qml
        qml/TextToolbar.qml
        qml/CanvasResizeHandles.qml
        qml/CanvasContextMenu.qml
        qml/SettingsPage.qml
        ${MSEPAINT_QML_CONTROLS}
        ${MSEPAINT_QML_DIALOGS}
    SOURCES
        src/ui/CanvasItem.h
        src/ui/AppController.h
    RESOURCES
        assets/fonts/selawk.ttf
        assets/fonts/selawksb.ttf
        assets/fonts/selawkb.ttf
        assets/fonts/selawksl.ttf
        assets/fonts/FluentSystemIcons-Regular.ttf
        assets/fonts/FluentSystemIcons-Filled.ttf
        ${MSEPAINT_ICONS}
)

enable_testing()
function(msepaint_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE msepaint_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
msepaint_test(tst_document)
msepaint_test(tst_imageops)
msepaint_test(tst_fill)
msepaint_test(tst_fileformats)
msepaint_test(tst_tools)

# Instalación (binario, lanzador e ícono)
include(GNUInstallDirs)
install(TARGETS msepaint RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
configure_file(dist/msepaint.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/msepaint.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/msepaint.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES dist/msepaint.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# Windows: aplicación de ventana (sin consola), ícono y datos de versión del .exe.
if(WIN32)
    configure_file(dist/msepaint.rc.in ${CMAKE_CURRENT_BINARY_DIR}/msepaint.rc @ONLY)
    target_sources(msepaint PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/msepaint.rc)
    set_target_properties(msepaint PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
