import QtQuick
import MsePaint

// Muestra de trazo de 120×40 para los menús de pinceles, contorno y relleno.
// La genera el motor de pinceles (image://swatch/<medio>?outline|fill).
Image {
    id: sw
    property string media: "brush"
    property bool outline: false
    property bool fill: false
    width: 120; height: 40
    fillMode: Image.PreserveAspectFit
    sourceSize: Qt.size(240, 80)
    source: "image://swatch/" + media + (outline ? "?outline" : fill ? "?fill" : "") + (Theme.dark ? "&dark" : "")
    cache: true
    smooth: true
}
