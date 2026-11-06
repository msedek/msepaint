import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Botón plano de la cinta/barra de menús: transparente en reposo, hover y presionado sutiles,
// y estado "seleccionado" con doble borde (exterior acento, interior claro).
AbstractButton {
    id: b
    property string glyph: ""
    property int glyphSize: 16
    property color glyphColor: enabled ? Theme.iconBrush : Theme.textDisabled
    property bool selected: false
    property string tip: ""
    property string shortcutText: ""
    property int radius: Theme.controlRadius
    property Component iconItem: null // contenido gráfico alternativo al glifo

    implicitWidth: 32
    implicitHeight: 32
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: b.radius
        color: b.down ? Theme.pressedBg : b.hovered ? Theme.hoverBg : b.selected ? Theme.selectedBg : "transparent"
        border.width: b.selected ? 1 : 0
        border.color: Theme.accent
        Rectangle {
            visible: b.selected
            anchors.fill: parent
            anchors.margins: 1
            radius: b.radius - 1
            color: "transparent"
            border.width: 1
            border.color: Theme.selectedInner
        }
    }

    contentItem: Item {
        Glyph {
            anchors.centerIn: parent
            visible: b.iconItem === null && b.glyph !== ""
            text: b.glyph
            size: b.glyphSize
            color: b.down ? Theme.textSecondary : b.glyphColor
        }
        Loader {
            anchors.centerIn: parent
            active: b.iconItem !== null
            sourceComponent: b.iconItem
        }
    }

    ToolTip.visible: hovered && tip !== ""
    ToolTip.delay: 600
    ToolTip.text: shortcutText !== "" ? tip + " (" + shortcutText + ")" : tip
}
