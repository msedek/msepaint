#include "core/tools/Select.h"
#include "core/Document.h"
#include "core/History.h"
#include "core/ImageOps.h"

#include <cmath>

namespace mp {

namespace {
QPointF pixelPoint(QPointF p) { return QPointF(std::floor(p.x()), std::floor(p.y())); }
}

void SelectTool::clear()
{
    m_state = State::None;
    m_region = QPainterPath();
    m_lasso.clear();
    m_float = QImage();
    m_beforeLift = QImage();
    m_layer = -1;
    m_hit = handles::None;
}

QImage SelectTool::regionMask(const QRect &box) const
{
    QImage mask(box.size(), QImage::Format_Alpha8);
    mask.fill(0);
    QPainter p(&mask);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.translate(-box.topLeft());
    p.fillPath(m_region, Qt::black);
    return mask;
}

void SelectTool::lift(ToolContext &ctx)
{
    if (m_state != State::Marquee) return;
    Document &doc = *ctx.doc;
    m_layer = doc.activeLayer();
    QImage &layer = doc.active().image;
    m_beforeLift = layer;
    m_beforeLift.detach();
    const QRect box = m_region.boundingRect().toAlignedRect().intersected(doc.rect());
    m_float = makeLayerImage(box.size().expandedTo(QSize(1, 1)));
    if (!box.isEmpty()) {
        const QImage mask = regionMask(box);
        for (int y = 0; y < box.height(); ++y) {
            const uchar *m = mask.constScanLine(y);
            auto *src = reinterpret_cast<QRgb *>(layer.scanLine(box.top() + y));
            auto *dst = reinterpret_cast<QRgb *>(m_float.scanLine(y));
            for (int x = 0; x < box.width(); ++x) {
                if (!m[x]) continue;
                dst[x] = src[box.left() + x];
                src[box.left() + x] = 0; // la huella queda transparente
            }
        }
    }
    m_target = QRectF(box);
    m_state = State::Floating;
    doc.notifyChanged(box);
}

QImage SelectTool::preparedFloat(const QColor &color2) const
{
    if (!transparent) return m_float;
    QImage out = m_float.copy();
    const QRgb key = qPremultiply(color2.rgb() | 0xff000000u);
    for (int y = 0; y < out.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < out.width(); ++x)
            if (line[x] == key) line[x] = 0;
    }
    return out;
}

void SelectTool::commit(ToolContext &ctx)
{
    if (m_state == State::Floating && m_layer >= 0 && m_layer < ctx.doc->layerCount()) {
        Document &doc = *ctx.doc;
        QImage &layer = doc.layer(m_layer).image;
        {
            QPainter p(&layer);
            p.setRenderHint(QPainter::SmoothPixmapTransform, false); // vecino más cercano
            p.drawImage(m_target, preparedFloat(ctx.color2));
        }
        doc.history()->pushLayerEdit(m_layer, m_beforeLift);
        doc.notifyChanged(m_target.toAlignedRect().adjusted(-1, -1, 1, 1));
    }
    clear();
}

void SelectTool::cancel(ToolContext &ctx)
{
    if (m_state == State::Floating && m_layer >= 0 && m_layer < ctx.doc->layerCount()) {
        ctx.doc->layer(m_layer).image = m_beforeLift;
        ctx.doc->notifyChanged(ctx.doc->rect());
    }
    clear();
}

void SelectTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    if (ev.button != Qt::LeftButton) return;
    if (m_state == State::Marquee || m_state == State::Floating) {
        const handles::Hit h = handles::hitTest(m_target, ev.pos, m_scale);
        if (h != handles::None) {
            if (m_state == State::Marquee) lift(ctx);
            m_hit = h;
            m_dragStart = ev.pos;
            m_dragBox = m_target;
            return;
        }
        commit(ctx);
    }
    m_state = State::Creating;
    m_anchor = pixelPoint(ev.pos);
    m_target = QRectF(m_anchor, QSizeF(0, 0));
    m_lasso.clear();
    if (freeForm) m_lasso << ev.pos;
}

void SelectTool::move(ToolContext &ctx, const ToolEvent &ev)
{
    if (m_state == State::Creating) {
        if (freeForm) {
            m_lasso << ev.pos;
            m_target = m_lasso.boundingRect();
        } else {
            const QPointF p = pixelPoint(ev.pos);
            QRectF r = QRectF(m_anchor, p).normalized();
            // La esquina del puntero incluye su píxel.
            if (p.x() >= m_anchor.x()) r.setRight(p.x() + 1); else r.setRight(m_anchor.x() + 1);
            if (p.y() >= m_anchor.y()) r.setBottom(p.y() + 1); else r.setBottom(m_anchor.y() + 1);
            m_target = r.intersected(QRectF(ctx.doc->rect()));
        }
        return;
    }
    if (m_hit != handles::None && m_state == State::Floating) {
        QPointF d = ev.pos - m_dragStart;
        d = QPointF(std::round(d.x()), std::round(d.y()));
        m_target = handles::drag(m_dragBox, m_hit, d, ev.modifiers & Qt::ShiftModifier);
        m_target = QRectF(std::round(m_target.x()), std::round(m_target.y()),
                          std::max(1.0, std::round(m_target.width())), std::max(1.0, std::round(m_target.height())));
        return;
    }
    hover(ctx, ev);
}

void SelectTool::release(ToolContext &ctx, const ToolEvent &)
{
    if (m_state == State::Creating) {
        if (freeForm) {
            QPainterPath path;
            path.addPolygon(m_lasso);
            path.closeSubpath();
            m_region = path.intersected([&] { QPainterPath r; r.addRect(QRectF(ctx.doc->rect())); return r; }());
            m_target = QRectF(m_region.boundingRect().toAlignedRect());
        } else {
            m_region = QPainterPath();
            m_region.addRect(m_target);
        }
        m_lasso.clear();
        m_state = (m_target.width() >= 1 && m_target.height() >= 1) ? State::Marquee : State::None;
        if (m_state == State::None) clear();
        return;
    }
    m_hit = handles::None;
}

void SelectTool::hover(ToolContext &, const ToolEvent &ev)
{
    if (m_state == State::Marquee || m_state == State::Floating)
        m_cursor = handles::cursorFor(handles::hitTest(m_target, ev.pos, m_scale));
    else
        m_cursor = Qt::CrossCursor;
}

void SelectTool::paintOverlay(QPainter &p, qreal scale) const
{
    if (m_state == State::None) return;
    if (m_state == State::Creating) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, false);
        QPen dash(Qt::black, 1.0 / scale, Qt::DashLine);
        dash.setDashPattern({4, 4});
        if (freeForm) {
            p.setPen(QPen(Qt::white, 1.0 / scale));
            p.drawPolyline(m_lasso);
            p.setPen(dash);
            p.drawPolyline(m_lasso);
        } else {
            p.setPen(QPen(Qt::white, 1.0 / scale));
            p.drawRect(m_target);
            p.setPen(dash);
            p.drawRect(m_target);
        }
        p.restore();
        return;
    }
    if (m_state == State::Floating) {
        p.save();
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        p.drawImage(m_target, m_float);
        p.restore();
    }
    if (freeForm && m_state == State::Marquee && !m_region.isEmpty()) {
        p.save();
        QPen dash(Qt::black, 1.0 / scale, Qt::DashLine);
        dash.setDashPattern({4, 4});
        p.setPen(QPen(Qt::white, 1.0 / scale));
        p.drawPath(m_region);
        p.setPen(dash);
        p.drawPath(m_region);
        p.restore();
    }
    handles::paint(p, m_target, scale);
}

void SelectTool::selectAll(ToolContext &ctx)
{
    commit(ctx);
    m_region = QPainterPath();
    m_region.addRect(QRectF(ctx.doc->rect()));
    m_target = QRectF(ctx.doc->rect());
    m_state = State::Marquee;
}

void SelectTool::invert(ToolContext &ctx)
{
    if (m_state == State::Floating) commit(ctx);
    QPainterPath all;
    all.addRect(QRectF(ctx.doc->rect()));
    m_region = (m_state == State::None) ? all : all.subtracted(m_region);
    m_target = QRectF(m_region.boundingRect().toAlignedRect());
    m_state = m_region.isEmpty() ? State::None : State::Marquee;
    if (m_state == State::None) clear();
}

void SelectTool::deleteContent(ToolContext &ctx)
{
    if (m_state == State::Marquee) lift(ctx);
    if (m_state != State::Floating) return;
    Document &doc = *ctx.doc;
    if (m_layer >= 0 && m_layer < doc.layerCount())
        doc.history()->pushLayerEdit(m_layer, m_beforeLift);
    clear();
}

QImage SelectTool::copyContent(ToolContext &ctx) const
{
    if (m_state == State::Floating) {
        const QSize s = m_target.toAlignedRect().size();
        return s == m_float.size() ? m_float : m_float.scaled(s, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    if (m_state != State::Marquee) return {};
    const QRect box = m_region.boundingRect().toAlignedRect().intersected(ctx.doc->rect());
    if (box.isEmpty()) return {};
    QImage out = makeLayerImage(box.size());
    const QImage mask = regionMask(box);
    const QImage &layer = ctx.doc->active().image;
    for (int y = 0; y < box.height(); ++y) {
        const uchar *m = mask.constScanLine(y);
        const auto *src = reinterpret_cast<const QRgb *>(layer.constScanLine(box.top() + y));
        auto *dst = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < box.width(); ++x)
            if (m[x]) dst[x] = src[box.left() + x];
    }
    return out;
}

void SelectTool::cut(ToolContext &ctx, QImage *out)
{
    if (out) *out = copyContent(ctx);
    deleteContent(ctx);
}

void SelectTool::paste(ToolContext &ctx, const QImage &img, QPoint at)
{
    commit(ctx);
    Document &doc = *ctx.doc;
    // Si lo pegado no cabe, se agranda el lienzo (paso de historial aparte).
    const QSize need(std::max(doc.size().width(), at.x() + img.width()), std::max(doc.size().height(), at.y() + img.height()));
    if (need != doc.size()) ops::resizeCanvas(doc, need);
    m_layer = doc.activeLayer();
    m_beforeLift = doc.active().image;
    m_beforeLift.detach();
    m_float = img.convertToFormat(Document::kFormat);
    m_target = QRectF(QRect(at, img.size()));
    m_region = QPainterPath();
    m_region.addRect(m_target);
    m_state = State::Floating;
}

void SelectTool::crop(ToolContext &ctx)
{
    if (m_state == State::None) return;
    if (m_state == State::Floating) commit(ctx), m_state = State::None;
    Document &doc = *ctx.doc;
    const QRect box = (m_region.isEmpty() ? m_target : m_region.boundingRect()).toAlignedRect().intersected(doc.rect());
    if (box.isEmpty()) { clear(); return; }
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    const bool masked = freeForm && !m_region.isEmpty();
    const QImage mask = masked ? regionMask(box) : QImage();
    for (Layer &l : layers) {
        QImage c = l.image.copy(box);
        if (masked) {
            for (int y = 0; y < c.height(); ++y) {
                const uchar *m = mask.constScanLine(y);
                auto *line = reinterpret_cast<QRgb *>(c.scanLine(y));
                for (int x = 0; x < c.width(); ++x)
                    if (!m[x]) line[x] = 0;
            }
        }
        l.image = c;
    }
    doc.replaceLayers(std::move(layers), box.size(), doc.activeLayer());
    doc.history()->pushSnapshot(std::move(before));
    clear();
}

void SelectTool::transformFloat(ToolContext &ctx, const QTransform &t)
{
    if (m_state == State::Marquee) lift(ctx);
    if (m_state != State::Floating) return;
    const QSize s = m_target.toAlignedRect().size();
    QImage scaled = s == m_float.size() ? m_float : m_float.scaled(s, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    m_float = scaled.transformed(t, Qt::FastTransformation).convertToFormat(Document::kFormat);
    const QPointF c = m_target.center();
    m_target = QRectF(std::round(c.x() - m_float.width() / 2.0), std::round(c.y() - m_float.height() / 2.0),
                      m_float.width(), m_float.height());
}

void SelectTool::resizeFloat(ToolContext &ctx, QSize size)
{
    if (m_state == State::Marquee) lift(ctx);
    if (m_state != State::Floating) return;
    m_target.setSize(QSizeF(size.expandedTo(QSize(1, 1))));
}

void SelectTool::nudge(ToolContext &ctx, QPoint delta)
{
    if (m_state == State::Marquee) lift(ctx);
    if (m_state != State::Floating) return;
    m_target.translate(delta);
}

void SelectTool::invertColors(ToolContext &ctx)
{
    if (m_state == State::Floating) {
        m_float.invertPixels(QImage::InvertRgb);
        return;
    }
    if (m_state != State::Marquee) return;
    Document &doc = *ctx.doc;
    const QRect box = m_region.boundingRect().toAlignedRect().intersected(doc.rect());
    QImage before = doc.active().image;
    before.detach();
    QImage &layer = doc.active().image;
    const QImage mask = regionMask(box);
    for (int y = 0; y < box.height(); ++y) {
        const uchar *m = mask.constScanLine(y);
        auto *line = reinterpret_cast<QRgb *>(layer.scanLine(box.top() + y));
        for (int x = 0; x < box.width(); ++x) {
            if (!m[x]) continue;
            QRgb &px = line[box.left() + x];
            const int a = qAlpha(px);
            px = qRgba(a - qRed(px), a - qGreen(px), a - qBlue(px), a); // invertir en premultiplicado
        }
    }
    doc.history()->pushLayerEdit(doc.activeLayer(), before, box);
    doc.notifyChanged(box);
}

} // namespace mp
