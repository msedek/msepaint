import QtQuick
import MsePaint

// Menú contextual del lienzo.
PMenu {
    id: m
    required property AppController app
    signal requestRotateCustom()
    signal requestResize()

    PMenuItem { text: qsTr("Cut"); glyph: Icons.cut; shortcutText: "Ctrl+X"; enabled: m.app.hasSelection; onTriggered: m.app.cut() }
    PMenuItem { text: qsTr("Copy"); glyph: Icons.copy; shortcutText: "Ctrl+C"; enabled: m.app.hasSelection; onTriggered: m.app.copy() }
    PMenuItem { text: qsTr("Paste"); glyph: Icons.paste; shortcutText: "Ctrl+V"; onTriggered: m.app.paste() }
    PMenuSeparator {}
    PMenuItem { text: qsTr("Copy visible layers"); glyph: Icons.copyLayers; shortcutText: "Ctrl+Shift+C"; onTriggered: m.app.copyVisibleLayers() }
    PMenuItem { text: qsTr("Crop"); glyph: Icons.crop; shortcutText: "Ctrl+Shift+X"; enabled: m.app.hasSelection; onTriggered: m.app.crop() }
    PMenuItem { text: qsTr("Select all"); glyph: Icons.selectAll; shortcutText: "Ctrl+A"; onTriggered: m.app.selectAll() }
    PMenuItem { text: qsTr("Invert selection"); glyph: Icons.selectInvert; onTriggered: m.app.invertSelection() }
    PMenuItem { text: qsTr("Delete"); glyph: Icons.trash; shortcutText: "Delete"; enabled: m.app.hasSelection; onTriggered: m.app.deleteSelection() }
    PMenuSeparator {}
    PMenu {
        title: qsTr("Rotate")
        PMenuItem { text: qsTr("Rotate right 90º"); glyph: Icons.rotateRight; onTriggered: m.app.rotateRight() }
        PMenuItem { text: qsTr("Rotate left 90º"); glyph: Icons.rotateLeft; onTriggered: m.app.rotateLeft() }
        PMenuItem { text: qsTr("Rotate 180º"); glyph: Icons.rotate180; onTriggered: m.app.rotate180() }
        PMenuItem { text: qsTr("Rotate custom"); glyph: Icons.rotateRight; onTriggered: m.requestRotateCustom() }
    }
    PMenu {
        title: qsTr("Flip")
        PMenuItem { text: qsTr("Flip vertical"); glyph: Icons.flipV; onTriggered: m.app.flipVertical() }
        PMenuItem { text: qsTr("Flip horizontal"); glyph: Icons.flipH; onTriggered: m.app.flipHorizontal() }
    }
    PMenuItem { text: qsTr("Resize"); glyph: Icons.resize; shortcutText: "Ctrl+W"; onTriggered: m.requestResize() }
    PMenuItem { text: qsTr("Invert color"); glyph: Icons.invertColor; shortcutText: "Ctrl+Shift+I"; onTriggered: m.app.invertColors() }
}
