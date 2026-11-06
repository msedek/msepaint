import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Menú flotante: radio 8, borde fino, sombra, ítems de 32 con margen 4.
Menu {
    id: m
    property bool reserveIcon: true // si algún ítem lleva ícono, se reserva la columna
    padding: 4
    margins: 0
    overlap: 0
    delegate: PMenuItem {}

    background: Rectangle {
        implicitWidth: 160
        color: Theme.flyoutBg
        radius: Theme.overlayRadius
        border.width: 1
        border.color: Theme.flyoutStroke
        layer.enabled: false
        Rectangle { // sombra simple
            z: -1
            anchors.fill: parent
            anchors.topMargin: 2
            anchors.leftMargin: -1
            anchors.rightMargin: -1
            anchors.bottomMargin: -4
            radius: parent.radius + 2
            color: "#14000000"
        }
    }
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize
}
