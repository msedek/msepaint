import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Panel de capas. La capa de arriba se muestra primero.
Rectangle {
    id: panel
    required property AppController app
    signal requestBackgroundColor()

    implicitWidth: 110
    color: Theme.layerFill
    radius: Theme.overlayRadius
    border.width: 1
    border.color: Theme.cardStrokeSolid

    readonly property real thumbBox: 80
    function thumbSize(w, h) {
        const s = Math.min(thumbBox / w, thumbBox / h)
        return Qt.size(Math.max(1, Math.round(w * s)), Math.max(1, Math.round(h * s)))
    }

    component Checker: Item {
        clip: true
        Grid {
            columns: Math.ceil(parent.width / 8)
            Repeater {
                model: Math.ceil(parent.parent.width / 8) * Math.ceil(parent.parent.height / 8)
                Rectangle {
                    required property int index
                    readonly property int cols: Math.ceil(parent.parent.width / 8)
                    width: 8; height: 8
                    color: ((index % cols) + Math.floor(index / cols)) % 2 ? Theme.checkerB : Theme.checkerA
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 12
        spacing: 0

        PButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 4
            glyph: Icons.addCircle
            tip: qsTr("Create new layer"); shortcutText: "Ctrl+Shift+N"
            onClicked: panel.app.addLayer()
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12; Layout.rightMargin: 12
            topMargin: 6; bottomMargin: 6
            clip: true
            spacing: 0
            model: panel.app.layerCount
            boundsBehavior: Flickable.StopAtBounds
            delegate: Item {
                id: row
                required property int index
                readonly property int layerIndex: panel.app.layerCount - 1 - index
                readonly property bool active: panel.app.activeLayer === layerIndex
                readonly property bool shown: { panel.app.layersRevision; return panel.app.layerVisible(layerIndex) }
                readonly property size ts: panel.thumbSize(panel.app.canvasSize.width, panel.app.canvasSize.height)
                width: list.width
                height: ts.height + 12 + 8

                Rectangle {
                    id: card
                    anchors.centerIn: parent
                    width: row.ts.width + 8; height: row.ts.height + 8
                    radius: Theme.controlRadius
                    color: row.active ? Theme.subtleHover : hover.hovered ? Theme.subtleHover : "transparent"
                    border.width: row.active ? 2 : 0
                    border.color: Theme.accent

                    Checker { anchors.centerIn: parent; width: row.ts.width; height: row.ts.height }
                    Image {
                        anchors.centerIn: parent
                        width: row.ts.width; height: row.ts.height
                        sourceSize: row.ts
                        cache: false
                        asynchronous: false
                        source: "image://layers/" + row.layerIndex + "/" + panel.app.layersRevision
                    }
                    Rectangle { anchors.centerIn: parent; width: row.ts.width; height: row.ts.height; color: Theme.hiddenVeil; visible: !row.shown }

                    PButton {
                        anchors.top: parent.top; anchors.right: parent.right
                        anchors.topMargin: 4; anchors.rightMargin: 4
                        width: 24; height: 24
                        visible: hover.hovered || !row.shown
                        glyph: row.shown ? Icons.eye : Icons.eyeOff
                        glyphSize: 16
                        onClicked: panel.app.toggleLayerVisible(row.layerIndex)
                    }
                }
                HoverHandler { id: hover }
                TapHandler {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onTapped: (pt, button) => {
                        panel.app.activeLayer = row.layerIndex
                        if (button === Qt.RightButton) layerMenu.popup()
                    }
                }
                PMenu {
                    id: layerMenu
                    PMenuItem { text: row.shown ? qsTr("Hide layer") : qsTr("Show layer"); glyph: row.shown ? Icons.eyeOff : Icons.eye; shortcutText: "Ctrl+H"; onTriggered: panel.app.toggleLayerVisible(row.layerIndex) }
                    PMenuItem { text: qsTr("Duplicate layer"); glyph: Icons.duplicate; onTriggered: panel.app.duplicateLayer() }
                    PMenuItem { text: qsTr("Merge down"); glyph: Icons.merge; enabled: row.layerIndex > 0; onTriggered: panel.app.mergeDown() }
                    PMenuItem { text: qsTr("Move up"); glyph: Icons.arrowUp; shortcutText: "Ctrl+Shift+Page Up"; enabled: row.layerIndex < panel.app.layerCount - 1; onTriggered: panel.app.moveLayerUp() }
                    PMenuItem { text: qsTr("Move down"); glyph: Icons.arrowDown; shortcutText: "Ctrl+Shift+Page Down"; enabled: row.layerIndex > 0; onTriggered: panel.app.moveLayerDown() }
                    PMenuItem { text: qsTr("Delete layer"); glyph: Icons.trash; shortcutText: "Ctrl+Shift+Delete"; enabled: panel.app.layerCount > 1; onTriggered: panel.app.deleteLayer() }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        // Pie: color de fondo del lienzo.
        Item {
            Layout.fillWidth: true
            Layout.topMargin: 8
            implicitHeight: bgBox.height
            Rectangle {
                id: bgBox
                anchors.horizontalCenter: parent.horizontalCenter
                readonly property size ts: panel.thumbSize(panel.app.canvasSize.width, panel.app.canvasSize.height)
                width: ts.width + 24; height: ts.height + 24
                radius: Theme.controlRadius
                color: Theme.solidSecondary
                Checker { anchors.centerIn: parent; width: bgBox.ts.width; height: bgBox.ts.height; visible: !panel.app.backgroundVisible }
                Rectangle {
                    anchors.centerIn: parent
                    width: bgBox.ts.width; height: bgBox.ts.height
                    visible: panel.app.backgroundVisible
                    color: panel.app.backgroundColor
                    border.width: 1; border.color: Theme.cardStroke
                }
                PButton {
                    anchors.top: parent.top; anchors.right: parent.right
                    anchors.topMargin: 4; anchors.rightMargin: 4
                    width: 24; height: 24
                    visible: bgHover.hovered || !panel.app.backgroundVisible
                    glyph: panel.app.backgroundVisible ? Icons.eye : Icons.eyeOff
                    glyphSize: 16
                    onClicked: panel.app.toggleBackgroundVisible()
                }
                HoverHandler { id: bgHover }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: bgMenu.popup()
                }
                ToolTip.visible: bgHover.hovered
                ToolTip.delay: 600
                ToolTip.text: qsTr("Background")
                PMenu {
                    id: bgMenu
                    PMenuItem { text: panel.app.backgroundVisible ? qsTr("Hide background") : qsTr("Show background"); glyph: panel.app.backgroundVisible ? Icons.eyeOff : Icons.eye; shortcutText: "Ctrl+Shift+H"; onTriggered: panel.app.toggleBackgroundVisible() }
                    PMenuItem { text: qsTr("Change color"); glyph: Icons.colorWheel; shortcutText: "Ctrl+Shift+B"; onTriggered: panel.requestBackgroundColor() }
                }
            }
        }
    }
}
