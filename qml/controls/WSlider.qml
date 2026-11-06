import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Slider  estilo Fluent: pista de 4 (ControlStrongFill) con tramo de acento y pulgar de 20 (aro claro, centro de acento de 12;
// 14 en hover, 10 presionado). `trackGradient` pinta la pista entera (p. ej. barra de valor del selector de color).
Slider {
    id: s
    property Gradient trackGradient: null
    hoverEnabled: true
    implicitWidth: orientation === Qt.Horizontal ? 200 : 32
    implicitHeight: orientation === Qt.Horizontal ? 32 : 200
    padding: 0

    background: Item {
        x: s.leftPadding; y: s.topPadding
        width: s.availableWidth; height: s.availableHeight
        Rectangle {
            id: track
            readonly property bool h: s.orientation === Qt.Horizontal
            readonly property int thick: s.trackGradient ? 12 : 4
            x: h ? 0 : (parent.width - width) / 2
            y: h ? (parent.height - height) / 2 : 0
            width: h ? parent.width : thick
            height: h ? thick : parent.height
            radius: thick / 2
            color: Theme.controlStrongFill
            gradient: s.trackGradient
            Rectangle {
                visible: !s.trackGradient
                width: track.h ? s.visualPosition * parent.width : parent.width
                height: track.h ? parent.height : (1 - s.visualPosition) * parent.height
                y: track.h ? 0 : s.visualPosition * parent.height
                radius: parent.radius
                color: s.enabled ? Theme.accent : Theme.accentDisabled
            }
        }
    }
    handle: Rectangle {
        readonly property bool h: s.orientation === Qt.Horizontal
        x: s.leftPadding + (h ? s.visualPosition * (s.availableWidth - width) : (s.availableWidth - width) / 2)
        y: s.topPadding + (h ? (s.availableHeight - height) / 2 : s.visualPosition * (s.availableHeight - height))
        width: 20; height: 20; radius: 10
        color: Theme.controlSolid
        border.width: 1
        border.color: Theme.controlStroke
        Rectangle {
            readonly property int d: s.pressed ? 10 : s.hovered ? 14 : 12
            anchors.centerIn: parent
            width: d; height: d; radius: d / 2
            color: s.enabled ? Theme.accent : Theme.accentDisabled
            Behavior on width { NumberAnimation { duration: 80 } }
            Behavior on height { NumberAnimation { duration: 80 } }
        }
    }
}
