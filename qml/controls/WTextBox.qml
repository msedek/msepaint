import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// TextBox  estilo Fluent: alto 32, radio 4, padding 10,5,6,6; al enfocar, fondo de entrada activo y línea inferior de acento
// de 2 px. `error` pinta el borde con SystemFillColorCritical y muestra `errorText` como tooltip.
TextField {
    id: f
    property bool error: false
    property string errorText: ""
    property string suffix: ""   // texto fijo a la derecha (p. ej. «°»)

    implicitWidth: 120
    implicitHeight: 32
    leftPadding: 10
    rightPadding: 6 + (suffix !== "" ? sfx.implicitWidth + 4 : 0)
    topPadding: 5
    bottomPadding: 6
    hoverEnabled: true
    selectByMouse: true
    font.family: Theme.uiFont
    font.pixelSize: Theme.bodySize
    color: enabled ? Theme.textPrimary : Theme.textDisabled
    placeholderTextColor: Theme.textSecondary
    selectionColor: Theme.accent
    selectedTextColor: Theme.textOnAccent
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: Theme.controlRadius
        color: f.activeFocus ? Theme.inputActive : f.hovered ? Theme.hoverBg : Theme.controlFill
        border.width: 1
        border.color: f.error ? Theme.critical : Theme.controlStroke
        Rectangle { // línea inferior: fuerte en reposo, acento de 2 px con foco
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.leftMargin: 1; anchors.rightMargin: 1
            height: f.activeFocus || f.error ? 2 : 1
            radius: 1
            color: f.error ? Theme.critical : f.activeFocus ? Theme.accent : Theme.strongStroke
        }
        Text {
            id: sfx
            visible: f.suffix !== ""
            anchors.right: parent.right; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: f.suffix
            font: f.font
            color: Theme.textSecondary
        }
    }

    ToolTip.visible: error && errorText !== "" && (hovered || activeFocus)
    ToolTip.text: errorText
}
