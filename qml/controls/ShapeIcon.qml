import QtQuick
import QtQuick.Shapes
import MsePaint

// Silueta de una forma de la galería, generada con la misma geometría que dibuja la herramienta.
Shape {
    id: s
    required property string shapeId
    required property AppController app
    width: 18; height: 18
    preferredRendererType: Shape.CurveRenderer
    ShapePath {
        strokeColor: Theme.iconBrush
        strokeWidth: 1
        fillColor: "transparent"
        joinStyle: ShapePath.MiterJoin
        PathSvg { path: s.app.shapeIconPath(s.shapeId, 18) }
    }
}
