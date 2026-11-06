import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Desplegable de la cinta (Rotate, Flip, Shape outline/fill): alto 32, padding 8,4, ícono + chevron.
PButton {
    id: d
    property Menu menu: null
    property string label: ""
    implicitWidth: row.implicitWidth + 16
    implicitHeight: 32
    contentItem: Item {
        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: 6
            Item {
                Layout.preferredWidth: 16; Layout.preferredHeight: 16
                Glyph { anchors.centerIn: parent; visible: d.iconItem === null; text: d.glyph; size: 16; color: d.glyphColor }
                Loader { anchors.centerIn: parent; active: d.iconItem !== null; sourceComponent: d.iconItem }
            }
            Text {
                visible: d.label !== ""
                text: d.label
                font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
                color: Theme.textPrimary
            }
            Glyph { text: Icons.chevronDown; size: 10; color: Theme.textSecondary }
        }
    }
    onClicked: if (menu) menu.popup(d, 0, d.height + 2)
}
