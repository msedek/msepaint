import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Botón dividido seleccionable (Select, Brushes): 40 de ancho, parte superior con ícono 24,
// línea de 1 px y franja inferior de 16 con chevron de 8 que abre `menu`.
Item {
    id: root
    property string glyph: ""
    property Component iconItem: null
    property bool selected: false
    property string tip: ""
    property string shortcutText: ""
    property Menu menu: null
    signal activated()

    implicitWidth: 40
    implicitHeight: 64

    Rectangle { // borde de selección alrededor de todo el botón
        anchors.fill: parent
        radius: Theme.controlRadius
        color: root.selected ? Theme.selectedBg : "transparent"
        border.width: root.selected ? 1 : 0
        border.color: Theme.accent
        Rectangle {
            visible: root.selected
            anchors.fill: parent; anchors.margins: 1
            radius: Theme.controlRadius - 1
            color: "transparent"; border.width: 1; border.color: Theme.selectedInner
        }
    }

    PButton {
        id: top
        x: 0; y: 0
        width: 40; height: parent.height - 17
        radius: Theme.controlRadius
        glyph: root.glyph
        glyphSize: 24
        iconItem: root.iconItem
        tip: root.tip
        shortcutText: root.shortcutText
        onClicked: root.activated()
    }
    Rectangle { x: 4; width: 32; y: top.height; height: 1; color: Theme.divider }
    PButton {
        id: bottom
        x: 0; y: top.height + 1
        width: 40; height: 16
        radius: Theme.controlRadius
        glyph: Icons.chevronDown
        glyphSize: 10
        onClicked: if (root.menu) root.menu.popup(bottom, 0, bottom.height + 2)
    }
}
