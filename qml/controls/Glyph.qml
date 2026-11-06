import QtQuick
import MsePaint

// Un glifo de la fuente de íconos.
Text {
    property int size: 16
    property bool filled: false
    font.family: filled ? Theme.iconFontFilled : Theme.iconFont
    font.pixelSize: size
    color: Theme.iconBrush
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    renderType: Text.NativeRendering
}
