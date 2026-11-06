import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// «Resize and Skew».
WDialog {
    id: dlg
    required property AppController app
    property int origW: 1
    property int origH: 1
    property bool percent: true
    readonly property int maxVal: percent ? 10000 : 99999
    readonly property string rangeError: qsTr("Please enter a number between %1 and %2.").arg(1).arg(maxVal)

    title: qsTr("Resize and Skew")
    primaryEnabled: hBox.valid && vBox.valid && skewH.valid && skewV.valid

    function reset() {
        origW = Math.max(1, app.canvasSize.width)
        origH = Math.max(1, app.canvasSize.height)
        percent = true
        pctRadio.checked = true
        link.checked = true
        hBox.setValue(100); vBox.setValue(100)
        skewH.setValue(0); skewV.setValue(0)
    }
    function switchMode(toPercent) {
        if (toPercent === percent) return
        const h = hBox.value, v = vBox.value
        percent = toPercent
        if (toPercent) { hBox.setValue(Math.round(h * 100 / origW)); vBox.setValue(Math.round(v * 100 / origH)) }
        else { hBox.setValue(Math.round(origW * h / 100)); vBox.setValue(Math.round(origH * v / 100)) }
    }
    // Con la cadena activa, cambiar un campo recalcula el otro: round(otroInicial × nuevo / inicial).
    function follow(changed, other, fromOrig, toOrig) {
        if (!link.checked || !changed.valid) return
        other.setValue(percent ? changed.value : Math.max(1, Math.round(toOrig * changed.value / fromOrig)))
    }

    onAboutToShow: reset()
    onOpened: hBox.forceActiveFocus()
    onAccepted: {
        const w = percent ? Math.max(1, Math.round(origW * hBox.value / 100)) : hBox.value
        const h = percent ? Math.max(1, Math.round(origH * vBox.value / 100)) : vBox.value
        app.resizeAndSkew(w, h, skewH.value, skewV.value)
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: 22
        spacing: 16

        Text {
            text: qsTr("Resize")
            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
            color: Theme.textPrimary
        }
        RowLayout {
            spacing: 0
            ButtonGroup { id: modeGroup }
            WRadioButton { id: pctRadio; text: qsTr("Percentage"); checked: true; ButtonGroup.group: modeGroup; onToggled: if (checked) dlg.switchMode(true) }
            WRadioButton { Layout.leftMargin: 10; text: qsTr("Pixels"); ButtonGroup.group: modeGroup; onToggled: if (checked) dlg.switchMode(false) }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 6
            rowSpacing: 8
            Text { text: qsTr("Horizontal"); font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; color: Theme.textPrimary }
            Item {}
            Text { text: qsTr("Vertical"); font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; color: Theme.textPrimary }
            WNumberBox {
                id: hBox
                Layout.fillWidth: true
                from: 1; to: dlg.maxVal
                errorText: dlg.rangeError
                Accessible.name: qsTr("Horizontal")
                onEdited: dlg.follow(hBox, vBox, dlg.origW, dlg.origH)
            }
            WButton {
                id: link
                checkable: true
                checked: true
                implicitWidth: 28; implicitHeight: 28
                padding: 6
                glyph: Icons.link
                glyphSize: 12
                tip: qsTr("Maintain aspect ratio")
                Accessible.name: tip
                onToggled: if (checked) dlg.follow(hBox, vBox, dlg.origW, dlg.origH)
            }
            WNumberBox {
                id: vBox
                Layout.fillWidth: true
                from: 1; to: dlg.maxVal
                errorText: dlg.rangeError
                Accessible.name: qsTr("Vertical")
                onEdited: dlg.follow(vBox, hBox, dlg.origH, dlg.origW)
            }
        }

        Text {
            text: qsTr("Skew")
            font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold
            color: Theme.textPrimary
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 8
            rowSpacing: 8
            Text { text: qsTr("Horizontal"); font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; color: Theme.textPrimary }
            Text { text: qsTr("Vertical"); font.family: Theme.uiFont; font.pixelSize: Theme.bodySize; color: Theme.textPrimary }
            WNumberBox {
                id: skewH
                Layout.fillWidth: true
                from: -89; to: 89
                suffix: "°"
                errorText: qsTr("Please enter a number between %1 and %2.").arg(-89).arg(89)
                Accessible.name: qsTr("Horizontal")
            }
            WNumberBox {
                id: skewV
                Layout.fillWidth: true
                from: -89; to: 89
                suffix: "°"
                errorText: skewH.errorText
                Accessible.name: qsTr("Vertical")
            }
        }
    }
}
