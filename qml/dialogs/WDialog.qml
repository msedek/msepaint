import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Marco común de diálogo modal:
// velo SmokeFill, caja centrada (ancho 320–548, alto mín. 184), radio 8, borde SurfaceStroke y sombra;
// zona superior LayerFillColorAlt con padding 24 y título 20 SemiBold; zona de botones con borde superior, padding 24
// y botones de igual ancho separados 8. Enter = principal, Esc = cerrar/cancelar.
// El contenido se declara como hijos (columna con el ancho del contenido).
Popup {
    id: dlg
    property string title: ""
    property string primaryText: qsTr("OK")
    property string secondaryText: ""
    property string closeText: qsTr("Cancel")
    property bool primaryEnabled: true
    property int minContentWidth: 0
    default property alias content: body.data

    signal accepted()
    signal secondaryClicked()
    signal rejected()

    function accept() { if (!primaryEnabled) return; dlg.accepted(); dlg._result = 1; dlg.close() }
    function reject() { dlg._result = 0; dlg.close() }
    property int _result: 0
    onAboutToShow: _result = 0
    onClosed: if (_result === 0) rejected()

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 0

    Overlay.modal: Rectangle { color: Theme.smoke }

    background: Item {
        // Sombra simple (sin efectos de shader: el backend de software no los dibuja).
        Repeater {
            model: 4
            Rectangle {
                required property int index
                anchors.fill: parent
                anchors.margins: -(index + 1) * 2
                anchors.topMargin: -(index + 1) * 2 + 8
                radius: Theme.overlayRadius + (index + 1) * 2
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(0, 0, 0, 0.05 - index * 0.01)
            }
        }
        Rectangle {
            anchors.fill: parent
            radius: Theme.overlayRadius
            color: Theme.windowBg
            border.width: 1
            border.color: Theme.surfaceStroke
        }
    }

    // Un Item envuelve la columna: el tamaño implícito de un Layout lo recalcula el propio Layout.
    contentItem: Item {
        implicitWidth: Math.min(548, Math.max(320, Math.max(dlg.minContentWidth, body.implicitWidth) + 50))
        implicitHeight: Math.max(184, top.implicitHeight + bottom.implicitHeight + 2)
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Zona superior: título y contenido.
        Rectangle {
            id: top
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 1
            Layout.bottomMargin: 0
            implicitHeight: topCol.implicitHeight + 48
            color: Theme.layerAlt
            radius: Theme.overlayRadius - 1
            Rectangle { // esquinas inferiores rectas
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: parent.radius
                color: parent.color
            }
            ColumnLayout {
                id: topCol
                anchors.fill: parent
                anchors.margins: 24
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 12
                    text: dlg.title
                    visible: text !== ""
                    font.family: Theme.uiFont
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                    wrapMode: Text.Wrap
                }
                ColumnLayout {
                    id: body
                    Layout.fillWidth: true
                    spacing: 0
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Zona de botones.
        Rectangle {
            id: bottom
            Layout.fillWidth: true
            Layout.margins: 1
            Layout.topMargin: 0
            implicitHeight: buttons.implicitHeight + 48
            color: Theme.windowBg
            radius: Theme.overlayRadius - 1
            Rectangle { // esquinas superiores rectas y borde superior
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                height: parent.radius
                color: parent.color
            }
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                height: 1
                color: Theme.cardStroke
            }
            RowLayout {
                id: buttons
                anchors.fill: parent
                anchors.margins: 24
                spacing: 8
                WButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    accent: true
                    text: dlg.primaryText
                    visible: text !== ""
                    enabled: dlg.primaryEnabled
                    onClicked: dlg.accept()
                }
                WButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    text: dlg.secondaryText
                    visible: text !== ""
                    onClicked: { dlg._result = 2; dlg.secondaryClicked(); dlg.close() }
                }
                WButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    text: dlg.closeText
                    visible: text !== ""
                    onClicked: dlg.reject()
                }
            }
        }

    }
        Shortcut {
            sequences: ["Return", "Enter"]
            enabled: dlg.opened
            onActivated: dlg.accept()
        }
    }
}
