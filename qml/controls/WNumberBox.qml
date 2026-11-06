import QtQuick
import QtQuick.Controls.Basic
import MsePaint

// NumberBox  estilo Fluent sobre WTextBox: valida el texto, flechas = paso pequeño, RePág/AvPág = paso grande.
// `value` solo cambia con texto numérico; `valid` dice si el texto actual es un número dentro de [from, to]
// (o solo un número si `rangeCheck` es false). `edited(v)` se emite cuando el usuario cambia el valor.
WTextBox {
    id: nb
    property real value: 0
    property real from: -1e9
    property real to: 1e9
    property int decimals: 0
    property real smallStep: 1
    property real largeStep: 10
    property bool rangeCheck: true
    readonly property bool valid: _valid
    property bool _valid: true
    property bool _internal: false
    signal edited(real v)

    function format(v) { return decimals > 0 ? Number(v).toFixed(decimals) : String(Math.round(v)) }
    function parse(t) {
        let s = String(t).trim()
        if (suffix !== "" && s.endsWith(suffix)) s = s.slice(0, s.length - suffix.length).trim()
        if (!/^[-+]?(\d+([.,]\d*)?|[.,]\d+)([eE][-+]?\d+)?$/.test(s)) return NaN
        return Number(s.replace(",", "."))
    }
    function roundTo(v) { const k = Math.pow(10, decimals); return Math.round(v * k) / k }
    // Asigna desde fuera (no emite `edited`).
    function setValue(v) {
        v = roundTo(v)
        _internal = true
        value = v
        text = format(v)
        _internal = false
        _valid = !rangeCheck || (v >= from && v <= to)
    }
    function step(d) {
        const cur = parse(text)
        const v = roundTo(Math.max(from, Math.min(to, (isNaN(cur) ? value : cur) + d)))
        setValue(v)
        edited(v)
        selectAll()
    }

    error: !_valid
    Component.onCompleted: text = format(value)
    onValueChanged: if (!_internal && parse(text) !== value) { text = format(value); _valid = !rangeCheck || (value >= from && value <= to) }
    onTextEdited: {
        const v = parse(text)
        _valid = !isNaN(v) && (!rangeCheck || (v >= from && v <= to))
        if (!isNaN(v)) { _internal = true; value = roundTo(v); _internal = false; edited(value) }
    }
    onEditingFinished: if (_valid) text = format(value)
    onActiveFocusChanged: if (activeFocus) selectAll()

    Keys.onUpPressed: step(smallStep)
    Keys.onDownPressed: step(-smallStep)
    Keys.onPressed: (ev) => {
        if (ev.key === Qt.Key_PageUp) { step(largeStep); ev.accepted = true }
        else if (ev.key === Qt.Key_PageDown) { step(-largeStep); ev.accepted = true }
    }
}
