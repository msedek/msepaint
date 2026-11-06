import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes
import MsePaint

// Fila 1: la cinta.
Rectangle {
    id: ribbon
    required property AppController app
    property bool layersOpen: false
    signal requestResizeDialog()
    signal requestRotateCustom()
    signal requestEditColors()
    signal requestRibbonMode(bool autoHide)
    signal toggleLayers()

    readonly property var paletteRow1: ["#000000", "#7F7F7F", "#880015", "#ED1C24", "#FF7F27", "#FFF200", "#22B14C", "#00A2E8", "#3F48CC", "#A349A4"]
    readonly property var paletteRow2: ["#FFFFFF", "#C3C3C3", "#B97A57", "#FFAEC9", "#FFC90E", "#EFE4B0", "#B5E61D", "#99D9EA", "#7092BE", "#C8BFE7"]
    readonly property var brushes: [
        { id: "brush", name: qsTr("Brush") },
        { id: "calligraphyBrush", name: qsTr("Calligraphy brush") },
        { id: "calligraphyPen", name: qsTr("Calligraphy pen") },
        { id: "airbrush", name: qsTr("Airbrush") },
        { id: "oilBrush", name: qsTr("Oil brush") },
        { id: "crayon", name: qsTr("Crayon") },
        { id: "marker", name: qsTr("Marker") },
        { id: "naturalPencil", name: qsTr("Natural pencil") },
        { id: "watercolor", name: qsTr("Watercolor brush") }
    ]
    readonly property var shapeNames: ({
        line: qsTr("Line"), curve: qsTr("Curve"), oval: qsTr("Oval"), rectangle: qsTr("Rectangle"),
        roundedRectangle: qsTr("Rounded rectangle"), polygon: qsTr("Polygon"), triangle: qsTr("Triangle"),
        rightTriangle: qsTr("Right triangle"), diamond: qsTr("Diamond"), pentagon: qsTr("Pentagon"),
        hexagon: qsTr("Hexagon"), rightArrow: qsTr("Right arrow"), leftArrow: qsTr("Left arrow"),
        upArrow: qsTr("Up arrow"), downArrow: qsTr("Down arrow"), fourPointStar: qsTr("Four-point star"),
        fivePointStar: qsTr("Five-point star"), sixPointStar: qsTr("Six-point star"),
        roundedRectangularCallout: qsTr("Rounded rectangular callout"), ovalCallout: qsTr("Oval callout"),
        cloudCallout: qsTr("Cloud callout"), heart: qsTr("Heart"), lightning: qsTr("Lightning")
    })
    readonly property var outlineMedia: [
        { id: "none", name: qsTr("No outline") }, { id: "solid", name: qsTr("Solid outline") },
        { id: "crayon", name: qsTr("Crayon") }, { id: "marker", name: qsTr("Marker") },
        { id: "oilBrush", name: qsTr("Oil brush") }, { id: "naturalPencil", name: qsTr("Natural pencil") },
        { id: "watercolor", name: qsTr("Watercolor brush") }
    ]
    readonly property var fillMedia: [
        { id: "none", name: qsTr("No fill") }, { id: "solid", name: qsTr("Solid fill") },
        { id: "crayon", name: qsTr("Crayon") }, { id: "marker", name: qsTr("Marker") },
        { id: "oilBrush", name: qsTr("Oil brush") }, { id: "naturalPencil", name: qsTr("Natural pencil") },
        { id: "watercolor", name: qsTr("Watercolor brush") }
    ]
    readonly property var shapeSizes: [1, 2, 3, 4, 5, 6, 8, 10, 16, 30, 40]

    function brushName(id) { for (const b of brushes) if (b.id === id) return b.name; return "" }

    implicitHeight: 4 + groups.implicitHeight + 4
    color: Theme.layerFill
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.cardStrokeSolid }

    // ---- Reglas de colapso por ancho (prioridad: Colors 1, Tools 2, Image 3, Shapes 4) ----
    readonly property real fixedWidth: selectionGroup.fullWidth + brushesGroup.fullWidth + layersGroup.fullWidth
    readonly property real avail: width - 24
    property int shapeColumnsDrop: {
        // Shapes primero pierde columnas: 306 → 218 en pasos de 22.
        const full = fixedWidth + imageGroup.fullWidth + toolsGroup.fullWidth + colorsGroup.fullWidth + 306 + 24 + 100
        if (avail >= full) return 0
        return Math.min(4, Math.ceil((full - avail) / 22))
    }
    readonly property real shapesWidth: 306 - shapeColumnsDrop * 22 + 24 + 100
    readonly property real needAll: fixedWidth + imageGroup.fullWidth + toolsGroup.fullWidth + colorsGroup.fullWidth + shapesWidth
    readonly property bool collapseColors: avail < needAll
    readonly property bool collapseTools: avail < needAll - colorsGroup.fullWidth + 72
    readonly property bool collapseImage: avail < needAll - colorsGroup.fullWidth - toolsGroup.fullWidth + 144
    readonly property bool collapseShapes: avail < needAll - colorsGroup.fullWidth - toolsGroup.fullWidth - imageGroup.fullWidth + 216

    RowLayout {
        id: groups
        x: 0; y: 4
        width: parent.width
        spacing: 0

        // 1. Selection
        RibbonGroup {
            id: selectionGroup
            label: qsTr("Selection")
            SplitToolButton {
                glyph: ribbon.app.selectMode === "freeform" ? Icons.selectFree : Icons.selectRect
                selected: ribbon.app.tool === "select"
                tip: qsTr("Select")
                shortcutText: "S"
                onActivated: ribbon.app.tool = "select"
                menu: PMenu {
                    PMenuItem { text: qsTr("Rectangle"); glyph: Icons.selectRect; radio: true; checkable: true; checked: ribbon.app.selectMode === "rectangle"; onTriggered: { ribbon.app.selectMode = "rectangle"; ribbon.app.tool = "select" } }
                    PMenuItem { text: qsTr("Free-form"); glyph: Icons.selectFree; radio: true; checkable: true; checked: ribbon.app.selectMode === "freeform"; onTriggered: { ribbon.app.selectMode = "freeform"; ribbon.app.tool = "select" } }
                    PMenuSeparator {}
                    PMenuItem { text: qsTr("Select all"); glyph: Icons.selectAll; shortcutText: "Ctrl+A"; onTriggered: ribbon.app.selectAll() }
                    PMenuItem { text: qsTr("Invert selection"); glyph: Icons.selectInvert; onTriggered: ribbon.app.invertSelection() }
                    PMenuItem { text: qsTr("Transparent selection"); checkable: true; checked: ribbon.app.transparentSelection; onTriggered: ribbon.app.transparentSelection = !ribbon.app.transparentSelection }
                    PMenuItem { text: qsTr("Delete"); glyph: Icons.trash; shortcutText: "Delete"; onTriggered: ribbon.app.deleteSelection() }
                }
            }
        }

        // 2. Image
        RibbonGroup {
            id: imageGroup
            label: qsTr("Image")
            collapsed: ribbon.collapseImage
            collapsedGlyph: Icons.resize
            GridLayout {
                columns: 3; rowSpacing: 8; columnSpacing: 8
                PButton { Layout.rowSpan: 2; glyph: Icons.crop; tip: qsTr("Crop"); shortcutText: "Ctrl+Shift+X"; enabled: ribbon.app.hasSelection; onClicked: ribbon.app.crop() }
                RibbonDropDown {
                    glyph: Icons.rotateRight; tip: qsTr("Rotate")
                    menu: PMenu {
                        PMenuItem { text: qsTr("Rotate right 90º"); glyph: Icons.rotateRight; onTriggered: ribbon.app.rotateRight() }
                        PMenuItem { text: qsTr("Rotate left 90º"); glyph: Icons.rotateLeft; onTriggered: ribbon.app.rotateLeft() }
                        PMenuItem { text: qsTr("Rotate 180º"); glyph: Icons.rotate180; onTriggered: ribbon.app.rotate180() }
                        PMenuItem { text: qsTr("Rotate custom"); glyph: Icons.rotateRight; onTriggered: ribbon.requestRotateCustom() }
                    }
                }
                PButton { Layout.rowSpan: 2; glyph: Icons.resize; glyphSize: 24; tip: qsTr("Resize and skew"); shortcutText: "Ctrl+W"; onClicked: ribbon.requestResizeDialog() }
                RibbonDropDown {
                    glyph: Icons.flipH; tip: qsTr("Flip")
                    menu: PMenu {
                        PMenuItem { text: qsTr("Flip vertical"); glyph: Icons.flipV; onTriggered: ribbon.app.flipVertical() }
                        PMenuItem { text: qsTr("Flip horizontal"); glyph: Icons.flipH; onTriggered: ribbon.app.flipHorizontal() }
                    }
                }
            }
        }

        // 3. Tools
        RibbonGroup {
            id: toolsGroup
            label: qsTr("Tools")
            collapsed: ribbon.collapseTools
            collapsedGlyph: Icons.tools
            GridLayout {
                columns: 3; rowSpacing: 8; columnSpacing: 8
                PButton {
                    tip: qsTr("Pencil"); shortcutText: "P"; selected: ribbon.app.tool === "pencil"; onClicked: ribbon.app.tool = "pencil"
                    iconItem: Component { ToolIcon { name: "pencil" } }
                }
                PButton {
                    tip: qsTr("Fill"); shortcutText: "B"; selected: ribbon.app.tool === "fill"; onClicked: ribbon.app.tool = "fill"
                    iconItem: Component { ToolIcon { name: "fill" } }
                }
                PButton {
                    tip: qsTr("Text"); shortcutText: "T"; selected: ribbon.app.tool === "text"; onClicked: ribbon.app.tool = "text"
                    iconItem: Component { ToolIcon { name: "text" } }
                }
                PButton {
                    tip: qsTr("Eraser"); shortcutText: "E"; selected: ribbon.app.tool === "eraser"; onClicked: ribbon.app.tool = "eraser"
                    iconItem: Component { ToolIcon { name: "eraser" } }
                }
                PButton {
                    tip: qsTr("Color picker"); shortcutText: "I"; selected: ribbon.app.tool === "picker"; onClicked: ribbon.app.tool = "picker"
                    iconItem: Component { ToolIcon { name: "picker" } }
                }
                PButton {
                    tip: qsTr("Magnifier"); shortcutText: "Z"; selected: ribbon.app.tool === "magnifier"; onClicked: ribbon.app.tool = "magnifier"
                    iconItem: Component { ToolIcon { name: "magnifier" } }
                }
            }
        }

        // 4. Brushes
        RibbonGroup {
            id: brushesGroup
            label: qsTr("Brushes")
            SplitToolButton {
                iconItem: Component { ToolIcon { name: "brush"; size: 24 } }
                selected: ribbon.app.tool === "brush"
                tip: ribbon.brushName(ribbon.app.brush)
                onActivated: ribbon.app.tool = "brush"
                menu: PMenu {
                    reserveIcon: false
                    Repeater {
                        model: ribbon.brushes
                        PMenuItem {
                            required property var modelData
                            text: modelData.name
                            radio: true; checkable: true
                            checked: ribbon.app.brush === modelData.id
                            preview: BrushSwatch { media: modelData.id }
                            onTriggered: { ribbon.app.brush = modelData.id; ribbon.app.tool = "brush" }
                        }
                    }
                }
            }
        }

        // 5. Shapes
        RibbonGroup {
            id: shapesGroup
            label: qsTr("Shapes")
            collapsed: ribbon.collapseShapes
            collapsedGlyph: Icons.shapes
            RowLayout {
                spacing: 8
                Rectangle {
                    Layout.preferredWidth: 258 - ribbon.shapeColumnsDrop * 22
                    Layout.preferredHeight: 72
                    color: "transparent"
                    radius: Theme.controlRadius
                    border.width: 1; border.color: Theme.divider
                    Flickable {
                        anchors.fill: parent
                        anchors.leftMargin: 4; anchors.topMargin: 4; anchors.bottomMargin: 4; anchors.rightMargin: 12
                        contentHeight: shapeFlow.height
                        clip: true
                        flickableDirection: Flickable.VerticalFlick
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 4 }
                        Flow {
                            id: shapeFlow
                            width: parent.width
                            Repeater {
                                model: ribbon.app.shapeIds()
                                Item {
                                    required property string modelData
                                    width: 22; height: 22
                                    PButton {
                                        anchors.centerIn: parent
                                        width: 20; height: 20
                                        radius: 2
                                        tip: ribbon.shapeNames[modelData]
                                        selected: ribbon.app.tool === "shape" && ribbon.app.shape === modelData
                                        iconItem: ShapeIcon { shapeId: modelData; app: ribbon.app }
                                        onClicked: { ribbon.app.shape = modelData; ribbon.app.tool = "shape" }
                                    }
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    spacing: 8
                    RibbonDropDown {
                        tip: qsTr("Shape outline")
                        glyph: Icons.outlineColor
                        glyphColor: ribbon.app.tool === "shape" ? Theme.iconBrush : Theme.textDisabled
                        menu: PMenu {
                            reserveIcon: false
                            Repeater {
                                model: ribbon.outlineMedia
                                PMenuItem {
                                    required property var modelData
                                    text: modelData.name; radio: true; checkable: true
                                    checked: ribbon.app.shapeOutline === modelData.id
                                    preview: BrushSwatch { media: modelData.id; outline: true }
                                    onTriggered: ribbon.app.shapeOutline = modelData.id
                                }
                            }
                        }
                    }
                    RibbonDropDown {
                        tip: qsTr("Shape fill")
                        glyph: Icons.fillColor
                        glyphColor: ribbon.app.tool === "shape" ? Theme.iconBrush : Theme.textDisabled
                        menu: PMenu {
                            reserveIcon: false
                            Repeater {
                                model: ribbon.fillMedia
                                PMenuItem {
                                    required property var modelData
                                    text: modelData.name; radio: true; checkable: true
                                    checked: ribbon.app.shapeFill === modelData.id
                                    preview: BrushSwatch { media: modelData.id; fill: true }
                                    onTriggered: ribbon.app.shapeFill = modelData.id
                                }
                            }
                        }
                    }
                }
                PButton {
                    id: sizeButton
                    implicitWidth: 40; implicitHeight: 64
                    tip: qsTr("Size")
                    enabled: ribbon.app.tool === "shape"
                    contentItem: Column {
                        spacing: 4
                        topPadding: 8
                        Item {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 24; height: 24
                            Column {
                                anchors.centerIn: parent
                                spacing: 3
                                Repeater {
                                    model: [1, 2, 3, 4]
                                    Rectangle { required property int modelData; width: 18; height: modelData; color: sizeButton.enabled ? Theme.iconBrush : Theme.textDisabled }
                                }
                            }
                        }
                        Glyph { anchors.horizontalCenter: parent.horizontalCenter; text: Icons.chevronDown; size: 8; color: Theme.textSecondary }
                    }
                    onClicked: sizeMenu.popup(sizeButton, 0, sizeButton.height + 2)
                    PMenu {
                        id: sizeMenu
                        reserveIcon: false
                        Repeater {
                            model: ribbon.shapeSizes
                            PMenuItem {
                                required property int modelData
                                text: modelData + "px"; radio: true; checkable: true
                                checked: ribbon.app.toolSize === modelData
                                preview: Item {
                                    Rectangle { anchors.verticalCenter: parent.verticalCenter; x: 8; width: 92; height: Math.min(32, modelData); color: Theme.textSecondary }
                                }
                                onTriggered: ribbon.app.toolSize = modelData
                            }
                        }
                    }
                }
            }
        }

        // 6. Colors
        RibbonGroup {
            id: colorsGroup
            label: qsTr("Colors")
            dividerRight: false
            collapsed: ribbon.collapseColors
            collapsedGlyph: Icons.colorWheel
            RowLayout {
                spacing: 8
                ColumnLayout {
                    spacing: 4
                    ColorSlot { which: 1; app: ribbon.app }
                    ColorSlot { which: 2; app: ribbon.app }
                }
                Grid {
                    columns: 10
                    width: 240
                    Repeater {
                        model: ribbon.paletteRow1.concat(ribbon.paletteRow2)
                        PaletteCell { required property string modelData; swatch: modelData; onPicked: ribbon.app.pickPaletteColor(modelData) }
                    }
                    Repeater {
                        model: 10
                        PaletteCell {
                            required property int index
                            swatch: index < ribbon.app.customColors.length ? ribbon.app.customColors[index] : ""
                            onPicked: if (swatch !== "") ribbon.app.pickPaletteColor(swatch)
                        }
                    }
                }
                PButton {
                    implicitWidth: 40; implicitHeight: 40
                    tip: qsTr("Edit colors")
                    iconItem: ColorWheel {}
                    onClicked: ribbon.requestEditColors()
                }
            }
        }

        Item { Layout.fillWidth: true }

        // 7. Layers
        RibbonGroup {
            id: layersGroup
            Layout.rightMargin: 16
            label: qsTr("Layers")
            dividerRight: false
            dividerLeft: true
            PButton {
                implicitWidth: 40; implicitHeight: 40
                iconItem: Component { ToolIcon { name: "layers"; size: 24 } }
                tip: qsTr("Layers")
                selected: ribbon.layersOpen
                onClicked: ribbon.toggleLayers()
            }
        }
    }

    // Botón de opciones de la cinta (esquina inferior derecha).
    PButton {
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 4; anchors.bottomMargin: 4
        width: 20; height: 20
        glyph: Icons.chevronDown; glyphSize: 12
        tip: qsTr("Toolbar options")
        onClicked: optionsMenu.popup(this, width - optionsMenu.width, height + 2)
        PMenu {
            id: optionsMenu
            PMenuItem { text: qsTr("Always show toolbar"); checkable: true; checked: true; onTriggered: ribbon.requestRibbonMode(false) }
            PMenuItem { text: qsTr("Automatically hide toolbar"); checkable: true; checked: false; onTriggered: ribbon.requestRibbonMode(true) }
        }
    }
}
