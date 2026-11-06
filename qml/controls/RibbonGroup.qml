import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// Grupo de la cinta: padding 12,4; contenido (alto mín. 72, centrado) y etiqueta 12 secundaria debajo.
// `collapsed` lo reemplaza por un botón con el ícono del grupo que abre el contenido en un flyout.
Item {
    id: g
    property string label: ""
    property bool dividerRight: true
    property bool dividerLeft: false
    property bool collapsed: false
    property string collapsedGlyph: ""
    default property alias content: holder.data
    readonly property real fullWidth: Math.max(holder.childrenRect.width, Math.min(100, labelText.implicitWidth)) + 24

    implicitWidth: collapsed ? Math.max(collapsedButton.width, Math.min(100, labelText.implicitWidth)) + 24 : fullWidth
    implicitHeight: 72 + 4 + 16 + 8

    Item {
        id: slot
        x: 12; y: 4
        width: g.width - 24
        height: 72
        Item {
            id: holder
            parent: g.collapsed ? flyoutBox : slot
            anchors.centerIn: parent
            width: childrenRect.width
            height: childrenRect.height
        }
        PButton {
            id: collapsedButton
            visible: g.collapsed
            anchors.centerIn: parent
            width: 48; height: 56
            tip: g.label
            contentItem: Column {
                spacing: 4
                Glyph { anchors.horizontalCenter: parent.horizontalCenter; text: g.collapsedGlyph; size: 24 }
                Glyph { anchors.horizontalCenter: parent.horizontalCenter; text: Icons.chevronDown; size: 8; color: Theme.textSecondary }
            }
            onClicked: flyout.open()
        }
    }

    Text {
        id: labelText
        x: 12; width: g.width - 24
        y: 4 + 72 + 4
        text: g.label
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        font.family: Theme.uiFont; font.pixelSize: Theme.captionSize
        color: Theme.textSecondary
    }

    Rectangle { visible: g.dividerRight; anchors.right: parent.right; y: 8; width: 1; height: parent.height - 16; color: Theme.divider }
    Rectangle { visible: g.dividerLeft; anchors.left: parent.left; y: 8; width: 1; height: parent.height - 16; color: Theme.divider }

    Popup {
        id: flyout
        y: collapsedButton.y + collapsedButton.height + 8
        x: 12 + (slot.width - width) / 2
        padding: 4
        background: Rectangle {
            color: Theme.flyoutBg; radius: Theme.controlRadius
            border.width: 1; border.color: Theme.cardStrokeSolid
        }
        contentItem: Column {
            spacing: 4
            Item {
                id: flyoutBox
                width: Math.max(holder.width + 16, 60)
                height: Math.max(holder.height + 8, 72)
            }
            Text {
                width: flyoutBox.width
                text: g.label
                horizontalAlignment: Text.AlignHCenter
                font.family: Theme.uiFont; font.pixelSize: Theme.captionSize
                color: Theme.textSecondary
            }
        }
    }
}
