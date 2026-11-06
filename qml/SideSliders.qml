import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// Sliders flotantes a la izquierda del lienzo: tamaño o tolerancia arriba, opacidad abajo

Item {
    id: root
    required property AppController app
    readonly property bool showSize: app.toolHasSize
    readonly property bool showTolerance: app.tool === "fill"
    readonly property bool showOpacity: app.toolHasOpacity
    property real windowHeight: 800
    readonly property bool sideBySide: windowHeight < 800

    component VSlider: Rectangle {
        id: panel
        property string glyph
        property string tip
        property real from: 0
        property real to: 100
        property real value: 0
        property string valueText: ""
        signal moved(real v)
        implicitWidth: 40
        color: Theme.acrylic
        radius: Theme.overlayRadius
        border.width: 1; border.color: Theme.cardStrokeSolid
        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 8; anchors.bottomMargin: 8
            spacing: 4
            Glyph { Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 4; text: panel.glyph; size: 16 }
            Slider {
                id: s
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                from: panel.from; to: panel.to; stepSize: 1
                value: panel.value
                onMoved: panel.moved(value)
                background: Rectangle {
                    x: s.leftPadding + s.availableWidth / 2 - 2; y: s.topPadding
                    width: 4; height: s.availableHeight; radius: 2
                    color: Theme.strongStroke
                    Rectangle { y: s.visualPosition * parent.height; width: 4; height: parent.height - y; radius: 2; color: Theme.accent }
                }
                handle: Rectangle {
                    x: s.leftPadding + s.availableWidth / 2 - width / 2
                    y: s.topPadding + s.visualPosition * (s.availableHeight - height)
                    width: 20; height: 20; radius: 10
                    color: Theme.controlSolid
                    border.width: 1; border.color: Theme.controlStroke
                    Rectangle { anchors.centerIn: parent; width: s.pressed ? 10 : 12; height: width; radius: width / 2; color: Theme.accent }
                }
                ToolTip.visible: pressed || hovered
                ToolTip.text: panel.valueText
            }
        }
        ToolTip.visible: false
    }

    GridLayout {
        anchors.verticalCenter: parent.verticalCenter
        columns: root.sideBySide ? 2 : 1
        rowSpacing: 8; columnSpacing: 8
        height: root.sideBySide ? root.height : Math.min(root.height, 2 * 360 + 8)

        VSlider {
            id: sizeSlider
            visible: root.showSize && !root.showTolerance
            Layout.preferredHeight: Math.min(360, root.sideBySide ? root.height : (root.height - 8) / 2)
            glyph: Icons.ruler
            readonly property var steps: { root.app.tool; root.app.brush; return root.app.toolSizeSteps() }
            from: 0; to: steps.length - 1
            value: { let i = 0; while (i < steps.length - 1 && steps[i] < root.app.toolSize) ++i; return i }
            valueText: root.app.toolSize + "px"
            onMoved: (v) => root.app.toolSize = steps[Math.round(v)]
        }
        VSlider {
            visible: root.showTolerance
            Layout.preferredHeight: Math.min(360, root.sideBySide ? root.height : (root.height - 8) / 2)
            glyph: Icons.bucket
            from: 0; to: 100
            value: root.app.tolerance
            valueText: root.app.tolerance + "%"
            onMoved: (v) => root.app.tolerance = Math.round(v)
        }
        VSlider {
            visible: root.showOpacity
            Layout.preferredHeight: Math.min(360, root.sideBySide ? root.height : (root.height - 8) / 2)
            glyph: Icons.invertColor
            from: 0; to: 100
            value: root.app.opacity
            valueText: root.app.opacity + "%"
            onMoved: (v) => root.app.opacity = Math.round(v)
        }
    }
}
