import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Botón estándar  estilo Fluent (diálogos y Ajustes): alto 32, radio 4, relleno ControlFillColorDefault y borde con la
// línea inferior más marcada. `accent` = botón principal (fondo de acento, texto blanco en claro / negro en oscuro).
// Si es `checkable`, al quedar marcado se dibuja como acento (ToggleButton  estilo Fluent).
Button {
    id: b
    property bool accent: false
    property string glyph: ""
    property int glyphSize: 16
    property string tip: ""
    readonly property bool accentLook: accent || (checkable && checked)

    implicitWidth: Math.max(text !== "" ? 120 : 32, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 32
    leftPadding: text !== "" ? 11 : 8
    rightPadding: leftPadding
    topPadding: 5
    bottomPadding: 6
    hoverEnabled: true
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    background: Rectangle {
        radius: Theme.controlRadius
        color: !b.enabled ? (b.accentLook ? Theme.accentDisabled : Theme.controlFill)
             : b.accentLook ? (b.down ? Theme.accentPressed : b.hovered ? Theme.accentHover : Theme.accent)
             : (b.down ? Theme.pressedBg : b.hovered ? Theme.hoverBg : Theme.controlFill)
        border.width: b.accentLook ? 0 : 1
        border.color: Theme.controlStroke
        // Línea inferior más marcada (ControlStrokeColorSecondary / borde de elevación de acento).
        Rectangle {
            visible: b.enabled && !b.down
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.leftMargin: parent.radius; anchors.rightMargin: parent.radius
            height: 1
            color: b.accentLook ? "#33000000" : Theme.controlStrokeBottom
        }
        Rectangle { // foco de teclado
            visible: b.visualFocus
            anchors.fill: parent; anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 2; border.color: Theme.textPrimary
        }
    }

    contentItem: Item {
        readonly property color fg: b.accentLook ? (b.enabled ? Theme.textOnAccent : Theme.textOnAccentDisabled)
                                    : !b.enabled ? Theme.textDisabled : b.down ? Theme.textSecondary : Theme.textPrimary
        implicitWidth: b.text === "" ? g.implicitWidth : label.implicitWidth
        implicitHeight: 20
        Glyph { id: g; anchors.centerIn: parent; visible: b.glyph !== "" && b.text === ""; text: b.glyph; size: b.glyphSize; color: parent.fg }
        Text {
            id: label
            anchors.centerIn: parent
            visible: b.text !== ""
            text: b.text
            font: b.font
            color: parent.fg
            elide: Text.ElideRight
            width: Math.min(implicitWidth, parent.width)
            horizontalAlignment: Text.AlignHCenter
        }
    }

    ToolTip.visible: hovered && tip !== ""
    ToolTip.delay: 600
    ToolTip.text: tip
}
