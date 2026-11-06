import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes
import MsePaint

// Fila 3: barra de estado.
Rectangle {
    id: sb
    required property AppController app
    property real windowWidth: width
    signal requestFitToWindow()

    implicitHeight: 37
    color: "transparent"
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.cardStrokeSolid }

    readonly property var zoomLevels: [10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0.75, 0.5, 0.25, 0.125]

    // --- Diálogos de imagen --- Unidad elegida en Propiedades de imagen: px enteros con «px» pegado; pulgadas y
    // centímetros con 2 decimales y « in» / « cm».
    function fmtPair(a, b, sep) {
        const u = sb.app.units
        if (u === 0) return a + sep + b + "px"
        const k = u === 1 ? 1 / sb.app.dpi() : 2.54 / sb.app.dpi()
        return (a * k).toFixed(2) + sep + (b * k).toFixed(2) + (u === 1 ? " in" : " cm")
    }

    component StatusIcon: Shape {
        property string kind
        width: 12; height: 12
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.iconBrush; strokeWidth: 1; fillColor: "transparent"
            strokeStyle: kind === "selection" ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [2, 2]
            PathSvg {
                path: kind === "cursor" ? "M 2.5 1.5 L 2.5 10.5 L 5 8 L 7 11.5 L 8.5 10.8 L 6.6 7.5 L 10 7.5 Z"
                    : kind === "selection" ? "M 1.5 2.5 L 10.5 2.5 L 10.5 9.5 L 1.5 9.5 Z"
                    : "M 1.5 1.5 L 10.5 1.5 L 10.5 10.5 L 1.5 10.5 Z M 7.5 10.5 L 10.5 7.5"
            }
        }
    }
    component StatusText: Text {
        font.family: Theme.uiFont; font.pixelSize: Theme.captionSize
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        Layout.rightMargin: 8
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 1
        spacing: 0

        StatusIcon { kind: "cursor"; Layout.leftMargin: 12; Layout.rightMargin: 8 }
        StatusText {
            Layout.minimumWidth: 100
            text: sb.app.cursorInside ? sb.fmtPair(sb.app.cursorPos.x, sb.app.cursorPos.y, ", ") : ""
        }

        VSep { visible: sb.windowWidth >= 864 }
        StatusIcon { kind: "selection"; visible: sb.windowWidth >= 864; Layout.leftMargin: 12; Layout.rightMargin: 8 }
        StatusText {
            visible: sb.windowWidth >= 864
            Layout.minimumWidth: 100
            text: sb.app.hasSelection ? sb.fmtPair(sb.app.selectionSize.width, sb.app.selectionSize.height, " × ") : ""
        }

        VSep { visible: sb.windowWidth >= 972 }
        StatusIcon { kind: "canvas"; visible: sb.windowWidth >= 972; Layout.leftMargin: 12; Layout.rightMargin: 8 }
        StatusText {
            visible: sb.windowWidth >= 972
            Layout.minimumWidth: 100
            text: sb.fmtPair(sb.app.canvasSize.width, sb.app.canvasSize.height, " × ")
        }

        VSep { visible: sb.windowWidth >= 1156 }
        Glyph { visible: sb.windowWidth >= 1156; text: Icons.save; size: 16; Layout.leftMargin: 16; Layout.rightMargin: 8 }
        StatusText {
            visible: sb.windowWidth >= 1156
            text: sb.app.fileSizeText !== "" ? qsTr("Size: ") + sb.app.fileSizeText : ""
        }

        Item { Layout.fillWidth: true }

        // Control de zoom
        PButton {
            Layout.margins: 4
            implicitWidth: 32; implicitHeight: 24
            radius: 2
            glyph: Icons.fitWindow
            tip: qsTr("Fit to window"); shortcutText: "Ctrl+1"
            onClicked: sb.requestFitToWindow()
        }
        ComboBox {
            id: zoomCombo
            Layout.margins: 4
            implicitWidth: 92; implicitHeight: 29
            font.family: Theme.uiFont; font.pixelSize: Theme.captionSize
            model: sb.zoomLevels.map(z => (z * 100) + "%")
            onActivated: (i) => sb.app.zoom = sb.zoomLevels[i]
            background: Rectangle {
                radius: Theme.controlRadius
                color: Theme.controlFill
                border.width: 1; border.color: Theme.controlStroke
            }
            contentItem: TextField {
                id: zoomField
                text: Math.round(sb.app.zoom * 100) + "%"
                font: zoomCombo.font
                color: Theme.textPrimary
                leftPadding: 11; rightPadding: 30
                verticalAlignment: Text.AlignVCenter
                selectByMouse: true
                background: Item {}
                onEditingFinished: {
                    const v = parseFloat(text)
                    if (!isNaN(v)) sb.app.zoom = v / 100
                    text = Qt.binding(() => Math.round(sb.app.zoom * 100) + "%")
                }
            }
            indicator: Glyph { x: zoomCombo.width - width - 10; anchors.verticalCenter: parent.verticalCenter; text: Icons.chevronDown; size: 10; color: Theme.textSecondary }
        }
        PButton {
            implicitWidth: 28; implicitHeight: 24; radius: 2
            glyph: Icons.zoomOut; glyphSize: 14
            enabled: sb.app.zoom > 0.1 + 1e-6
            tip: qsTr("Zoom out"); shortcutText: "Ctrl+Page Down"
            onClicked: sb.app.zoomOut()
        }
        Slider {
            id: zoomSlider
            Layout.preferredWidth: 130
            from: 10; to: 1000; stepSize: 1
            value: sb.app.zoom * 100
            onMoved: sb.app.zoom = value / 100
            background: Rectangle {
                x: zoomSlider.leftPadding; y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - 2
                width: zoomSlider.availableWidth; height: 4; radius: 2
                color: Theme.strongStroke
                Rectangle { width: zoomSlider.visualPosition * parent.width; height: 4; radius: 2; color: Theme.accent }
            }
            handle: Rectangle {
                x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                width: 20; height: 20; radius: 10
                color: Theme.controlSolid
                border.width: 1; border.color: Theme.controlStroke
                Rectangle { anchors.centerIn: parent; width: zoomSlider.pressed ? 10 : 12; height: width; radius: width / 2; color: Theme.accent }
            }
            ToolTip.visible: pressed
            ToolTip.text: Math.round(value) + "%"
        }
        PButton {
            Layout.rightMargin: 8
            implicitWidth: 28; implicitHeight: 24; radius: 2
            glyph: Icons.zoomIn; glyphSize: 14
            enabled: sb.app.zoom < 10 - 1e-6
            tip: qsTr("Zoom in"); shortcutText: "Ctrl+Page Up"
            onClicked: sb.app.zoomIn()
        }
    }
}
