import QtQuick

// Rueda de color de 28×28 para el botón «Edit colors» (dibujo propio).
Canvas {
    width: 28; height: 28
    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        const cx = width / 2, cy = height / 2, r = 13
        for (let a = 0; a < 360; a += 3) {
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.arc(cx, cy, r, (a - 1.5) * Math.PI / 180, (a + 3) * Math.PI / 180)
            ctx.closePath()
            ctx.fillStyle = Qt.hsva(a / 360, 1, 1, 1)
            ctx.fill()
        }
        const g = ctx.createRadialGradient(cx, cy, 0, cx, cy, r)
        g.addColorStop(0, "#FFFFFFFF")
        g.addColorStop(1, "#00FFFFFF")
        ctx.beginPath()
        ctx.arc(cx, cy, r, 0, Math.PI * 2)
        ctx.fillStyle = g
        ctx.fill()
    }
}
