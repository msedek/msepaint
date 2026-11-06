import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Fila 0: menús File/Edit/View, botones rápidos y Ajustes.
Rectangle {
    id: bar
    required property AppController app
    property bool ribbonVisible: true
    property bool ribbonAutoHide: false
    signal requestOpen()
    signal requestSaveAs(string format)
    signal requestImageProperties()
    signal requestSettings()
    signal requestToggleRibbon()
    signal requestFitToWindow()
    signal requestFullScreen()
    signal requestThumbnail()
    signal requestRibbonMode(bool autoHide)
    signal requestPrint()
    signal requestPageSetup()
    signal requestPrintPreview()
    signal requestDesktopBackground(string mode)
    signal requestExit()
    signal requestNew()
    signal requestSave()
    signal requestImport()
    signal requestOpenRecent(string path) // --- Archivos e impresión ---

    implicitHeight: 40
    color: "transparent"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.cardStrokeSolid }

    component MenuTitle: AbstractButton {
        id: mt
        property PMenu menu
        implicitHeight: 32
        implicitWidth: Math.max(36, label.implicitWidth + 22)
        hoverEnabled: true
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: Theme.controlRadius
            color: (mt.down || mt.menu.visible) ? Theme.subtlePressed : mt.hovered ? Theme.subtleHover : "transparent"
        }
        contentItem: Text {
            id: label
            text: mt.text
            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
        onClicked: menu.popup(mt, 0, mt.height + 2)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 4
        spacing: 0

        MenuTitle {
            text: qsTr("File")
            menu: PMenu {
                id: fileMenu
                // --- Archivos e impresión --- sin escritorio compatible no hay «Set as desktop background».
                Component.onCompleted: if (!bar.app.canSetDesktopBackground) { fileMenu.removeMenu(desktopMenu); fileMenu.removeItem(desktopSeparator) }
                PMenuItem { text: qsTr("New"); glyph: Icons.newDoc; shortcutText: "Ctrl+N"; onTriggered: bar.requestNew() }
                PMenuItem { text: qsTr("Open"); glyph: Icons.open; shortcutText: "Ctrl+O"; onTriggered: bar.requestOpen() }
                PMenu {
                    title: qsTr("Import to canvas")
                    reserveIcon: false
                    PMenuItem { text: qsTr("From a file"); onTriggered: bar.requestImport() }
                    PMenuItem { text: qsTr("From scanner or camera"); enabled: false }
                }
                PMenu {
                    id: recentMenu
                    title: qsTr("Recent")
                    reserveIcon: false
                    enabled: bar.app.recentFiles.length > 0
                    // --- Archivos e impresión --- hasta 9 recientes, el primero es el último abierto o guardado.
                    Instantiator {
                        model: bar.app.recentFiles
                        delegate: PMenuItem {
                            required property string modelData
                            text: bar.app.recentDisplayName(modelData)
                            onTriggered: bar.requestOpenRecent(modelData)
                        }
                        onObjectAdded: (index, object) => recentMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => recentMenu.removeItem(object)
                    }
                }
                PMenuItem { text: qsTr("Save"); glyph: Icons.save; shortcutText: "Ctrl+S"; onTriggered: bar.requestSave() }
                PMenu {
                    title: qsTr("Save as")
                    reserveIcon: false
                    PMenuItem { text: qsTr("PNG picture"); onTriggered: bar.requestSaveAs("png") }
                    PMenuItem { text: qsTr("JPEG picture"); onTriggered: bar.requestSaveAs("jpg") }
                    PMenuItem { text: qsTr("BMP picture"); onTriggered: bar.requestSaveAs("bmp") }
                    PMenuItem { text: qsTr("GIF picture"); onTriggered: bar.requestSaveAs("gif") }
                    PMenuItem { text: qsTr("Other format"); shortcutText: "F12"; onTriggered: bar.requestSaveAs("") }
                }
                PMenuItem { text: qsTr("Save as project"); glyph: Icons.saveAs; shortcutText: "Ctrl+Shift+S"; onTriggered: bar.requestSaveAs("paint") }
                PMenuSeparator {}
                PMenu {
                    title: qsTr("Print")
                    reserveIcon: false
                    PMenuItem { text: qsTr("Print"); shortcutText: "Ctrl+P"; onTriggered: bar.requestPrint() }
                    PMenuItem { text: qsTr("Page setup"); onTriggered: bar.requestPageSetup() }
                    PMenuItem { text: qsTr("Print preview"); onTriggered: bar.requestPrintPreview() }
                }
                PMenuSeparator {}
                PMenu {
                    id: desktopMenu
                    title: qsTr("Set as desktop background")
                    reserveIcon: false
                    PMenuItem { text: qsTr("Fill"); onTriggered: bar.requestDesktopBackground("fill") }
                    PMenuItem { text: qsTr("Tile"); onTriggered: bar.requestDesktopBackground("tile") }
                    PMenuItem { text: qsTr("Center"); onTriggered: bar.requestDesktopBackground("center") }
                }
                PMenuSeparator { id: desktopSeparator }
                PMenuItem { text: qsTr("Image properties"); glyph: Icons.properties; shortcutText: "Ctrl+E"; onTriggered: bar.requestImageProperties() }
                PMenuSeparator {}
                PMenuItem { text: qsTr("Exit"); glyph: Icons.close; onTriggered: bar.requestExit() }
            }
        }
        MenuTitle {
            text: qsTr("Edit")
            menu: PMenu {
                PMenuItem { text: qsTr("Cut"); glyph: Icons.cut; shortcutText: "Ctrl+X"; enabled: bar.app.hasSelection; onTriggered: bar.app.cut() }
                PMenuItem { text: qsTr("Copy"); glyph: Icons.copy; shortcutText: "Ctrl+C"; enabled: bar.app.hasSelection; onTriggered: bar.app.copy() }
                PMenuItem { text: qsTr("Paste"); glyph: Icons.paste; shortcutText: "Ctrl+V"; onTriggered: bar.app.paste() }
                PMenuSeparator {}
                PMenuItem { text: qsTr("Copy visible layers"); glyph: Icons.copyLayers; shortcutText: "Ctrl+Shift+C"; onTriggered: bar.app.copyVisibleLayers() }
            }
        }
        MenuTitle {
            text: qsTr("View")
            menu: PMenu {
                PMenu {
                    title: qsTr("Zoom")
                    reserveIcon: false
                    PMenuItem { text: qsTr("100%"); shortcutText: "Ctrl+0"; onTriggered: bar.app.zoom100() }
                    PMenuItem { text: qsTr("Zoom in"); shortcutText: "Ctrl+Page Up"; onTriggered: bar.app.zoomIn() }
                    PMenuItem { text: qsTr("Zoom out"); shortcutText: "Ctrl+Page Down"; onTriggered: bar.app.zoomOut() }
                    PMenuItem { text: qsTr("Fit to window"); shortcutText: "Ctrl+1"; onTriggered: bar.requestFitToWindow() }
                }
                PMenuSeparator {}
                PMenuItem { text: qsTr("Rulers"); checkable: true; checked: bar.app.showRulers; shortcutText: "Ctrl+R"; onTriggered: bar.app.showRulers = !bar.app.showRulers }
                PMenuItem { text: qsTr("Gridlines"); checkable: true; checked: bar.app.showGridlines; shortcutText: "Ctrl+G"; onTriggered: bar.app.showGridlines = !bar.app.showGridlines }
                PMenuItem { text: qsTr("Status bar"); checkable: true; checked: bar.app.showStatusBar; onTriggered: bar.app.showStatusBar = !bar.app.showStatusBar }
                PMenuSeparator {}
                PMenuItem { text: qsTr("Always show toolbar"); checkable: true; checked: !bar.ribbonAutoHide; onTriggered: bar.requestRibbonMode(false) }
                PMenuItem { text: qsTr("Automatically hide toolbar"); checkable: true; checked: bar.ribbonAutoHide; onTriggered: bar.requestRibbonMode(true) }
                PMenuSeparator {}
                PMenuItem { text: qsTr("Full screen"); glyph: Icons.fullScreen; shortcutText: "F11"; onTriggered: bar.requestFullScreen() }
                PMenuItem { text: qsTr("Thumbnail"); glyph: Icons.thumbnail; onTriggered: bar.requestThumbnail() }
            }
        }

        VSep { Layout.leftMargin: 4; Layout.rightMargin: 4 }
        PButton { Layout.margins: 4; glyph: Icons.save; tip: qsTr("Save"); shortcutText: "Ctrl+S"; onClicked: bar.requestSave() }
        VSep { Layout.leftMargin: 4; Layout.rightMargin: 4 }
        PButton { Layout.margins: 4; glyph: Icons.undo; tip: qsTr("Undo"); shortcutText: "Ctrl+Z"; enabled: bar.app.canUndo; onClicked: bar.app.undo() }
        PButton { Layout.margins: 4; glyph: Icons.redo; tip: qsTr("Redo"); shortcutText: "Ctrl+Y"; enabled: bar.app.canRedo; onClicked: bar.app.redo() }
        VSep { Layout.leftMargin: 4; Layout.rightMargin: 4 }
        PButton {
            id: toggleRibbon
            Layout.leftMargin: 4; Layout.rightMargin: 4
            implicitWidth: toggleRow.implicitWidth + 16
            tip: text
            text: bar.ribbonVisible ? qsTr("Hide toolbar") : qsTr("Show toolbar")
            shortcutText: "Ctrl+T"
            contentItem: Item {
                RowLayout {
                    id: toggleRow
                    anchors.centerIn: parent
                    spacing: 8
                    Glyph { text: bar.ribbonVisible ? Icons.eyeOff : Icons.eye; size: 16 }
                    Text { text: toggleRibbon.text; font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; color: Theme.textPrimary }
                }
            }
            onClicked: bar.requestToggleRibbon()
        }
        Item { Layout.fillWidth: true }
        PButton { Layout.rightMargin: 4; glyph: Icons.settings; tip: qsTr("Open settings"); onClicked: bar.requestSettings() }
    }
}
