#include "core/Brushes.h"

#include <QPainter>
#include <QPolygonF>
#include <QRandomGenerator>
#include <algorithm>
#include <array>
#include <cmath>

namespace mp::brushes {

namespace {

inline qreal sat(qreal v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
inline qreal smooth01(qreal f) { f = sat(f); return f * f * (3 - 2 * f); }
inline qreal smoothstep(qreal e0, qreal e1, qreal x) { return smooth01((x - e0) / (e1 - e0)); }

// --- Ruido propio para las texturas de los pinceles artísticos -------------------------------------------
inline quint32 hash2(qint32 x, qint32 y, quint32 seed)
{
    quint32 h = quint32(x) * 0x8da6b343u ^ quint32(y) * 0xd8163841u ^ seed * 0xcb1ab31fu;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
inline qreal rnd(qint32 x, qint32 y, quint32 seed) { return (hash2(x, y, seed) & 0xffffff) / qreal(0x1000000); }

qreal valueNoise(qreal x, qreal y, quint32 seed)
{
    const qreal fx = std::floor(x), fy = std::floor(y);
    const qint32 ix = qint32(fx), iy = qint32(fy);
    const qreal tx = smooth01(x - fx), ty = smooth01(y - fy);
    const qreal a = rnd(ix, iy, seed), b = rnd(ix + 1, iy, seed);
    const qreal c = rnd(ix, iy + 1, seed), d = rnd(ix + 1, iy + 1, seed);
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}

qreal fbm(qreal x, qreal y, quint32 seed)
{
    return 0.55 * valueNoise(x, y, seed) + 0.3 * valueNoise(x * 2.1, y * 2.1, seed + 1)
        + 0.15 * valueNoise(x * 4.3, y * 4.3, seed + 2);
}

// Cobertura de los medios artísticos. u = largo de arco, v = desplazamiento normalizado (−1…1),
// (x, y) = píxel del lienzo para el grano, w = grosor.
qreal artisticCoverage(Media m, qreal u, qreal v, int x, int y, qreal w)
{
    const qreal av = std::abs(v);
    switch (m) {
    case Media::Oil: {
        const qreal edge = 1.0 - smoothstep(0.82, 1.0, av + 0.06 * (valueNoise(u / 9.0, v * 2, 11) - 0.5));
        const qreal streak = 0.6 + 0.4 * valueNoise(v * w * 0.32, u / (w * 1.5 + 20), 12);
        const qreal head = smoothstep(0.0, std::max(2.0, w * 0.4), u);
        return edge * streak * (0.55 + 0.45 * head);
    }
    case Media::Watercolor: {
        const qreal edgeV = av + 0.18 * (valueNoise(u / 14.0, v * 1.5, 21) - 0.5);
        const qreal body = 1.0 - smoothstep(0.55, 1.0, edgeV);
        const qreal rim = smoothstep(0.55, 0.9, edgeV) * body;
        const qreal blot = 0.75 + 0.25 * fbm(x / 22.0, y / 22.0, 22);
        return std::min(0.75, (0.38 + 0.35 * rim) * body * blot + 0.12 * body);
    }
    case Media::Crayon: {
        const qreal jag = av + 0.14 * (valueNoise(u / 3.0, v > 0 ? 1 : 7, 31) - 0.5);
        if (jag > 0.97) return 0;
        const qreal grain = fbm(x / 1.4, y / 1.4, 32);
        const qreal c = grain > 0.42 ? 0.55 + 0.45 * grain : 0.15 * grain;
        return c * (1.0 - smoothstep(0.85, 0.97, jag));
    }
    case Media::NaturalPencil: {
        const qreal jag = av + 0.1 * (valueNoise(u / 2.5, v > 0 ? 3 : 9, 41) - 0.5);
        if (jag > 1.0) return 0;
        const qreal grain = fbm(x / 0.9, y / 0.9, 42);
        return (grain > 0.35 ? 0.45 + 0.4 * grain : 0.12) * (1.0 - smoothstep(0.75, 1.0, jag));
    }
    case Media::Marker: {
        const qreal edge = 1.0 - smoothstep(0.9, 1.0, av);
        return edge * (0.86 + 0.08 * valueNoise(x / 6.0, y / 6.0, 51));
    }
    default:
        return 0;
    }
}

qreal fillTexture(Media m, int x, int y)
{
    switch (m) {
    case Media::Oil: return 0.65 + 0.35 * valueNoise(y / 3.0, x / 40.0, 61);
    case Media::Watercolor: return 0.35 + 0.3 * fbm(x / 22.0, y / 22.0, 62);
    case Media::Crayon: { const qreal g = fbm(x / 1.4, y / 1.4, 32); return g > 0.42 ? 0.55 + 0.45 * g : 0.15 * g; }
    case Media::NaturalPencil: { const qreal g = fbm(x / 0.9, y / 0.9, 42); return g > 0.35 ? 0.45 + 0.4 * g : 0.12; }
    case Media::Marker: return 0.86 + 0.08 * valueNoise(x / 6.0, y / 6.0, 51);
    default: return 1.0;
    }
}

// Cobertura del shader de cápsula.
inline qreal capsuleCoverage(qreal d, qreal R, qreal border)
{
    if (d >= R) return 0;
    const qreal inner = R - border;
    if (d <= inner) return 1;
    const qreal f = sat((d - inner) / (R - inner));
    return 1 - f * f * (3 - 2 * f);
}

inline qreal distToSegment(QPointF p, QPointF a, QPointF dir, qreal len, qreal *tOut = nullptr)
{
    const QPointF ap = p - a;
    const qreal t = ap.x() * dir.x() + ap.y() * dir.y();
    if (tOut) *tOut = t;
    if (t <= 0) return std::hypot(ap.x(), ap.y());
    if (t >= len) { const QPointF bp = p - (a + dir * len); return std::hypot(bp.x(), bp.y()); }
    return std::abs(ap.x() * -dir.y() + ap.y() * dir.x());
}

QRect boundsOf(QPointF a, QPointF b, qreal pad)
{
    const int l = int(std::floor(std::min(a.x(), b.x()) - pad)) - 1;
    const int t = int(std::floor(std::min(a.y(), b.y()) - pad)) - 1;
    const int r = int(std::ceil(std::max(a.x(), b.x()) + pad)) + 1;
    const int bt = int(std::ceil(std::max(a.y(), b.y()) + pad)) + 1;
    return QRect(QPoint(l, t), QPoint(r, bt));
}

// Sello redondo del lápiz, sin antialiasing.
const std::vector<uint8_t> &roundStamp(int w)
{
    static std::vector<std::vector<uint8_t>> cache(512);
    w = std::clamp(w, 1, 511);
    auto &s = cache[size_t(w)];
    if (s.empty()) {
        s.assign(size_t(w) * size_t(w), 0);
        const qreal c = (w % 2 == 0) ? (w - 1) / 2.0 : qreal(w / 2);
        const qreal R = (w % 2 == 0) ? w / 2.0 : (w - 0.5) / 2.0;
        for (int j = 0; j < w; ++j)
            for (int i = 0; i < w; ++i)
                s[size_t(j) * w + i] = std::hypot(i - c, j - c) > R ? 0 : 255;
    }
    return s;
}

template <typename F>
void bresenham(QPoint a, QPoint b, F &&plot)
{
    int x0 = a.x(), y0 = a.y();
    const int x1 = b.x(), y1 = b.y();
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

inline QPoint px(QPointF p) { return QPoint(int(std::floor(p.x())), int(std::floor(p.y()))); }

// Punto dentro de un polígono convexo (vértices en cualquier sentido).
bool insideConvex(const QPointF &p, const QPolygonF &poly)
{
    int sign = 0;
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i], b = poly[(i + 1) % poly.size()];
        const qreal cross = (b.x() - a.x()) * (p.y() - a.y()) - (b.y() - a.y()) * (p.x() - a.x());
        if (cross == 0) continue;
        const int s = cross > 0 ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return true;
}

QPolygonF convexHull(std::vector<QPointF> pts)
{
    std::sort(pts.begin(), pts.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    if (pts.size() < 3) return QPolygonF(QList<QPointF>(pts.begin(), pts.end()));
    auto cross = [](QPointF o, QPointF a, QPointF b) {
        return (a.x() - o.x()) * (b.y() - o.y()) - (a.y() - o.y()) * (b.x() - o.x());
    };
    std::vector<QPointF> h(pts.size() * 2);
    size_t k = 0;
    for (size_t i = 0; i < pts.size(); ++i) {
        while (k >= 2 && cross(h[k - 2], h[k - 1], pts[i]) <= 0) --k;
        h[k++] = pts[i];
    }
    for (size_t i = pts.size() - 1, t = k + 1; i > 0; --i) {
        while (k >= t && cross(h[k - 2], h[k - 1], pts[i - 1]) <= 0) --k;
        h[k++] = pts[i - 1];
    }
    h.resize(k - 1);
    return QPolygonF(QList<QPointF>(h.begin(), h.end()));
}

// Distancia con signo al borde de un polígono convexo: negativa dentro.
qreal signedDistToPolygon(const QPointF &p, const QPolygonF &poly);

qreal distToPolygon(const QPointF &p, const QPolygonF &poly)
{
    if (insideConvex(p, poly)) return 0;
    qreal best = 1e9;
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i], b = poly[(i + 1) % poly.size()];
        const QPointF ab = b - a;
        const qreal len = std::hypot(ab.x(), ab.y());
        if (len == 0) { best = std::min(best, std::hypot(p.x() - a.x(), p.y() - a.y())); continue; }
        best = std::min(best, distToSegment(p, a, ab / len, len));
    }
    return best;
}

qreal signedDistToPolygon(const QPointF &p, const QPolygonF &poly)
{
    qreal best = 1e9;
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i], b = poly[(i + 1) % poly.size()];
        const QPointF ab = b - a;
        const qreal len = std::hypot(ab.x(), ab.y());
        best = std::min(best, len == 0 ? std::hypot(p.x() - a.x(), p.y() - a.y()) : distToSegment(p, a, ab / len, len));
    }
    return insideConvex(p, poly) ? -best : best;
}

void capsule(CoverageMask &mask, QPointF p0, QPointF p1, qreal width, const QPolygonF *clip = nullptr)
{
    const qreal R = std::max(0.5, width / 2.0);
    const qreal border = std::clamp(width / 10.0, 0.1, 1.0);
    const QPointF d = p1 - p0;
    const qreal len = std::hypot(d.x(), d.y());
    const QPointF dir = len > 0 ? d / len : QPointF(1, 0);
    const QRect box = boundsOf(p0, p1, R).intersected(QRect(QPoint(0, 0), mask.size()));
    for (int y = box.top(); y <= box.bottom(); ++y)
        for (int x = box.left(); x <= box.right(); ++x) {
            const QPointF c(x + 0.5, y + 0.5);
            // Recorte por la plumilla con un píxel de transición (borde suavizado, como el resto del pincel).
            const qreal clipCov = clip ? sat(0.5 - signedDistToPolygon(c, *clip)) : 1.0;
            if (clipCov <= 0) continue;
            const qreal cov = capsuleCoverage(distToSegment(c, p0, dir, len), R, border) * clipCov;
            if (cov > 0) mask.maxAt(x, y, int(cov * 255 + 0.5));
        }
    mask.touch(box);
    mask.grow(box);
}

// Plumilla recta de semieje r·1,42 a 45° («/») o 135° («\\») barrida de p0 a p1 y recortada a la cápsula de
// radio W/2. Los bordes largos del barrido llevan un píxel de suavizado; los extremos se
// solapan medio píxel con el segmento vecino para que las uniones no dejen marcas.
void calligraphy(CoverageMask &mask, bool slash, QPointF p0, QPointF p1, qreal width)
{
    const qreal k = width / 4.0 * 1.42;
    const QPointF n = slash ? QPointF(-k, k) : QPointF(k, k);
    const QPointF d = p1 - p0;
    const qreal len = std::hypot(d.x(), d.y());
    const qreal R = std::max(0.5, width / 2.0);
    const qreal border = std::clamp(width / 10.0, 0.1, 1.0);
    const QPointF dir = len > 0 ? d / len : QPointF(1, 0);
    const QPointF nd(-dir.y(), dir.x());
    // Semiancho del barrido medido en perpendicular al movimiento (mínimo medio píxel para que se vea).
    const qreal half = std::max(0.5, std::abs(n.x() * nd.x() + n.y() * nd.y()));
    const qreal det = d.x() * n.y() - d.y() * n.x();
    const QRect box = boundsOf(p0, p1, std::max(R, k) + 1).intersected(QRect(QPoint(0, 0), mask.size()));
    for (int y = box.top(); y <= box.bottom(); ++y)
        for (int x = box.left(); x <= box.right(); ++x) {
            const QPointF c(x + 0.5, y + 0.5);
            const QPointF v = c - p0;
            qreal cov;
            if (std::abs(det) < 1e-6 || len == 0) {
                // Movimiento paralelo a la plumilla (o un clic): queda la línea de la plumilla.
                const qreal nl = std::hypot(n.x(), n.y());
                const qreal dn = distToSegment(c, p0 - n, n / nl, 2 * nl);
                cov = sat(1.0 - dn);
            } else {
                const qreal t = (v.x() * n.y() - v.y() * n.x()) / det; // posición a lo largo del movimiento
                const qreal tol = 0.5 / len;
                if (t < -tol || t > 1 + tol) continue;
                const qreal sPerp = v.x() * nd.x() + v.y() * nd.y();
                cov = sat(0.5 + half - std::abs(sPerp));
            }
            if (cov <= 0) continue;
            cov *= capsuleCoverage(distToSegment(c, p0, dir, len), R + 1.0, border);
            if (cov > 0) mask.maxAt(x, y, int(cov * 255 + 0.5));
        }
    mask.touch(box);
    mask.grow(box);
}

void artisticSegment(CoverageMask &mask, Media m, QPointF p0, QPointF p1, qreal width, qreal arc)
{
    const qreal half = std::max(1.0, width / 2.0);
    const QPointF d = p1 - p0;
    const qreal len = std::hypot(d.x(), d.y());
    const QPointF dir = len > 0 ? d / len : QPointF(1, 0);
    const QRect box = boundsOf(p0, p1, half).intersected(QRect(QPoint(0, 0), mask.size()));
    for (int y = box.top(); y <= box.bottom(); ++y)
        for (int x = box.left(); x <= box.right(); ++x) {
            const QPointF c(x + 0.5, y + 0.5);
            qreal t;
            const qreal dist = distToSegment(c, p0, dir, len, &t);
            if (dist > half) continue;
            const QPointF ap = c - p0;
            const qreal side = (ap.x() * -dir.y() + ap.y() * dir.x()) >= 0 ? 1 : -1;
            const qreal cov = artisticCoverage(m, arc + std::clamp(t, 0.0, len), side * dist / half, x, y, width);
            if (cov > 0) mask.maxAt(x, y, int(cov * 255 + 0.5));
        }
    mask.touch(box);
    mask.grow(box);
}

void pencilSegment(CoverageMask &mask, QPointF p0, QPointF p1, int w)
{
    QRect touched;
    if (w <= 1) {
        bresenham(px(p0), px(p1), [&](int x, int y) { mask.maxAt(x, y, 255); touched |= QRect(x, y, 1, 1); });
    } else {
        const auto &stamp = roundStamp(w);
        const int off = w / 2;
        bresenham(px(p0), px(p1), [&](int x, int y) {
            for (int j = 0; j < w; ++j)
                for (int i = 0; i < w; ++i)
                    if (stamp[size_t(j) * w + i]) mask.maxAt(x - off + i, y - off + j, 255);
            touched |= QRect(x - off, y - off, w, w);
        });
    }
    mask.touch(touched);
    mask.grow(touched);
}

void squareSweep(CoverageMask &mask, QPointF p0, QPointF p1, int w)
{
    w = std::max(1, w);
    const int h = w / 2;
    const QPoint a = px(p0) - QPoint(h, h), b = px(p1) - QPoint(h, h);
    std::vector<QPointF> pts;
    for (QPoint q : {a, b})
        for (QPoint c : {QPoint(0, 0), QPoint(w, 0), QPoint(0, w), QPoint(w, w)})
            pts.push_back(QPointF(q + c));
    const QPolygonF hull = convexHull(pts);
    const QRect box = hull.boundingRect().toAlignedRect().intersected(QRect(QPoint(0, 0), mask.size()));
    for (int y = box.top(); y <= box.bottom(); ++y)
        for (int x = box.left(); x <= box.right(); ++x)
            if (insideConvex(QPointF(x + 0.5, y + 0.5), hull)) mask.maxAt(x, y, 255);
    mask.touch(box);
    mask.grow(box);
}

} // namespace

Media mediaFromId(const QString &id)
{
    if (id == QLatin1String("pencil")) return Media::Pencil;
    if (id == QLatin1String("calligraphyBrush")) return Media::CalligraphyBrush;
    if (id == QLatin1String("calligraphyPen")) return Media::CalligraphyPen;
    if (id == QLatin1String("airbrush")) return Media::Airbrush;
    if (id == QLatin1String("oilBrush")) return Media::Oil;
    if (id == QLatin1String("crayon")) return Media::Crayon;
    if (id == QLatin1String("marker")) return Media::Marker;
    if (id == QLatin1String("naturalPencil")) return Media::NaturalPencil;
    if (id == QLatin1String("watercolor")) return Media::Watercolor;
    if (id == QLatin1String("squareEraser")) return Media::SquareEraser;
    if (id == QLatin1String("roundEraser")) return Media::RoundEraser;
    return Media::Brush;
}

bool isArtistic(Media m)
{
    return m == Media::Oil || m == Media::Crayon || m == Media::Marker || m == Media::NaturalPencil
        || m == Media::Watercolor;
}

void CoverageMask::reset(QSize size)
{
    m_size = size;
    m_data.assign(size_t(size.width()) * size_t(size.height()), 0);
    m_dirty = QRect();
    m_bounds = QRect();
}

void segment(CoverageMask &mask, Media media, QPointF p0, QPointF p1, qreal width, qreal arc)
{
    switch (media) {
    case Media::Pencil:
        pencilSegment(mask, p0, p1, int(std::lround(width)));
        break;
    case Media::Brush:
    case Media::RoundEraser:
        capsule(mask, p0, p1, width);
        break;
    case Media::CalligraphyBrush:
    case Media::CalligraphyPen:
        calligraphy(mask, media == Media::CalligraphyBrush, p0, p1, width);
        break;
    case Media::Airbrush:
        airbrushPulse(mask, p1, width);
        break;
    case Media::SquareEraser:
        squareSweep(mask, p0, p1, int(std::lround(width)));
        break;
    default:
        artisticSegment(mask, media, p0, p1, width, arc);
        break;
    }
}

void airbrushPulse(CoverageMask &mask, QPointF center, qreal width)
{
    const int D = std::max(4, (int(std::lround(width)) + 1) & ~1);
    const int r = D / 2;
    const QPoint c = px(center);
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < 10; ++i) {
        int dx, dy;
        do {
            dx = int(rng->bounded(D + 1)) - r;
            dy = int(rng->bounded(D + 1)) - r;
        } while (dx * dx + dy * dy > r * r);
        mask.maxAt(c.x() + dx, c.y() + dy, 255);
    }
    const QRect box(c.x() - r, c.y() - r, D + 1, D + 1);
    mask.touch(box);
    mask.grow(box);
}

void composePaint(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area,
                  const QColor &color, qreal opacity)
{
    const QRect r = area.intersected(layer.rect());
    const QRgb c = color.rgba();
    const int ca = int(std::lround(qAlpha(c) * opacity));
    for (int y = r.top(); y <= r.bottom(); ++y) {
        const auto *src = reinterpret_cast<const QRgb *>(before.constScanLine(y));
        auto *dst = reinterpret_cast<QRgb *>(layer.scanLine(y));
        for (int x = r.left(); x <= r.right(); ++x) {
            const int m = mask.at(x, y);
            if (m == 0) { dst[x] = src[x]; continue; }
            const int a = (ca * m + 127) / 255; // alfa del trazo en este píxel
            if (a >= 255) { dst[x] = qPremultiply(c | 0xff000000u); continue; }
            const QRgb b = src[x];
            const int inv = 255 - a;
            const int rr = (qRed(c) * a + qRed(b) * inv + 127) / 255;
            const int gg = (qGreen(c) * a + qGreen(b) * inv + 127) / 255;
            const int bb = (qBlue(c) * a + qBlue(b) * inv + 127) / 255;
            const int aa = a + (qAlpha(b) * inv + 127) / 255;
            dst[x] = qRgba(rr, gg, bb, aa);
        }
    }
}

void composeErase(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area, qreal opacity)
{
    const QRect r = area.intersected(layer.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        const auto *src = reinterpret_cast<const QRgb *>(before.constScanLine(y));
        auto *dst = reinterpret_cast<QRgb *>(layer.scanLine(y));
        for (int x = r.left(); x <= r.right(); ++x) {
            const int m = mask.at(x, y);
            if (m == 0) { dst[x] = src[x]; continue; }
            const int keep = 255 - int(std::lround(m * opacity));
            const QRgb b = src[x];
            dst[x] = qRgba(qRed(b) * keep / 255, qGreen(b) * keep / 255, qBlue(b) * keep / 255, qAlpha(b) * keep / 255);
        }
    }
}

void composeRecolor(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area,
                    const QColor &from, const QColor &to)
{
    const QRect r = area.intersected(layer.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        const auto *src = reinterpret_cast<const QRgb *>(before.constScanLine(y));
        auto *dst = reinterpret_cast<QRgb *>(layer.scanLine(y));
        for (int x = r.left(); x <= r.right(); ++x) {
            const QRgb b = src[x];
            dst[x] = b;
            if (mask.at(x, y) == 0) continue;
            const int a = qAlpha(b);
            // |lienzo.rgb − from.rgb × lienzo.a| < 0.01 (premultiplicado, 0–1)
            const bool same = std::abs(qRed(b) - from.red() * a / 255) <= 2
                && std::abs(qGreen(b) - from.green() * a / 255) <= 2
                && std::abs(qBlue(b) - from.blue() * a / 255) <= 2 && a > 0;
            if (same) {
                const int na = a * to.alpha() / 255;
                dst[x] = qRgba(to.red() * na / 255, to.green() * na / 255, to.blue() * na / 255, na);
            }
        }
    }
}

void strokePath(CoverageMask &mask, Media media, const QPainterPath &path, qreal width)
{
    const QList<QPolygonF> polys = path.toSubpathPolygons();
    for (const QPolygonF &poly : polys) {
        if (poly.isEmpty()) continue;
        qreal arc = 0;
        if (poly.size() == 1) { segment(mask, media, poly[0], poly[0], width, 0); continue; }
        for (int i = 1; i < poly.size(); ++i) {
            const QPointF a = poly[i - 1], b = poly[i];
            if (media == Media::Airbrush) {
                const qreal len = std::hypot(b.x() - a.x(), b.y() - a.y());
                for (qreal t = 0; t < len; t += std::max(1.0, width / 3.0))
                    airbrushPulse(mask, a + (b - a) * (t / len), width);
            } else {
                segment(mask, media, a, b, width, arc);
            }
            arc += std::hypot(b.x() - a.x(), b.y() - a.y());
        }
    }
}

void fillPath(CoverageMask &mask, Media media, const QPainterPath &path)
{
    const QRect box = path.boundingRect().toAlignedRect().adjusted(-1, -1, 1, 1).intersected(QRect(QPoint(0, 0), mask.size()));
    if (box.isEmpty()) return;
    QImage cov(box.size(), QImage::Format_Alpha8);
    cov.fill(0);
    {
        QPainter p(&cov);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.translate(-box.topLeft());
        p.fillPath(path, Qt::black);
    }
    for (int y = 0; y < box.height(); ++y) {
        const uchar *line = cov.constScanLine(y);
        for (int x = 0; x < box.width(); ++x) {
            if (!line[x]) continue;
            const int gx = box.left() + x, gy = box.top() + y;
            const qreal t = isArtistic(media) ? fillTexture(media, gx, gy) : 1.0;
            mask.maxAt(gx, gy, int(line[x] * t + 0.5));
        }
    }
    mask.touch(box);
    mask.grow(box);
}

void renderSwatch(QImage &img, const QString &mediaId, const QPainterPath &path, const QColor &ink, qreal width)
{
    Media m = mediaFromId(mediaId);
    if (mediaId == QLatin1String("solid")) m = Media::Brush;
    if (isArtistic(m)) width = std::max(width, 8.0);
    CoverageMask mask;
    mask.reset(img.size());
    strokePath(mask, m, path, width);
    const QImage before = img.copy();
    composePaint(img, before, mask, img.rect(), ink, 1.0);
}

void renderFillSwatch(QImage &img, const QString &mediaId, const QColor &ink)
{
    const QRectF r(img.width() * 0.1, img.height() * 0.2, img.width() * 0.8, img.height() * 0.6);
    QPainterPath path;
    path.addRoundedRect(r, img.height() * 0.08, img.height() * 0.08);
    Media m = mediaFromId(mediaId);
    if (mediaId == QLatin1String("solid")) m = Media::Brush;
    CoverageMask mask;
    mask.reset(img.size());
    fillPath(mask, m, path);
    const QImage before = img.copy();
    composePaint(img, before, mask, img.rect(), ink, 1.0);
}

} // namespace mp::brushes
