import QtQuick
import MsePaint

// Asas del borde derecho, inferior y esquina del lienzo: arrastrar cambia el tamaño del lienzo
// (sin escalar; la zona nueva queda transparente).
Item {
    id: h
    required property AppController app
    required property Item canvasItem
    property size preview: Qt.size(0, 0)
    property bool dragging: false
    x: canvasItem.x; y: canvasItem.y
    width: canvasItem.width; height: canvasItem.height

    component Grip: Rectangle {
        id: grip
        property bool horiz: false
        property bool vert: false
        width: 7; height: 7
        color: Theme.controlSolid
        border.width: 1; border.color: Theme.strongStroke
        MouseArea {
            anchors.fill: parent
            anchors.margins: -4
            cursorShape: grip.horiz && grip.vert ? Qt.SizeFDiagCursor : grip.horiz ? Qt.SizeHorCursor : Qt.SizeVerCursor
            property point start
            property size startSize
            onPressed: (m) => {
                start = mapToItem(h, m.x, m.y)
                startSize = h.app.canvasSize
                h.preview = startSize
                h.dragging = true
            }
            onPositionChanged: (m) => {
                if (!pressed) return
                const p = mapToItem(h, m.x, m.y)
                const w = grip.horiz ? Math.max(1, Math.round(startSize.width + (p.x - start.x) / h.app.zoom)) : startSize.width
                const hh = grip.vert ? Math.max(1, Math.round(startSize.height + (p.y - start.y) / h.app.zoom)) : startSize.height
                h.preview = Qt.size(w, hh)
            }
            onReleased: {
                h.dragging = false
                if (h.preview.width !== startSize.width || h.preview.height !== startSize.height)
                    h.app.resizeCanvas(h.preview.width, h.preview.height)
            }
        }
    }

    Grip { horiz: true; x: parent.width + 2; y: parent.height / 2 - height / 2 }
    Grip { vert: true; x: parent.width / 2 - width / 2; y: parent.height + 2 }
    Grip { horiz: true; vert: true; x: parent.width + 2; y: parent.height + 2 }

    // Vista previa del nuevo borde.
    Rectangle {
        visible: h.dragging
        width: h.preview.width * h.app.zoom
        height: h.preview.height * h.app.zoom
        color: "transparent"
        border.width: 1
        border.color: Theme.textPrimary
        opacity: 0.7
    }
}
