import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// «Image Properties».
// Ancho/alto en la unidad elegida (conversión con el DPI del documento). OK cambia el tamaño del lienzo (sin escalar)
// y guarda la unidad, que también usa la barra de estado.
WDialog {
    id: dlg
    required property AppController app
    property int unit: 0          // 0 Pixels, 1 Inches, 2 Centimeters
    property real dpi: 96
    property int pxW: 1
    property int pxH: 1
    property var info: ({})

    readonly property var decimalsFor: [0, 3, 2]
    readonly property var smallStepFor: [1, 0.1, 0.1]
    readonly property var largeStepFor: [1 * 10, 1, 1]

    title: qsTr("Image Properties")
    minContentWidth: 496
    primaryEnabled: wBox.valid && hBox.valid

    function toUnit(px) { return unit === 0 ? px : unit === 1 ? px / dpi : px / dpi * 2.54 }
    function toPx(v) { return Math.max(1, Math.round(unit === 0 ? v : unit === 1 ? v * dpi : v / 2.54 * dpi)) }
    function showValues() { wBox.setValue(toUnit(pxW)); hBox.setValue(toUnit(pxH)) }
    function setUnit(u) { if (u === unit) return; unit = u; showValues() }

    onAboutToShow: {
        info = app.imageFileInfo()
        dpi = app.dpi()
        unit = app.units
        unitRadios.itemAt(unit).checked = true
        pxW = app.canvasSize.width
        pxH = app.canvasSize.height
        showValues()
    }
    onAccepted: app.applyImageProperties(pxW, pxH, unit)

    component Label14: Text {
        font.family: Theme.uiFont
        font.pixelSize: Theme.bodySize
        color: Theme.textPrimary
        lineHeight: 20; lineHeightMode: Text.FixedHeight
        verticalAlignment: Text.AlignVCenter
    }

    // 1. File Attributes
    Label14 { Layout.topMargin: 16; text: qsTr("File Attributes"); font.weight: Font.DemiBold }
    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 0
        rowSpacing: 0
        uniformCellWidths: true
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; text: qsTr("Last Saved:") }
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; text: dlg.info.lastSaved ? dlg.info.lastSaved : qsTr("Not Available") }
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; text: qsTr("Size on disk:") }
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; text: dlg.info.sizeOnDisk ? dlg.info.sizeOnDisk : qsTr("Not Available") }
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; Layout.bottomMargin: 16; text: qsTr("Resolution:") }
        Label14 { Layout.fillWidth: true; Layout.topMargin: 16; Layout.bottomMargin: 16; text: qsTr("%1 DPI").arg(Math.round(dlg.dpi)) }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.systemChromeMedium }

    // 2. Units
    Label14 { Layout.topMargin: 16; Layout.bottomMargin: 16; text: qsTr("Units") }
    ButtonGroup { id: unitGroup }
    Repeater {
        id: unitRadios
        model: [qsTr("Pixels"), qsTr("Inches"), qsTr("Centimeters")]
        WRadioButton {
            required property int index
            required property string modelData
            text: modelData
            ButtonGroup.group: unitGroup
            onToggled: if (checked) dlg.setUnit(index)
        }
    }
    Item { implicitHeight: 16 }

    // 3. Image size
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.systemChromeMedium }
    GridLayout {
        Layout.fillWidth: true
        Layout.topMargin: 16
        columns: 3
        columnSpacing: 0
        rowSpacing: 4
        Label14 { Layout.topMargin: 16; text: qsTr("Width:") }
        Label14 { Layout.topMargin: 16; text: qsTr("Height:") }
        Item {}
        WNumberBox {
            id: wBox
            Layout.minimumWidth: 112
            Layout.fillWidth: true
            Layout.rightMargin: 28
            decimals: dlg.decimalsFor[dlg.unit]
            smallStep: dlg.smallStepFor[dlg.unit]
            largeStep: dlg.largeStepFor[dlg.unit]
            from: Math.pow(10, -decimals); to: dlg.toUnit(99999)
            Accessible.name: qsTr("Width:")
            onEdited: (v) => dlg.pxW = dlg.toPx(v)
        }
        WNumberBox {
            id: hBox
            Layout.minimumWidth: 112
            Layout.fillWidth: true
            Layout.rightMargin: 28
            decimals: wBox.decimals
            smallStep: wBox.smallStep
            largeStep: wBox.largeStep
            from: wBox.from; to: wBox.to
            Accessible.name: qsTr("Height:")
            onEdited: (v) => dlg.pxH = dlg.toPx(v)
        }
        WButton {
            Layout.minimumWidth: 98
            Layout.fillWidth: true
            text: qsTr("Default")
            onClicked: {
                const s = dlg.app.defaultImageSize()
                dlg.pxW = s.width; dlg.pxH = s.height
                dlg.showValues()
                Accessible.announce(qsTr("Dimensions set to default values."))
            }
        }
    }
}
