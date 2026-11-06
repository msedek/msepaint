import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Barra de texto flotante bajo la cinta mientras se edita un cuadro de texto.
Rectangle {
    id: bar
    required property AppController app
    property real windowWidth: 1280
    readonly property var sizes: [8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72, 96, 128, 172, 256]

    implicitHeight: 48
    implicitWidth: row.implicitWidth + 16
    radius: 7
    color: Theme.acrylic
    border.width: 1
    border.color: Theme.cardStrokeSolid

    component FlatCombo: ComboBox {
        id: c
        implicitHeight: 32
        font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
        background: Rectangle {
            radius: Theme.controlRadius
            color: c.hovered ? Theme.subtleHover : Theme.controlFill
            border.width: 1; border.color: Theme.controlStroke
        }
        indicator: Glyph { x: c.width - width - 10; anchors.verticalCenter: parent.verticalCenter; text: Icons.chevronDown; size: 10; color: Theme.textSecondary }
        popup.background: Rectangle { color: Theme.flyoutBg; radius: Theme.overlayRadius; border.width: 1; border.color: Theme.flyoutStroke }
    }
    component Toggle: PButton {
        id: tg
        property string letter
        property bool on: false
        property bool fb: false
        property bool fi: false
        property bool fu: false
        property bool fs: false
        selected: on
        contentItem: Text {
            text: tg.letter
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            font.family: Theme.uiFont; font.pixelSize: 16
            font.bold: tg.fb; font.italic: tg.fi; font.underline: tg.fu; font.strikeout: tg.fs
            color: Theme.textPrimary
        }
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 8

        FlatCombo {
            id: family
            Layout.preferredWidth: bar.windowWidth >= 810 ? 190 : 126
            model: bar.app.fontFamilies()
            currentIndex: model.indexOf(bar.app.textFamily)
            onActivated: (i) => bar.app.textFamily = model[i]
            ToolTip.visible: hovered; ToolTip.delay: 600; ToolTip.text: qsTr("Font family")
        }
        FlatCombo {
            Layout.preferredWidth: 72
            editable: true
            model: bar.sizes
            editText: bar.app.textSize
            onActivated: (i) => bar.app.textSize = bar.sizes[i]
            onAccepted: { const v = parseFloat(editText); if (!isNaN(v)) bar.app.textSize = v }
            ToolTip.visible: hovered; ToolTip.delay: 600; ToolTip.text: qsTr("Font size")
        }
        VSep {}
        Toggle { letter: "B"; fb: true; on: bar.app.textBold; tip: qsTr("Bold"); shortcutText: "Ctrl+B"; onClicked: bar.app.textBold = !bar.app.textBold }
        Toggle { letter: "I"; fi: true; on: bar.app.textItalic; tip: qsTr("Italic"); shortcutText: "Ctrl+I"; onClicked: bar.app.textItalic = !bar.app.textItalic }
        Toggle { letter: "U"; fu: true; on: bar.app.textUnderline; tip: qsTr("Underline"); shortcutText: "Ctrl+U"; onClicked: bar.app.textUnderline = !bar.app.textUnderline }
        Toggle { letter: "S"; fs: true; on: bar.app.textStrike; tip: qsTr("Strikethrough"); onClicked: bar.app.textStrike = !bar.app.textStrike }
        VSep {}
        PButton { glyph: Icons.alignLeft; selected: bar.app.textAlign === 0; tip: qsTr("Left align"); onClicked: bar.app.textAlign = 0 }
        PButton { glyph: Icons.alignCenter; selected: bar.app.textAlign === 1; tip: qsTr("Center align"); onClicked: bar.app.textAlign = 1 }
        PButton { glyph: Icons.alignRight; selected: bar.app.textAlign === 2; tip: qsTr("Right align"); onClicked: bar.app.textAlign = 2 }
        VSep {}
        CheckBox {
            id: fillBox
            text: qsTr("Background fill")
            checked: bar.app.textFill
            onToggled: bar.app.textFill = checked
            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize
            focusPolicy: Qt.NoFocus
            indicator: Rectangle {
                x: fillBox.leftPadding; anchors.verticalCenter: parent.verticalCenter
                width: 20; height: 20; radius: 4
                color: fillBox.checked ? Theme.accent : Theme.controlFill
                border.width: fillBox.checked ? 0 : 1; border.color: Theme.strongStroke
                Glyph { anchors.centerIn: parent; visible: fillBox.checked; text: Icons.check; size: 12; color: Theme.textOnAccent }
            }
            contentItem: Text {
                leftPadding: fillBox.indicator.width + 8
                text: fillBox.text; font: fillBox.font; color: Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Shortcut { sequence: "Ctrl+B"; enabled: bar.visible; onActivated: bar.app.textBold = !bar.app.textBold }
    Shortcut { sequence: "Ctrl+I"; enabled: bar.visible; onActivated: bar.app.textItalic = !bar.app.textItalic }
    Shortcut { sequence: "Ctrl+U"; enabled: bar.visible; onActivated: bar.app.textUnderline = !bar.app.textUnderline }
}
