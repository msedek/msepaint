import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Celda de paleta 24×24 con un círculo de 16; vacía si `swatch` es "".
AbstractButton {
    id: p
    property string swatch: ""
    signal picked()
    implicitWidth: 24; implicitHeight: 24
    hoverEnabled: swatch !== ""
    enabled: swatch !== ""
    focusPolicy: Qt.NoFocus
    background: Rectangle {
        radius: 12
        color: p.down ? Theme.pressedBg : p.hovered ? Theme.hoverBg : "transparent"
    }
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 16; height: 16; radius: 8
            color: p.swatch !== "" ? p.swatch : "transparent"
            border.width: 1
            border.color: Theme.textDisabled
            opacity: p.swatch !== "" ? 1 : 0.6
        }
    }
    onClicked: picked()
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: swatch.toUpperCase()
}
