#include "core/tools/Stroke.h"
#include "core/Document.h"
#include "core/History.h"

#include <cmath>

namespace mp {

using brushes::Media;

QPointF StrokeTool::snap(QPointF p) const
{
    // Los puntos del puntero son enteros de lienzo; el motor trabaja con centros de píxel.
    return QPointF(std::floor(p.x()) + 0.5, std::floor(p.y()) + 0.5);
}

QPointF StrokeTool::constrain(QPointF p, Qt::KeyboardModifiers mods)
{
    // Shift fija una de cuatro direcciones en el primer movimiento.
    if (!(mods & Qt::ShiftModifier)) { m_shiftDir = -1; return p; }
    const qreal dx = p.x() - m_start.x(), dy = p.y() - m_start.y();
    if (m_shiftDir < 0) {
        if (dx == 0 && dy == 0) return p;
        const qreal m = dx == 0 ? 1e9 : dy / dx;
        const qreal am = std::abs(m);
        if (am < 0.41421356) m_shiftDir = 0;
        else if (am >= 2.41421356) m_shiftDir = 1;
        else m_shiftDir = m > 0 ? 2 : 3;
    }
    switch (m_shiftDir) {
    case 0: return QPointF(p.x(), m_start.y());
    case 1: return QPointF(m_start.x(), p.y());
    default: {
        const qreal d = std::min(std::abs(dx), std::abs(dy));
        const qreal sx = dx >= 0 ? 1 : -1;
        const qreal sy = m_shiftDir == 2 ? sx : -sx;
        return QPointF(m_start.x() + sx * d, m_start.y() + sy * d);
    }
    }
}

void StrokeTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    Document &doc = *ctx.doc;
    m_active = true;
    m_layer = doc.activeLayer();
    m_before = doc.active().image;
    m_before.detach();
    m_mask.reset(doc.size());
    m_secondary = ev.button == Qt::RightButton;
    m_color = colorFor(ctx, ev);
    m_start = m_last = snap(ev.pos);
    m_arc = 0;
    m_shiftDir = -1;
    brushes::segment(m_mask, m_media, m_last, m_last, ctx.size, 0);
    apply(ctx, m_mask.dirty());
}

void StrokeTool::move(ToolContext &ctx, const ToolEvent &ev)
{
    m_hover = ev.pos;
    if (!m_active) return;
    const QPointF p = constrain(snap(ev.pos), ev.modifiers);
    if (p == m_last) return;
    brushes::segment(m_mask, m_media, m_last, p, ctx.size, m_arc);
    m_arc += std::hypot(p.x() - m_last.x(), p.y() - m_last.y());
    m_last = p;
    apply(ctx, m_mask.dirty());
}

void StrokeTool::tick(ToolContext &ctx)
{
    if (!m_active) return;
    brushes::airbrushPulse(m_mask, m_last, ctx.size);
    apply(ctx, m_mask.dirty());
}

void StrokeTool::release(ToolContext &ctx, const ToolEvent &)
{
    if (!m_active) return;
    m_active = false;
    if (m_layer < ctx.doc->layerCount())
        ctx.doc->history()->pushLayerEdit(m_layer, m_before, m_mask.bounds());
    m_before = QImage();
    m_mask.reset(QSize());
}

void StrokeTool::hover(ToolContext &ctx, const ToolEvent &ev)
{
    m_hover = ev.pos;
    m_hoverSize = ctx.size;
}

void StrokeTool::apply(ToolContext &ctx, const QRect &dirty)
{
    if (dirty.isEmpty() || m_layer >= ctx.doc->layerCount()) return;
    QImage &layer = ctx.doc->layer(m_layer).image;
    if (m_kind == Kind::Eraser) {
        if (m_secondary)
            brushes::composeRecolor(layer, m_before, m_mask, dirty, ctx.color1, ctx.color2);
        else
            brushes::composeErase(layer, m_before, m_mask, dirty, 1.0);
    } else {
        brushes::composePaint(layer, m_before, m_mask, dirty, m_color, ctx.opacity);
    }
    m_mask.clearDirty();
    ctx.doc->notifyChanged(dirty);
}

void StrokeTool::paintOverlay(QPainter &p, qreal scale) const
{
    // Contorno del borrador bajo el cursor.
    if (m_kind != Kind::Eraser || m_hover.x() < 0) return;
    const int w = std::max(1, m_hoverSize);
    const QPointF c(std::floor(m_hover.x()) - w / 2, std::floor(m_hover.y()) - w / 2);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(Qt::black, 1.0 / scale));
    p.setBrush(Qt::NoBrush);
    if (m_media == Media::RoundEraser) p.drawEllipse(QRectF(c, QSizeF(w, w)));
    else p.drawRect(QRectF(c, QSizeF(w, w)));
    p.restore();
}

} // namespace mp
