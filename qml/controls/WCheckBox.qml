import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// CheckBox  estilo Fluent: caja de 20, radio 4; marcada = relleno de acento con ✓.
CheckBox {
    id: c
    hoverEnabled: true
    spacing: 8
    padding: 0
    implicitHeight: 32
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    indicator: Rectangle {
        x: c.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: 20; height: 20
        radius: Theme.controlRadius
        color: c.checked ? (c.enabled ? (c.down ? Theme.accentPressed : c.hovered ? Theme.accentHover : Theme.accent) : Theme.accentDisabled)
             : (c.down ? Theme.pressedBg : c.hovered ? Theme.hoverBg : Theme.controlAltFill)
        border.width: c.checked ? 0 : 1
        border.color: c.enabled ? Theme.strongStroke : Theme.textDisabled
        Glyph {
            anchors.centerIn: parent
            visible: c.checked
            text: Icons.check
            size: 12
            color: Theme.textOnAccent
        }
    }
    contentItem: Text {
        leftPadding: c.indicator.width + c.spacing
        text: c.text
        font: c.font
        color: c.enabled ? Theme.textPrimary : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
