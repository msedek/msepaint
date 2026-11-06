#include "core/tools/Handles.h"

#include <QPen>
#include <cmath>

namespace mp::handles {

QPointF handlePos(const QRectF &b, Hit h)
{
    switch (h) {
    case Top: return QPointF(b.center().x(), b.top());
    case Right: return QPointF(b.right(), b.center().y());
    case Bottom: return QPointF(b.center().x(), b.bottom());
    case Left: return QPointF(b.left(), b.center().y());
    case TopLeft: return b.topLeft();
    case TopRight: return b.topRight();
    case BottomRight: return b.bottomRight();
    case BottomLeft: return b.bottomLeft();
    default: return b.center();
    }
}

static QPointF rotatePos(const QRectF &b, qreal scale) { return QPointF(b.center().x(), b.top() - 24.0 / scale); }

Hit hitTest(const QRectF &box, QPointF p, qreal scale, bool withRotate)
{
    const qreal r = (kHandleSize / 2 + 1) / scale;
    if (withRotate) {
        const QPointF rp = rotatePos(box, scale);
        if (std::abs(p.x() - rp.x()) <= r && std::abs(p.y() - rp.y()) <= r) return Rotate;
    }
    for (Hit h : {TopLeft, TopRight, BottomRight, BottomLeft, Top, Right, Bottom, Left}) {
        const QPointF hp = handlePos(box, h);
        if (std::abs(p.x() - hp.x()) <= r && std::abs(p.y() - hp.y()) <= r) return h;
    }
    if (box.contains(p)) return Move;
    return None;
}

QRectF drag(const QRectF &s, Hit h, QPointF d, bool keepAspect)
{
    qreal l = s.left(), t = s.top(), r = s.right(), b = s.bottom();
    switch (h) {
    case Move: return s.translated(d);
    case Top: t += d.y(); break;
    case Bottom: b += d.y(); break;
    case Left: l += d.x(); break;
    case Right: r += d.x(); break;
    case TopLeft: l += d.x(); t += d.y(); break;
    case TopRight: r += d.x(); t += d.y(); break;
    case BottomRight: r += d.x(); b += d.y(); break;
    case BottomLeft: l += d.x(); b += d.y(); break;
    default: return s;
    }
    // Tamaño mínimo 1 px, sin invertir la caja.
    if (r - l < 1) { if (h == Left || h == TopLeft || h == BottomLeft) l = r - 1; else r = l + 1; }
    if (b - t < 1) { if (h == Top || h == TopLeft || h == TopRight) t = b - 1; else b = t + 1; }
    QRectF out(QPointF(l, t), QPointF(r, b));
    if (keepAspect && s.width() > 0 && s.height() > 0) {
        const qreal ratio = s.height() / s.width();
        if (h == Top || h == Bottom) {
            const qreal w = std::round(out.height() / ratio);
            out = QRectF(s.center().x() - w / 2, out.top(), w, out.height());
        } else if (h == Left || h == Right) {
            const qreal hh = std::round(out.width() * ratio);
            out = QRectF(out.left(), s.center().y() - hh / 2, out.width(), hh);
        } else {
            // Esquina: se ajusta el eje que queda corto.
            qreal w = out.width(), hh = out.height();
            if (hh < w * ratio) hh = std::round(w * ratio); else w = std::round(hh / ratio);
            const bool anchorRight = h == TopLeft || h == BottomLeft;
            const bool anchorBottom = h == TopLeft || h == TopRight;
            const qreal x = anchorRight ? s.right() - w : s.left();
            const qreal y = anchorBottom ? s.bottom() - hh : s.top();
            out = QRectF(x, y, w, hh);
        }
    }
    return out;
}

void paint(QPainter &p, const QRectF &box, qreal scale, bool withRotate, bool dashed)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    const qreal px = 1.0 / scale;
    QPen solid(QColor(255, 255, 255), px);
    p.setPen(solid);
    p.setBrush(Qt::NoBrush);
    p.drawRect(box);
    QPen dash(QColor(0, 0, 0), px, dashed ? Qt::DashLine : Qt::SolidLine);
    dash.setDashPattern({4, 4});
    p.setPen(dash);
    p.drawRect(box);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal hs = kHandleSize * px;
    p.setPen(QPen(QColor(0x5F, 0x5F, 0x5F), px));
    p.setBrush(QColor(255, 255, 255));
    for (Hit h : {TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left}) {
        const QPointF c = handlePos(box, h);
        p.drawEllipse(QRectF(c.x() - hs / 2, c.y() - hs / 2, hs, hs));
    }
    if (withRotate) {
        const QPointF rp = rotatePos(box, scale);
        p.drawLine(QPointF(box.center().x(), box.top()), rp);
        p.drawEllipse(QRectF(rp.x() - hs / 2, rp.y() - hs / 2, hs, hs));
    }
    p.restore();
}

int cursorFor(Hit h)
{
    switch (h) {
    case Move: return Qt::SizeAllCursor;
    case Top: case Bottom: return Qt::SizeVerCursor;
    case Left: case Right: return Qt::SizeHorCursor;
    case TopLeft: case BottomRight: return Qt::SizeFDiagCursor;
    case TopRight: case BottomLeft: return Qt::SizeBDiagCursor;
    case Rotate: return Qt::PointingHandCursor;
    default: return Qt::CrossCursor;
    }
}

} // namespace mp::handles
