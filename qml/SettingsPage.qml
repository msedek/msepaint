import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Página de Ajustes a pantalla completa.
// Acerca de: nombre, versión y autor.
Rectangle {
    id: page
    required property AppController app
    signal back()

    color: Theme.windowBg
    focus: visible
    Keys.onEscapePressed: page.back()
    // Bloquea el mouse hacia lo que queda debajo.
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: (w) => w.accepted = false }

    readonly property bool twoColumns: width >= 960

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 24
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            id: content
            x: 56
            y: 24
            width: Math.min(1000, page.width - 112)
            implicitHeight: col.implicitHeight

            PButton {
                id: backButton
                x: -56; y: -24
                width: 48; height: 48
                glyph: Icons.back
                glyphSize: 16
                tip: qsTr("Back")
                Accessible.name: qsTr("Back")
                onClicked: page.back()
            }

            ColumnLayout {
                id: col
                width: parent.width
                spacing: 0

                Text {
                    Layout.bottomMargin: 24
                    text: qsTr("Settings")
                    font.family: Theme.uiFont; font.pixelSize: 20; font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: page.twoColumns ? 2 : 1
                    columnSpacing: 56
                    rowSpacing: 24

                    // Appearance
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 1
                        spacing: 0
                        Text {
                            Layout.bottomMargin: 4
                            text: qsTr("Appearance")
                            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        // Tarjeta desplegable (Expander) «App theme», expandida por defecto.
                        Rectangle {
                            id: card
                            property bool expanded: true
                            Layout.fillWidth: true
                            implicitHeight: header.height + (expanded ? options.implicitHeight + 1 : 0)
                            radius: Theme.controlRadius
                            color: Theme.cardBg
                            border.width: 1
                            border.color: Theme.cardStroke
                            clip: true

                            AbstractButton {
                                id: header
                                width: parent.width
                                height: 64
                                hoverEnabled: true
                                Accessible.name: qsTr("App theme")
                                onClicked: card.expanded = !card.expanded
                                background: Rectangle {
                                    radius: Theme.controlRadius
                                    color: header.down ? Theme.subtlePressed : header.hovered ? Theme.subtleHover : "transparent"
                                }
                                contentItem: RowLayout {
                                    spacing: 0
                                    Glyph { Layout.leftMargin: 16; text: Icons.appTheme; size: 16; color: Theme.textPrimary }
                                    Text {
                                        Layout.leftMargin: 12
                                        Layout.fillWidth: true
                                        text: qsTr("App theme")
                                        font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
                                        color: Theme.textPrimary
                                    }
                                    Glyph {
                                        Layout.rightMargin: 16
                                        text: card.expanded ? Icons.chevronUp : Icons.chevronDown
                                        size: 12
                                        color: Theme.textSecondary
                                    }
                                }
                            }
                            Rectangle { y: header.height; width: parent.width; height: 1; color: Theme.cardStroke; visible: card.expanded }
                            ColumnLayout {
                                id: options
                                y: header.height + 1
                                x: 16 + 36
                                spacing: 8
                                visible: card.expanded
                                Item { implicitHeight: 4 }
                                ButtonGroup { id: themeGroup }
                                WRadioButton {
                                    text: qsTr("Light")
                                    ButtonGroup.group: themeGroup
                                    checked: Theme.themeMode === "light"
                                    onToggled: if (checked) Theme.themeMode = "light"
                                }
                                WRadioButton {
                                    text: qsTr("Dark")
                                    ButtonGroup.group: themeGroup
                                    checked: Theme.themeMode === "dark"
                                    onToggled: if (checked) Theme.themeMode = "dark"
                                }
                                WRadioButton {
                                    text: qsTr("Use system setting")
                                    ButtonGroup.group: themeGroup
                                    checked: Theme.themeMode === "system"
                                    onToggled: if (checked) Theme.themeMode = "system"
                                }
                                Item { implicitHeight: 12 }
                            }
                        }
                    }

                    // About
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 1
                        Layout.bottomMargin: 24
                        spacing: 4
                        Text {
                            Layout.bottomMargin: 4
                            text: qsTr("About msepaint")
                            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
                            color: Theme.textPrimary
                        }
                        Repeater {
                            model: [qsTr("%1 %2").arg("msepaint").arg(page.app.appVersion), "© 2026 Msedek"]
                            TextEdit {
                                required property string modelData
                                text: modelData
                                readOnly: true
                                selectByMouse: true
                                font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
                                color: Theme.textPrimary
                                selectionColor: Theme.accent
                                selectedTextColor: Theme.textOnAccent
                            }
                        }
                    }
                }
            }
        }
    }
}
