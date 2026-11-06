import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Ítem de menú: columna de ícono de 16, texto 14, atajo 12 secundario a la derecha.
// `radio` = ítem seleccionable (indicador de acento 3×16 cuando está marcado); `checkable` normal = marca ✓.
MenuItem {
    id: it
    property string glyph: ""
    property string shortcutText: ""
    property bool radio: false
    property Component preview: null // muestra grande (pinceles, contorno, grosor)
    readonly property bool reserveIcon: menu && menu.reserveIcon !== undefined ? menu.reserveIcon : true

    implicitHeight: preview ? Math.max(40, implicitContentHeight + 8) : 32
    implicitWidth: Math.max(200, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: 11
    rightPadding: 11
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    indicator: Item {}
    arrow: Glyph {
        x: it.width - width - 8
        anchors.verticalCenter: parent.verticalCenter
        visible: it.subMenu
        text: Icons.chevronRight
        size: 12
        color: Theme.textSecondary
    }

    background: Rectangle {
        implicitHeight: 32
        radius: Theme.controlRadius
        color: it.down ? Theme.subtlePressed
             : it.highlighted ? Theme.subtleHover
             : (it.radio && it.checked) ? Theme.subtleHover : "transparent"
        Rectangle {
            visible: it.radio && it.checked
            width: 3; height: 16; radius: 2
            anchors.verticalCenter: parent.verticalCenter
            x: 0
            color: Theme.accentText
        }
    }

    contentItem: RowLayout {
        spacing: 12
        opacity: it.enabled ? 1 : 0.4
        Item {
            visible: it.reserveIcon && !it.preview
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            Glyph {
                anchors.centerIn: parent
                text: (!it.radio && it.checkable) ? (it.checked ? Icons.check : "") : it.glyph
                size: (!it.radio && it.checkable) ? 12 : 16
                color: Theme.textPrimary
            }
        }
        Loader {
            visible: it.preview !== null
            active: it.preview !== null
            sourceComponent: it.preview
            Layout.preferredWidth: 120
            Layout.preferredHeight: 40
        }
        Text {
            text: it.text
            font: it.font
            color: Theme.textPrimary
            Layout.fillWidth: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            visible: it.shortcutText !== ""
            text: it.shortcutText
            font.family: Theme.uiFont
            font.pixelSize: Theme.captionSize
            color: Theme.textSecondary
            Layout.leftMargin: 24
            Layout.rightMargin: it.subMenu ? 16 : 0
        }
        Item { visible: it.subMenu && it.shortcutText === ""; Layout.preferredWidth: 16 }
    }
}
