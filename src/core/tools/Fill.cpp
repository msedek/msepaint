#include "core/tools/Fill.h"
#include "core/Document.h"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace mp {

// Comparación de color: igualdad exacta o distancia euclídea en ARGB
// premultiplicado ≤ tol² × 510², con tol en 0–1.
static bool similar(QRgb a, QRgb b, qint64 limit)
{
    if (a == b) return true;
    if (limit == 0) return false;
    const qint64 dA = qAlpha(a) - qAlpha(b), dR = qRed(a) - qRed(b), dG = qGreen(a) - qGreen(b), dB = qBlue(a) - qBlue(b);
    return dA * dA + dR * dR + dG * dG + dB * dB <= limit;
}

QRect floodFill(QImage &img, QPoint seed, QRgb color, int tolerance)
{
    if (!img.rect().contains(seed)) return {};
    const int w = img.width(), h = img.height();
    auto px = [&](int x, int y) -> QRgb & { return reinterpret_cast<QRgb *>(img.scanLine(y))[x]; };
    const QRgb target = px(seed.x(), seed.y());
    const double tol = qBound(0, tolerance, 100) / 100.0;
    const qint64 maxDelta = qint64(std::floor(tol * tol * 260100.0));
    if (target == color) return {};

    std::vector<uint8_t> seen(size_t(w) * size_t(h), 0);
    std::vector<QPoint> stack{seed};
    int minX = seed.x(), maxX = seed.x(), minY = seed.y(), maxY = seed.y();

    // Relleno por tramos horizontales.
    while (!stack.empty()) {
        const QPoint p = stack.back();
        stack.pop_back();
        int y = p.y();
        int x = p.x();
        if (seen[size_t(y) * w + x] || !similar(px(x, y), target, maxDelta)) continue;
        int l = x, r = x;
        while (l > 0 && !seen[size_t(y) * w + l - 1] && similar(px(l - 1, y), target, maxDelta)) --l;
        while (r < w - 1 && !seen[size_t(y) * w + r + 1] && similar(px(r + 1, y), target, maxDelta)) ++r;
        for (int i = l; i <= r; ++i) {
            seen[size_t(y) * w + i] = 1;
            px(i, y) = color;
        }
        minX = std::min(minX, l); maxX = std::max(maxX, r);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        for (int ny : {y - 1, y + 1}) {
            if (ny < 0 || ny >= h) continue;
            bool inRun = false;
            for (int i = l; i <= r; ++i) {
                const bool ok = !seen[size_t(ny) * w + i] && similar(px(i, ny), target, maxDelta);
                if (ok && !inRun) stack.push_back(QPoint(i, ny));
                inRun = ok;
            }
        }
    }
    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}

void FillTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    LayerEdit edit;
    edit.begin(*ctx.doc);
    const QPoint p(int(std::floor(ev.pos.x())), int(std::floor(ev.pos.y())));
    // El color de relleno se escribe siempre opaco.
    const QRect r = floodFill(ctx.doc->active().image, p, colorFor(ctx, ev).rgb() | 0xff000000u, ctx.tolerance);
    edit.end(*ctx.doc, r);
    if (!r.isEmpty()) ctx.doc->notifyChanged(r);
}

void ColorPickerTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    const QPoint p(int(std::floor(ev.pos.x())), int(std::floor(ev.pos.y())));
    if (!ctx.doc->rect().contains(p)) return;
    // Toma el color visible: compone las capas solo en ese píxel.
    QImage tmp(1, 1, Document::kFormat);
    tmp.fill(ctx.doc->backgroundVisible() ? ctx.doc->background() : QColor(Qt::transparent));
    QPainter pt(&tmp);
    for (const Layer &l : ctx.doc->layers()) {
        if (!l.visible) continue;
        pt.setOpacity(l.opacity);
        pt.drawImage(QPoint(0, 0), l.image, QRect(p, QSize(1, 1)));
    }
    pt.end();
    (ev.button == Qt::RightButton ? ctx.color2 : ctx.color1) = QColor::fromRgba(tmp.pixel(0, 0));
}

} // namespace mp
