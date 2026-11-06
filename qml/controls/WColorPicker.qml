import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Selector de color al estilo del ColorPicker  estilo Fluent en orientación horizontal, sin alfa (
// colores): espectro de matiz (x) y saturación (y) a la izquierda; a la derecha vista previa, barra de valor, selector
// RGB/HSV, campo hexadecimal y tres campos numéricos. Tamaño máximo 392×392.
Item {
    id: picker
    // Estado en HSV para no perder el matiz en los grises.
    property real hue: 0
    property real sat: 0
    property real val: 0
    readonly property color color: Qt.hsva(hue, sat, val, 1)
    property bool hsvMode: false
    property bool _syncing: false

    implicitWidth: 392
    implicitHeight: Math.max(spectrum.height, side.implicitHeight)

    function setColor(c) {
        const cc = Qt.color(c)
        if (cc.hsvHue >= 0) hue = cc.hsvHue
        sat = cc.hsvSaturation
        val = cc.hsvValue
    }
    function hex2(n) { return ("0" + Math.round(n).toString(16).toUpperCase()).slice(-2) }
    function hexOf(c) { return hex2(c.r * 255) + hex2(c.g * 255) + hex2(c.b * 255) }
    function sync() {
        _syncing = true
        const c = color
        if (!hexBox.activeFocus) hexBox.text = hexOf(c)
        const vals = hsvMode ? [Math.min(359, hue * 360), sat * 100, val * 100]
                             : [c.r * 255, c.g * 255, c.b * 255]
        for (let i = 0; i < 3; ++i) {
            const row = fields.itemAt(i)
            if (row && !row.box.activeFocus) row.box.setValue(Math.round(vals[i]))
        }
        _syncing = false
    }
    function fieldEdited(i, v) {
        if (_syncing) return
        if (hsvMode) {
            if (i === 0) hue = v / 360; else if (i === 1) sat = v / 100; else val = v / 100
        } else {
            const c = color
            const rgb = [c.r, c.g, c.b]
            rgb[i] = v / 255
            setColor(Qt.rgba(rgb[0], rgb[1], rgb[2], 1))
        }
    }
    onColorChanged: sync()
    onHsvModeChanged: sync()
    Component.onCompleted: sync()

    RowLayout {
        anchors.fill: parent
        spacing: 12

        // Espectro (caja HueSaturation al valor actual).
        Item {
            id: spectrum
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 192
            Layout.preferredHeight: 192
            width: 192; height: 192
            Rectangle {
                id: specBox
                anchors.fill: parent
                radius: Theme.controlRadius
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0 / 6; color: "#FF0000" }
                    GradientStop { position: 1 / 6; color: "#FFFF00" }
                    GradientStop { position: 2 / 6; color: "#00FF00" }
                    GradientStop { position: 3 / 6; color: "#00FFFF" }
                    GradientStop { position: 4 / 6; color: "#0000FF" }
                    GradientStop { position: 5 / 6; color: "#FF00FF" }
                    GradientStop { position: 6 / 6; color: "#FF0000" }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#00FFFFFF" }
                        GradientStop { position: 1; color: "#FFFFFFFF" }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "black"
                    opacity: 1 - picker.val
                }
                border.width: 1
                border.color: Theme.controlStroke
            }
            Rectangle { // marcador
                width: 16; height: 16; radius: 8
                x: picker.hue * spectrum.width - 8
                y: (1 - picker.sat) * spectrum.height - 8
                color: "transparent"
                border.width: 2
                border.color: picker.val > 0.5 && picker.sat < 0.5 ? "black" : "white"
            }
            MouseArea {
                anchors.fill: parent
                function pick(m) {
                    picker.hue = Math.max(0, Math.min(1, m.x / width))
                    picker.sat = Math.max(0, Math.min(1, 1 - m.y / height))
                }
                onPressed: (m) => pick(m)
                onPositionChanged: (m) => { if (pressed) pick(m) }
            }
            Accessible.role: Accessible.ColorChooser
            Accessible.name: qsTr("Color spectrum")
        }

        ColumnLayout {
            id: side
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            spacing: 8

            // Vista previa.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: Theme.controlRadius
                color: picker.color
                border.width: 1
                border.color: Theme.controlStroke
            }
            // Barra de valor (tercera dimensión).
            WSlider {
                Layout.fillWidth: true
                from: 0; to: 1
                value: picker.val
                onMoved: picker.val = value
                trackGradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: "black" }
                    GradientStop { position: 1; color: Qt.hsva(picker.hue, picker.sat, 1, 1) }
                }
                Accessible.name: qsTr("Value")
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                WComboBox {
                    Layout.preferredWidth: 84
                    model: ["RGB", "HSV"]
                    currentIndex: picker.hsvMode ? 1 : 0
                    onActivated: (i) => picker.hsvMode = i === 1
                }
                WTextBox {
                    id: hexBox
                    Layout.fillWidth: true
                    leftPadding: 22
                    maximumLength: 7
                    placeholderText: qsTr("Hex")
                    error: false
                    validator: RegularExpressionValidator { regularExpression: /#?[0-9A-Fa-f]{0,6}/ }
                    Text {
                        x: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "#"
                        font: hexBox.font
                        color: Theme.textSecondary
                    }
                    onTextEdited: {
                        const t = text.replace("#", "")
                        error = t.length !== 6
                        if (!error) picker.setColor("#" + t)
                    }
                    onEditingFinished: { error = false; text = picker.hexOf(picker.color) }
                    Accessible.name: qsTr("Hex")
                }
            }
            Repeater {
                id: fields
                model: 3
                RowLayout {
                    id: fieldRow
                    required property int index
                    property alias box: nb
                    Layout.fillWidth: true
                    spacing: 8
                    WNumberBox {
                        id: nb
                        Layout.preferredWidth: 84
                        from: 0
                        to: picker.hsvMode ? (fieldRow.index === 0 ? 359 : 100) : 255
                        largeStep: 10
                        errorText: qsTr("Please enter a number between %1 and %2.").arg(from).arg(to)
                        Accessible.name: lbl.text
                        onEdited: (v) => { if (valid) picker.fieldEdited(fieldRow.index, v) }
                    }
                    Text {
                        id: lbl
                        Layout.fillWidth: true
                        text: picker.hsvMode ? [qsTr("Hue"), qsTr("Saturation"), qsTr("Value")][fieldRow.index]
                                             : [qsTr("Red"), qsTr("Green"), qsTr("Blue")][fieldRow.index]
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.bodySize
                        color: Theme.textPrimary
                    }
                }
            }
        }
    }
}
