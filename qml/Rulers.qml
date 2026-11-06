import QtQuick
import MsePaint

// Reglas horizontal y vertical de 20 px con números de 12 y la marca roja del cursor

Item {
    id: r
    required property AppController app
    required property Item canvasItem
    required property Flickable flick

    // Origen del lienzo en coordenadas de la regla.
    readonly property real originX: canvasItem.x - flick.contentX
    readonly property real originY: canvasItem.y - flick.contentY
    readonly property real zoom: app.zoom

    function majorStep() {
        // Paso de las marcas largas en píxeles de imagen: el menor "redondo" que deje ≥ 50 px de pantalla.
        const cands = [1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000]
        for (const c of cands) if (c * zoom >= 50) return c
        return 10000
    }

    Rectangle { width: 20; height: 20; color: Theme.rulerBg }

    Canvas {
        id: hRuler
        x: 20; width: parent.width - 20; height: 20
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = Theme.rulerBg
            ctx.fillRect(0, 0, width, height)
            ctx.strokeStyle = Theme.rulerFg
            ctx.fillStyle = Theme.rulerFg
            ctx.font = "12px '" + Theme.uiFont + "'"
            ctx.lineWidth = 1
            const step = r.majorStep(), minor = step / 10
            const ox = r.originX
            const first = Math.floor(-ox / r.zoom / minor) * minor
            for (let v = first; v * r.zoom + ox <= width; v += minor) {
                const x = Math.round(v * r.zoom + ox) + 0.5
                const isMajor = Math.abs(v / step - Math.round(v / step)) < 1e-6
                const isHalf = !isMajor && Math.abs(v / (step / 2) - Math.round(v / (step / 2))) < 1e-6
                const len = isMajor ? 20 : isHalf ? 8 : 4
                ctx.beginPath(); ctx.moveTo(x, height); ctx.lineTo(x, height - len); ctx.stroke()
                if (isMajor) ctx.fillText(Math.round(v), x + 2, 11)
            }
            if (r.app.cursorInside) {
                const cx = Math.round(r.app.cursorPos.x * r.zoom + ox) + 0.5
                ctx.strokeStyle = "#FF0000"
                ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, height); ctx.stroke()
            }
            ctx.strokeStyle = Theme.divider
            ctx.beginPath(); ctx.moveTo(0, height - 0.5); ctx.lineTo(width, height - 0.5); ctx.stroke()
        }
    }

    Canvas {
        id: vRuler
        y: 20; width: 20; height: parent.height - 20
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = Theme.rulerBg
            ctx.fillRect(0, 0, width, height)
            ctx.strokeStyle = Theme.rulerFg
            ctx.fillStyle = Theme.rulerFg
            ctx.font = "12px '" + Theme.uiFont + "'"
            const step = r.majorStep(), minor = step / 10
            const oy = r.originY
            const first = Math.floor(-oy / r.zoom / minor) * minor
            for (let v = first; v * r.zoom + oy <= height; v += minor) {
                const y = Math.round(v * r.zoom + oy) + 0.5
                const isMajor = Math.abs(v / step - Math.round(v / step)) < 1e-6
                const isHalf = !isMajor && Math.abs(v / (step / 2) - Math.round(v / (step / 2))) < 1e-6
                const len = isMajor ? 20 : isHalf ? 8 : 4
                ctx.beginPath(); ctx.moveTo(width, y); ctx.lineTo(width - len, y); ctx.stroke()
                if (isMajor) {
                    ctx.save()
                    ctx.translate(11, y + 2)
                    ctx.rotate(-Math.PI / 2)
                    ctx.textAlign = "right"
                    ctx.fillText(Math.round(v), 0, 0)
                    ctx.restore()
                }
            }
            if (r.app.cursorInside) {
                const cy = Math.round(r.app.cursorPos.y * r.zoom + oy) + 0.5
                ctx.strokeStyle = "#FF0000"
                ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(width, cy); ctx.stroke()
            }
            ctx.strokeStyle = Theme.divider
            ctx.beginPath(); ctx.moveTo(width - 0.5, 0); ctx.lineTo(width - 0.5, height); ctx.stroke()
        }
    }

    Connections {
        target: r.app
        enabled: r.visible
        function onCursorChanged() { hRuler.requestPaint(); vRuler.requestPaint() }
        function onZoomChanged() { hRuler.requestPaint(); vRuler.requestPaint() }
    }
    Connections {
        target: r.flick
        enabled: r.visible
        function onContentXChanged() { hRuler.requestPaint() }
        function onContentYChanged() { vRuler.requestPaint() }
    }
    onVisibleChanged: if (visible) { hRuler.requestPaint(); vRuler.requestPaint() }
    onWidthChanged: hRuler.requestPaint()
    onHeightChanged: vRuler.requestPaint()
}
