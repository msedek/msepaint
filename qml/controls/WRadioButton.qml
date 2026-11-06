import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// RadioButton  estilo Fluent: círculo de 20; marcado = relleno de acento con punto interior (12; 14 en hover, 10 presionado).
RadioButton {
    id: r
    hoverEnabled: true
    spacing: 8
    padding: 0
    implicitHeight: 32
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    indicator: Rectangle {
        x: r.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: 20; height: 20; radius: 10
        color: r.checked ? (r.enabled ? Theme.accent : Theme.accentDisabled)
             : (r.down ? Theme.pressedBg : r.hovered ? Theme.hoverBg : Theme.controlAltFill)
        border.width: r.checked ? 0 : 1
        border.color: r.enabled ? Theme.strongStroke : Theme.textDisabled
        Rectangle {
            readonly property int d: r.checked ? (r.down ? 10 : r.hovered ? 14 : 12) : (r.down ? 10 : 0)
            anchors.centerIn: parent
            width: d; height: d; radius: d / 2
            visible: d > 0
            color: r.checked ? Theme.textOnAccent : Theme.strongStroke
            Behavior on width { NumberAnimation { duration: 80 } }
            Behavior on height { NumberAnimation { duration: 80 } }
        }
    }
    contentItem: Text {
        leftPadding: r.indicator.width + r.spacing
        text: r.text
        font: r.font
        color: r.enabled ? Theme.textPrimary : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
