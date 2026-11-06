#include "core/tools/Shape.h"
#include "core/Brushes.h"
#include "core/Document.h"
#include "core/History.h"
#include "core/ShapeGeometry.h"

#include <QTransform>
#include <cmath>
#include <cstring>

namespace mp {

namespace {
QPointF centerOf(QPointF p) { return QPointF(std::floor(p.x()) + 0.5, std::floor(p.y()) + 0.5); }
qreal dist(QPointF a, QPointF b) { return std::hypot(a.x() - b.x(), a.y() - b.y()); }
}

QPointF ShapeTool::toLocal(QPointF p) const
{
    if (m_angle == 0) return p;
    const QPointF c = m_box.center();
    return QTransform().translate(c.x(), c.y()).rotate(-m_angle).translate(-c.x(), -c.y()).map(p);
}

QPainterPath ShapeTool::currentPath() const
{
    QPainterPath path;
    if (isLine()) {
        path.moveTo(m_p0);
        path.lineTo(m_p1);
    } else if (isCurve()) {
        path.moveTo(m_p0);
        path.cubicTo(m_c1, m_c2, m_p1);
    } else if (isPolygon()) {
        if (!m_poly.isEmpty()) {
            path.addPolygon(m_poly);
            if (m_closed) path.closeSubpath();
        }
    } else if (m_box.width() > 0 && m_box.height() > 0) {
        path = shapes::fitted(shape, m_box, m_angle);
    }
    return path;
}

void ShapeTool::render(ToolContext &ctx)
{
    if (m_layer < 0 || m_layer >= ctx.doc->layerCount()) return;
    Document &doc = *ctx.doc;
    QImage &layer = doc.layer(m_layer).image;
    const QPainterPath path = currentPath();
    const int w = std::max(1, ctx.size);
    const QRect dirty = path.boundingRect().toAlignedRect().adjusted(-w - 2, -w - 2, w + 2, w + 2).intersected(doc.rect());
    const QRect area = dirty | m_lastDirty;
    // Restaurar la zona anterior desde la copia previa.
    for (int y = area.top(); y <= area.bottom(); ++y)
        std::memcpy(layer.scanLine(y) + area.left() * 4, m_before.constScanLine(y) + area.left() * 4, size_t(area.width()) * 4);
    m_lastDirty = dirty;
    if (path.isEmpty()) { doc.notifyChanged(area); return; }

    const bool open = isLine() || isCurve() || (isPolygon() && !m_closed);
    const bool hasFill = !open && fill != QLatin1String("none");
    const bool hasOutline = outline != QLatin1String("none") || open;
    const QString outMedia = open && outline == QLatin1String("none") ? QStringLiteral("solid") : outline;

    // 1) Relleno
    if (hasFill) {
        if (fill == QLatin1String("solid")) {
            QPainter p(&layer);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.fillPath(path, m_fillColor);
            if (!hasOutline) { // costura del AA tapada con 1 px del color de relleno
                p.setPen(QPen(m_fillColor, 1));
                p.drawPath(path);
            }
        } else {
            brushes::CoverageMask mask;
            mask.reset(doc.size());
            brushes::fillPath(mask, brushes::mediaFromId(fill), path);
            const QImage base = layer.copy();
            brushes::composePaint(layer, base, mask, mask.bounds(), m_fillColor, 1.0);
        }
    }
    // 2) Contorno
    if (hasOutline) {
        if (outMedia == QLatin1String("solid")) {
            QPainter p(&layer);
            p.setRenderHint(QPainter::Antialiasing, true);
            QPen pen(m_outlineColor, w);
            pen.setJoinStyle(open ? Qt::RoundJoin : Qt::MiterJoin);
            pen.setCapStyle(open ? Qt::RoundCap : Qt::SquareCap);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        } else {
            brushes::CoverageMask mask;
            mask.reset(doc.size());
            brushes::strokePath(mask, brushes::mediaFromId(outMedia), path, std::max(4, w));
            const QImage base = layer.copy();
            brushes::composePaint(layer, base, mask, mask.bounds(), m_outlineColor, 1.0);
        }
    }
    doc.notifyChanged(area);
}

void ShapeTool::begin(ToolContext &ctx, const ToolEvent &ev)
{
    m_layer = ctx.doc->activeLayer();
    m_before = ctx.doc->active().image;
    m_before.detach();
    m_lastDirty = QRect();
    const bool right = ev.button == Qt::RightButton;
    m_outlineColor = right ? ctx.color2 : ctx.color1;
    m_fillColor = right ? ctx.color1 : ctx.color2;
    m_angle = 0;
    m_closed = false;
}

void ShapeTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    const QPointF p = centerOf(ev.pos);
    switch (m_state) {
    case State::CurveCtrl1:
    case State::CurveCtrl2:
        if (m_state == State::CurveCtrl1) m_c1 = m_c2 = p; else m_c2 = p;
        render(ctx);
        return;
    case State::Polygon:
        if (m_poly.size() >= 3 && dist(p, m_poly.first()) <= 4.0 / m_scale + 2) {
            m_closed = true;
            render(ctx);
            commit(ctx);
            return;
        }
        m_poly << p;
        render(ctx);
        return;
    case State::Editing: {
        if (isLine()) {
            const qreal r = (handles::kHandleSize / 2 + 3) / m_scale;
            if (dist(ev.pos, m_p0) <= r) { m_endpoint = 0; m_dragStart = ev.pos; return; }
            if (dist(ev.pos, m_p1) <= r) { m_endpoint = 1; m_dragStart = ev.pos; return; }
        } else {
            const handles::Hit h = handles::hitTest(m_box, toLocal(ev.pos), m_scale, true);
            if (h != handles::None) {
                m_hit = h;
                m_dragStart = ev.pos;
                m_dragBox = m_box;
                m_dragAngle = m_angle;
                return;
            }
        }
        commit(ctx);
        break;
    }
    default:
        break;
    }
    // Empieza una forma nueva.
    begin(ctx, ev);
    m_state = State::Creating;
    m_anchor = m_p0 = m_p1 = p;
    m_box = QRectF(p, QSizeF(0, 0));
    if (isPolygon()) { m_poly.clear(); m_poly << p << p; }
}

void ShapeTool::move(ToolContext &ctx, const ToolEvent &ev)
{
    QPointF p = centerOf(ev.pos);
    const bool shift = ev.modifiers & Qt::ShiftModifier;
    switch (m_state) {
    case State::Creating:
        if (isLine() || isCurve()) {
            if (shift) p = QPointF(shapes::constrainLine(m_p0.toPoint(), p.toPoint()));
            m_p1 = p;
            m_c1 = m_p0; m_c2 = m_p1;
        } else if (isPolygon()) {
            if (shift) p = QPointF(shapes::constrainLine(m_poly[m_poly.size() - 2].toPoint(), p.toPoint()));
            m_poly.last() = p;
        } else {
            QPointF d = p - m_anchor;
            if (shift) { const qreal s = std::max(std::abs(d.x()), std::abs(d.y())); d = QPointF(d.x() < 0 ? -s : s, d.y() < 0 ? -s : s); }
            m_box = QRectF(m_anchor, m_anchor + d).normalized();
        }
        render(ctx);
        return;
    case State::CurveCtrl1:
        m_c1 = m_c2 = p;
        render(ctx);
        return;
    case State::CurveCtrl2:
        m_c2 = p;
        render(ctx);
        return;
    case State::Polygon:
        if (shift && m_poly.size() >= 2) p = QPointF(shapes::constrainLine(m_poly[m_poly.size() - 2].toPoint(), p.toPoint()));
        if (!m_poly.isEmpty()) m_poly.last() = p;
        render(ctx);
        return;
    case State::Editing:
        if (m_endpoint >= 0) {
            QPointF q = p;
            if (shift) q = QPointF(shapes::constrainLine((m_endpoint == 0 ? m_p1 : m_p0).toPoint(), p.toPoint()));
            (m_endpoint == 0 ? m_p0 : m_p1) = q;
            render(ctx);
        } else if (m_hit == handles::Rotate) {
            const QPointF c = m_box.center();
            const qreal a0 = std::atan2(m_dragStart.y() - c.y(), m_dragStart.x() - c.x());
            const qreal a1 = std::atan2(ev.pos.y() - c.y(), ev.pos.x() - c.x());
            m_angle = m_dragAngle + (a1 - a0) * 180.0 / M_PI;
            render(ctx);
        } else if (m_hit != handles::None) {
            // Desplazamiento en el marco de la forma girada.
            const QPointF d = QTransform().rotate(-m_angle).map(ev.pos - m_dragStart);
            m_box = handles::drag(m_dragBox, m_hit, m_hit == handles::Move ? ev.pos - m_dragStart : d, shift);
            render(ctx);
        } else {
            hover(ctx, ev);
        }
        return;
    default:
        hover(ctx, ev);
    }
}

void ShapeTool::release(ToolContext &ctx, const ToolEvent &)
{
    switch (m_state) {
    case State::Creating:
        if (isCurve()) { m_state = State::CurveCtrl1; return; }
        if (isPolygon()) { m_state = State::Polygon; m_poly << m_poly.last(); render(ctx); return; }
        if (isLine()) {
            if (m_p0 == m_p1) { cancel(ctx); return; }
            m_state = State::Editing;
            return;
        }
        if (m_box.width() < 1 || m_box.height() < 1) { cancel(ctx); return; }
        m_state = State::Editing;
        return;
    case State::CurveCtrl1:
        m_state = State::CurveCtrl2;
        return;
    case State::CurveCtrl2:
        commit(ctx);
        return;
    case State::Editing:
        m_hit = handles::None;
        m_endpoint = -1;
        return;
    default:
        return;
    }
}

void ShapeTool::doubleClick(ToolContext &ctx)
{
    if (m_state != State::Polygon) return;
    // El doble clic añadió un vértice repetido: quitarlo y cerrar.
    while (m_poly.size() > 3 && m_poly.last() == m_poly[m_poly.size() - 2]) m_poly.removeLast();
    m_closed = true;
    render(ctx);
    commit(ctx);
}

void ShapeTool::hover(ToolContext &, const ToolEvent &ev)
{
    if (m_state != State::Editing) { m_cursor = Qt::CrossCursor; return; }
    if (isLine()) {
        const qreal r = (handles::kHandleSize / 2 + 3) / m_scale;
        m_cursor = (dist(ev.pos, m_p0) <= r || dist(ev.pos, m_p1) <= r) ? Qt::SizeAllCursor : Qt::CrossCursor;
        return;
    }
    m_cursor = handles::cursorFor(handles::hitTest(m_box, toLocal(ev.pos), m_scale, true));
}

void ShapeTool::commit(ToolContext &ctx)
{
    if (m_state == State::None) return;
    if (m_state == State::Polygon && !m_closed) {
        while (m_poly.size() > 2 && m_poly.last() == m_poly[m_poly.size() - 2]) m_poly.removeLast();
        m_closed = m_poly.size() >= 3;
        render(ctx);
    }
    if (m_layer >= 0 && m_layer < ctx.doc->layerCount())
        ctx.doc->history()->pushLayerEdit(m_layer, m_before, m_lastDirty);
    m_state = State::None;
    m_before = QImage();
    m_layer = -1;
    m_hit = handles::None;
    m_endpoint = -1;
    m_poly.clear();
}

void ShapeTool::cancel(ToolContext &ctx)
{
    if (m_state != State::None && m_layer >= 0 && m_layer < ctx.doc->layerCount()) {
        ctx.doc->layer(m_layer).image = m_before;
        ctx.doc->notifyChanged(m_lastDirty);
    }
    m_state = State::None;
    m_before = QImage();
    m_layer = -1;
    m_poly.clear();
}

void ShapeTool::restyle(ToolContext &ctx)
{
    if (m_state == State::None) return;
    m_outlineColor = ctx.color1;
    m_fillColor = ctx.color2;
    render(ctx);
}

void ShapeTool::paintOverlay(QPainter &p, qreal scale) const
{
    if (m_state != State::Editing) return;
    if (isLine()) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal hs = handles::kHandleSize / scale;
        p.setPen(QPen(QColor(0x5F, 0x5F, 0x5F), 1.0 / scale));
        p.setBrush(Qt::white);
        for (QPointF c : {m_p0, m_p1}) p.drawEllipse(QRectF(c.x() - hs / 2, c.y() - hs / 2, hs, hs));
        p.restore();
        return;
    }
    p.save();
    const QPointF c = m_box.center();
    p.translate(c);
    p.rotate(m_angle);
    p.translate(-c);
    handles::paint(p, m_box, scale, true);
    p.restore();
}

} // namespace mp
