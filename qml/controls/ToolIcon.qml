import QtQuick
import MsePaint

// Ícono propio de color (assets/icons, generado por scripts/gen-icons.py) con variante por tema.
Image {
    property string name
    property int size: 16
    width: size; height: size
    sourceSize: Qt.size(size * 2, size * 2)
    source: "qrc:/qt/qml/MsePaint/assets/icons/" + name + (Theme.dark ? "-dark" : "-light") + ".svg"
    smooth: true
}
