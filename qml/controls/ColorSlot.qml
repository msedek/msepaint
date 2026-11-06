import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Botón redondo Color 1 / Color 2: alto 34, fondo redondo de 32 en hover,
// círculo de color de 24 con borde fino y anillo de acento de 32 cuando es el activo.
AbstractButton {
    id: c
    required property int which
    required property AppController app
    readonly property color swatch: which === 1 ? app.color1 : app.color2
    readonly property bool active: app.activeColor === which
    implicitWidth: 34; implicitHeight: 34
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    background: Item {}
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 32; height: 32; radius: 16
            color: c.down ? Theme.pressedBg : c.hovered ? Theme.hoverBg : "transparent"
            border.width: c.active ? 1 : 0
            border.color: Theme.accent
        }
        Rectangle {
            anchors.centerIn: parent
            width: 24; height: 24; radius: 12
            color: c.swatch
            border.width: 1
            border.color: Theme.textDisabled
        }
    }
    onClicked: app.activeColor = which
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: (which === 1 ? qsTr("Color 1") : qsTr("Color 2")) + ": " + swatch.toString().toUpperCase()
}
