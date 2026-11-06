import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import MsePaint

// Ventana principal: grilla de 4 filas.
ApplicationWindow {
    id: win
    required property AppController app
    width: 1280
    height: 800
    visible: true
    title: app.windowTitle // --- Archivos e impresión ---
    color: Theme.windowBg
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    property bool ribbonAutoHide: false
    property bool ribbonShown: true          // en modo auto-ocultar: desplegada o no
    property bool layersOpen: false
    property bool thumbnailOpen: false
    readonly property bool ribbonVisible: !ribbonAutoHide || ribbonShown

    function fitToWindow() { win.app.fitToWindow(scroller.width - 64, scroller.height - 64) }
    // --- Archivos e impresión --- (FileFlow.qml)
    function saveAs(format) { fileFlow.saveAs(format) }
    function save() { fileFlow.save() }
    onClosing: (close) => fileFlow.handleClosing(close)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Fila 0
        MenuBarRow {
            Layout.fillWidth: true
            app: win.app
            ribbonVisible: win.ribbonVisible
            ribbonAutoHide: win.ribbonAutoHide
            onRequestNew: fileFlow.newImage()
            onRequestOpen: fileFlow.open()
            onRequestOpenRecent: (path) => fileFlow.openRecent(path)
            onRequestPrint: fileFlow.printDocument()
            onRequestPageSetup: fileFlow.pageSetup()
            onRequestPrintPreview: fileFlow.printPreview()
            onRequestDesktopBackground: (mode) => fileFlow.desktopBackground(mode)
            onRequestSave: win.save()
            onRequestSaveAs: (f) => win.saveAs(f)
            onRequestImport: importDialog.open()
            onRequestFitToWindow: win.fitToWindow()
            onRequestFullScreen: win.visibility = (win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen)
            onRequestThumbnail: win.thumbnailOpen = !win.thumbnailOpen
            onRequestToggleRibbon: win.toggleRibbon()
            onRequestRibbonMode: (autoHide) => { win.ribbonAutoHide = autoHide; win.ribbonShown = !autoHide }
            onRequestExit: win.close()
            // --- Diálogos de imagen ---
            onRequestImageProperties: imagePropertiesDialog.open()
            onRequestSettings: settingsPopup.open()
        }

        // Fila 1
        Ribbon {
            id: ribbon
            Layout.fillWidth: true
            visible: win.ribbonVisible
            app: win.app
            layersOpen: win.layersOpen
            onToggleLayers: win.layersOpen = !win.layersOpen
            // --- Diálogos de imagen ---
            onRequestResizeDialog: resizeSkewDialog.open()
            onRequestRotateCustom: rotateCustomDialog.open()
            onRequestEditColors: editColorsDialog.openFor(false)
            onRequestRibbonMode: (autoHide) => { win.ribbonAutoHide = autoHide; win.ribbonShown = !autoHide }
        }

        // Fila 2: zona de trabajo
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                id: workArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Rulers {
                    id: rulers
                    anchors.fill: parent
                    visible: win.app.showRulers
                    app: win.app
                    canvasItem: canvas
                    flick: scroller
                }

                Flickable {
                    id: scroller
                    anchors.fill: parent
                    anchors.leftMargin: win.app.showRulers ? 20 : 0
                    anchors.topMargin: win.app.showRulers ? 20 : 0
                    contentWidth: Math.max(canvas.width + 64, width)
                    contentHeight: Math.max(canvas.height + 64, height)
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: false
                    clip: true
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    Rectangle { // borde de 1 px del lienzo
                        x: canvas.x - 1; y: canvas.y - 1
                        width: canvas.width + 2; height: canvas.height + 2
                        color: "transparent"
                        border.width: 1; border.color: Theme.canvasBorder
                    }
                    CanvasItem {
                        id: canvas
                        controller: win.app
                        width: implicitWidth
                        height: implicitHeight
                        x: Math.max(32, (scroller.contentWidth - width) / 2)
                        y: Math.max(32, (scroller.contentHeight - height) / 2)
                    }

                    CanvasResizeHandles { app: win.app; canvasItem: canvas }

                    WheelHandler {
                        acceptedModifiers: Qt.ControlModifier
                        onWheel: (ev) => ev.angleDelta.y > 0 ? win.app.zoomIn() : win.app.zoomOut()
                    }
                    WheelHandler {
                        acceptedModifiers: Qt.NoModifier
                        onWheel: (ev) => {
                            scroller.contentY = Math.max(0, Math.min(scroller.contentHeight - scroller.height, scroller.contentY - ev.angleDelta.y))
                            scroller.contentX = Math.max(0, Math.min(scroller.contentWidth - scroller.width, scroller.contentX - ev.angleDelta.x))
                        }
                    }
                    WheelHandler {
                        acceptedModifiers: Qt.ShiftModifier
                        onWheel: (ev) => scroller.contentX = Math.max(0, Math.min(scroller.contentWidth - scroller.width, scroller.contentX - ev.angleDelta.y))
                    }
                }

                // --- Herramientas ---
                TextToolbar {
                    app: win.app
                    windowWidth: win.width
                    visible: win.app.textEditing
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 8
                    z: 10
                }
                Binding {
                    target: win.app
                    property: "viewOrigin"
                    value: Qt.point(Math.max(0, (scroller.contentX - canvas.x) / win.app.zoom),
                                    Math.max(0, (scroller.contentY - canvas.y) / win.app.zoom))
                }
                Connections {
                    target: win.app
                    // La lupa centra la vista en el punto del clic.
                    function onCenterViewOn(pt) {
                        Qt.callLater(() => {
                            const cx = canvas.x + pt.x * win.app.zoom - scroller.width / 2
                            const cy = canvas.y + pt.y * win.app.zoom - scroller.height / 2
                            scroller.contentX = Math.max(0, Math.min(scroller.contentWidth - scroller.width, cx))
                            scroller.contentY = Math.max(0, Math.min(scroller.contentHeight - scroller.height, cy))
                        })
                    }
                }

                SideSliders {
                    app: win.app
                    windowHeight: win.height
                    x: win.app.showRulers ? 32 : 12
                    y: 16
                    width: 100
                    height: parent.height - 32
                }
            }

            Rectangle {
                visible: win.thumbnailOpen
                Layout.preferredWidth: 460
                Layout.fillHeight: true
                Layout.margins: 12
                Layout.leftMargin: 0
                color: Theme.layerFill
                radius: Theme.overlayRadius
                border.width: 1; border.color: Theme.cardStrokeSolid
                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 24; anchors.bottomMargin: 24
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.leftMargin: 24; text: qsTr("Thumbnail"); font.family: Theme.uiFont; font.pixelSize: 20; font.weight: Font.DemiBold; color: Theme.textPrimary }
                        Item { Layout.fillWidth: true }
                        PButton { Layout.rightMargin: 16; glyph: Icons.close; onClicked: win.thumbnailOpen = false }
                    }
                    Image {
                        Layout.fillWidth: true
                        Layout.margins: 24; Layout.topMargin: 12
                        Layout.preferredHeight: width * win.app.canvasSize.height / Math.max(1, win.app.canvasSize.width)
                        fillMode: Image.PreserveAspectFit
                        cache: false
                        source: win.thumbnailOpen ? "image://composite/" + win.app.layersRevision : ""
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            LayersPanel {
                visible: win.layersOpen
                Layout.fillHeight: true
                Layout.margins: 12
                Layout.leftMargin: 0
                app: win.app
                onRequestBackgroundColor: editColorsDialog.openFor(true) // --- Diálogos de imagen ---
            }
        }

        // Fila 3
        StatusBar {
            Layout.fillWidth: true
            visible: win.app.showStatusBar
            app: win.app
            windowWidth: win.width
            onRequestFitToWindow: win.fitToWindow()
        }
    }

    function toggleRibbon() {
        if (!win.ribbonAutoHide) { win.ribbonAutoHide = true; win.ribbonShown = false }
        else win.ribbonShown = !win.ribbonShown
    }

    // ---- Diálogos de archivo (los del sistema) ----
    FileDialog {
        id: importDialog
        title: qsTr("Import to canvas")
        nameFilters: win.app.openNameFilters
        onAccepted: win.app.importFile(selectedFile)
    }
    FileFlow { id: fileFlow; anchors.fill: parent; app: win.app; window: win } // --- Archivos e impresión ---

    // --- Diálogos de imagen ---
    ResizeSkewDialog { id: resizeSkewDialog; app: win.app }
    // --- Herramientas ---
    CanvasContextMenu {
        id: canvasMenu
        app: win.app
        onRequestRotateCustom: rotateCustomDialog.open()
        onRequestResize: resizeSkewDialog.open()
    }
    Connections {
        target: win.app
        function onContextMenuRequested() { canvasMenu.popup() }
    }
    RotateCustomDialog { id: rotateCustomDialog; app: win.app }
    ImagePropertiesDialog { id: imagePropertiesDialog; app: win.app }
    EditColorsDialog { id: editColorsDialog; app: win.app } // también «Background color» (Change color / Ctrl+Shift+B)
    // Ajustes a pantalla completa: popup modal sin velo, así los atajos de la ventana quedan bloqueados mientras está
    // abierta; Esc o Volver la cierran.
    Popup {
        id: settingsPopup
        parent: Overlay.overlay
        x: 0; y: 0
        width: parent ? parent.width : 0
        height: parent ? parent.height : 0
        modal: true
        dim: false
        padding: 0
        closePolicy: Popup.CloseOnEscape
        background: null
        contentItem: SettingsPage { app: win.app; onBack: settingsPopup.close() }
    }
    Shortcut { sequence: "Ctrl+W"; onActivated: resizeSkewDialog.open() }
    Shortcut { sequence: "Ctrl+E"; onActivated: imagePropertiesDialog.open() }
    Component.onCompleted: {
        // Desarrollo: ver AppController::devOpenDialog (MSEPAINT_OPEN_DIALOG).
        const d = win.app.devOpenDialog()
        if (d === "resize") resizeSkewDialog.open()
        else if (d === "rotate") rotateCustomDialog.open()
        else if (d === "properties") imagePropertiesDialog.open()
        else if (d === "colors") editColorsDialog.openFor(false)
        else if (d === "background") editColorsDialog.openFor(true)
        else if (d === "settings") settingsPopup.open()
    }

    // ---- Atajos globales ----
    Shortcut { sequence: "Ctrl+N"; onActivated: fileFlow.newImage() }
    Shortcut { sequence: "Ctrl+O"; onActivated: fileFlow.open() }
    Shortcut { sequence: "Ctrl+P"; onActivated: fileFlow.printDocument() }
    Shortcut { sequence: "Ctrl+S"; onActivated: win.save() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: win.saveAs("paint") }
    Shortcut { sequence: "F12"; onActivated: win.saveAs("") }
    Shortcut { sequence: "F11"; onActivated: win.visibility = (win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen) }
    Shortcut { sequence: "Ctrl+T"; onActivated: win.toggleRibbon() }
    Shortcut { sequence: "Ctrl+Z"; onActivated: win.app.undo() }
    Shortcut { sequence: "Ctrl+Y"; onActivated: win.app.redo() }
    Shortcut { sequences: ["Ctrl+X", "Shift+Del"]; onActivated: win.app.cut() }
    Shortcut { sequences: ["Ctrl+C", "Ctrl+Ins"]; onActivated: win.app.copy() }
    Shortcut { sequence: "Ctrl+Shift+C"; onActivated: win.app.copyVisibleLayers() }
    Shortcut { sequences: ["Ctrl+V", "Shift+Ins"]; onActivated: win.app.paste() }
    Shortcut { sequence: "Ctrl+A"; onActivated: win.app.selectAll() }
    Shortcut { sequence: "Del"; onActivated: win.app.deleteSelection() }
    Shortcut { sequence: "Ctrl+Shift+X"; onActivated: win.app.crop() }
    Shortcut { sequence: "Ctrl+Shift+I"; onActivated: win.app.invertColors() }
    Shortcut { sequences: ["Ctrl+PgUp", "Ctrl+=", "Ctrl++"]; onActivated: win.app.zoomIn() }
    Shortcut { sequences: ["Ctrl+PgDown", "Ctrl+-"]; onActivated: win.app.zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: win.app.zoom100() }
    Shortcut { sequence: "Ctrl+1"; onActivated: win.fitToWindow() }
    Shortcut { sequence: "Ctrl+R"; onActivated: win.app.showRulers = !win.app.showRulers }
    Shortcut { sequence: "Ctrl+G"; onActivated: win.app.showGridlines = !win.app.showGridlines }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: win.app.addLayer() }
    Shortcut { sequence: "Ctrl+Shift+Del"; onActivated: win.app.deleteLayer() }
    Shortcut { sequence: "Ctrl+H"; onActivated: win.app.toggleLayerVisible(win.app.activeLayer) }
    Shortcut { sequence: "Ctrl+Shift+PgUp"; onActivated: win.app.moveLayerUp() }
    Shortcut { sequence: "Ctrl+Shift+PgDown"; onActivated: win.app.moveLayerDown() }
    Shortcut { sequence: "Ctrl+Shift+H"; onActivated: win.app.toggleBackgroundVisible() }
    Shortcut { sequence: "Ctrl+Shift+B"; onActivated: editColorsDialog.openFor(true) } // --- Diálogos de imagen ---
    Shortcut { sequence: "Esc"; onActivated: { if (win.ribbonAutoHide && win.ribbonShown) win.ribbonShown = false; else win.app.cancelPending() } }
    // Herramientas sin modificador
    Shortcut { sequence: "S"; onActivated: win.app.tool = "select" }
    Shortcut { sequence: "P"; onActivated: win.app.tool = "pencil" }
    Shortcut { sequence: "B"; onActivated: win.app.tool = "fill" }
    Shortcut { sequence: "T"; onActivated: win.app.tool = "text" }
    Shortcut { sequence: "E"; onActivated: win.app.tool = "eraser" }
    Shortcut { sequence: "I"; onActivated: win.app.tool = "picker" }
    Shortcut { sequence: "Z"; onActivated: win.app.tool = "magnifier" }
}
