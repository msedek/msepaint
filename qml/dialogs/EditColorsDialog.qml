import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// «Edit colors» y su variante «Background color».
// OK asigna el color al Color activo (o al fondo si `forBackground`) y lo agrega a los colores personalizados de la cinta.
WDialog {
    id: dlg
    required property AppController app
    property bool forBackground: false
    // Colores personalizados del diálogo (sesión; se agregan al principio).
    property var customColors: []

    // Los 48 colores básicos (4 filas de 12).
    readonly property var basicColors: [
        "#F08784", "#EB3324", "#774342", "#8E403A", "#3A0603", "#9FFCFD", "#73FBFD", "#3282F6", "#0023F5", "#00129A", "#16417C", "#000C7B",
        "#FFFE91", "#FFFD55", "#F09B59", "#F08650", "#784315", "#817F26", "#7E84F7", "#732BF5", "#3580BB", "#00023D", "#58135E", "#3A083E",
        "#A1FB8E", "#A1FA4F", "#75F94D", "#75FA61", "#75FA8D", "#818049", "#EF88BE", "#EE8AF8", "#EA3FF7", "#EA3680", "#7F82BB", "#75163F",
        "#377D22", "#377E47", "#367E7F", "#507F80", "#183E0C", "#173F3F", "#741B7C", "#39107B", "#000000", "#808080", "#C0C0C0", "#FFFFFF"]

    title: forBackground ? qsTr("Background color") : qsTr("Edit colors")

    function openFor(isBackground) {
        forBackground = isBackground
        open()
    }

    onAboutToShow: picker.setColor(forBackground ? app.backgroundColor : (app.activeColor === 1 ? app.color1 : app.color2))
    onAccepted: {
        const c = picker.color
        if (forBackground) app.setBackgroundColor(c)
        else app.pickPaletteColor(c)
        app.addCustomColor(c)
    }

    component Swatch: AbstractButton {
        id: sw
        property color swatch: "transparent"
        implicitWidth: 24; implicitHeight: 24
        hoverEnabled: true
        background: Rectangle {
            anchors.centerIn: parent
            width: 20; height: 20; radius: 10
            color: sw.swatch
            border.width: 1
            border.color: Theme.textDisabled
            Rectangle {
                visible: sw.hovered || sw.visualFocus
                anchors.fill: parent; anchors.margins: -2
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: Theme.textSecondary
            }
        }
        onClicked: picker.setColor(swatch)
        ToolTip.visible: hovered
        ToolTip.delay: 600
        ToolTip.text: String(swatch).toUpperCase()
    }

    WColorPicker {
        id: picker
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 392
        Layout.bottomMargin: 24
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: 24

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            spacing: 0
            Text {
                Layout.bottomMargin: 16
                Layout.preferredHeight: 28
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Basic colors")
                font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
                color: Theme.textPrimary
            }
            Grid {
                columns: 12
                Repeater {
                    model: dlg.basicColors
                    Swatch { required property string modelData; swatch: modelData }
                }
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            spacing: 0
            RowLayout {
                Layout.bottomMargin: 16
                Layout.preferredWidth: 6 * 24
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Custom colors")
                    font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }
                PButton {
                    implicitWidth: 28; implicitHeight: 28
                    glyph: Icons.plus
                    glyphSize: 16
                    tip: qsTr("Add color")
                    Accessible.name: tip
                    onClicked: {
                        dlg.customColors = [picker.color.toString()].concat(dlg.customColors)
                        Accessible.announce(qsTr("Color added"))
                    }
                }
            }
            Grid {
                columns: 6
                Repeater {
                    model: dlg.customColors
                    Swatch {
                        id: cs
                        required property string modelData
                        required property int index
                        swatch: modelData
                        // Aparece con 80 ms: opacidad 0→1 y escala 0,85→1.
                        opacity: 0; scale: 0.85
                        Component.onCompleted: if (index === 0) appear.start(); else { opacity = 1; scale = 1 }
                        ParallelAnimation {
                            id: appear
                            NumberAnimation { target: cs; property: "opacity"; to: 1; duration: 80 }
                            NumberAnimation { target: cs; property: "scale"; to: 1; duration: 80 }
                        }
                    }
                }
            }
        }
    }
}
