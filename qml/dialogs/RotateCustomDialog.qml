import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MsePaint

// «Rotate custom». Ángulo positivo = horario.
// Valor inicial 0, rango −360…360.
WDialog {
    id: dlg
    required property AppController app

    title: qsTr("Rotate custom")
    primaryEnabled: angle.valid

    onAboutToShow: angle.setValue(0)
    onOpened: angle.forceActiveFocus()
    onAccepted: app.rotateCustom(angle.value)

    WNumberBox {
        id: angle
        Layout.fillWidth: true
        from: -360; to: 360
        decimals: 0
        rangeCheck: false
        suffix: "°"
        // Solo se marca error si no es un número; fuera de −360…360 se acota al aceptar.
        error: !valid
        errorText: qsTr("Please enter a valid number.")
        Accessible.name: qsTr("Angle in degrees")
        placeholderText: qsTr("Angle in degrees")
    }
}
