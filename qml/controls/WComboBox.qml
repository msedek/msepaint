import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// ComboBox  estilo Fluent: como el botón estándar, chevron a la derecha y lista en flyout (radio 8).
ComboBox {
    id: cb
    implicitWidth: 120
    implicitHeight: 32
    leftPadding: 11
    rightPadding: 32
    hoverEnabled: true
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize

    background: Rectangle {
        radius: Theme.controlRadius
        color: cb.down ? Theme.pressedBg : cb.hovered ? Theme.hoverBg : Theme.controlFill
        border.width: 1
        border.color: Theme.controlStroke
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.leftMargin: 4; anchors.rightMargin: 4
            height: 1
            color: Theme.controlStrokeBottom
        }
    }
    contentItem: Text {
        text: cb.displayText
        font: cb.font
        color: cb.enabled ? Theme.textPrimary : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Glyph {
        x: cb.width - width - 11
        anchors.verticalCenter: parent.verticalCenter
        text: Icons.chevronDown
        size: 12
        color: Theme.textSecondary
    }
    delegate: ItemDelegate {
        id: d
        required property int index
        required property var modelData
        width: ListView.view ? ListView.view.width : 0
        height: 36
        hoverEnabled: true
        leftPadding: 12
        contentItem: Text {
            text: typeof d.modelData === "object" && cb.textRole ? d.modelData[cb.textRole] : d.modelData
            font: cb.font
            color: Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            anchors.fill: parent; anchors.margins: 2
            radius: Theme.controlRadius
            color: d.down ? Theme.subtlePressed : (d.hovered || cb.currentIndex === d.index) ? Theme.subtleHover : "transparent"
            Rectangle {
                visible: cb.currentIndex === d.index
                width: 3; height: 16; radius: 2
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.accentText
            }
        }
    }
    popup: Popup {
        y: cb.height + 2
        width: cb.width
        padding: 2
        implicitHeight: contentItem.implicitHeight + 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: cb.popup.visible ? cb.delegateModel : null
            currentIndex: cb.highlightedIndex
        }
        background: Rectangle {
            color: Theme.flyoutBg
            radius: Theme.overlayRadius
            border.width: 1
            border.color: Theme.flyoutStroke
        }
    }
}
