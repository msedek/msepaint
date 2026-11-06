#include "core/ImageOps.h"
#include "core/History.h"

#include <QPainter>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace mp::ops {

namespace {

template <typename F>
void transformAll(Document &doc, QSize newSize, F &&fn)
{
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    for (Layer &l : layers)
        l.image = fn(l.image).convertToFormat(Document::kFormat);
    doc.replaceLayers(std::move(layers), newSize, doc.activeLayer());
    doc.history()->pushSnapshot(std::move(before));
}

QString nextLayerName(const Document &doc)
{
    int n = 1;
    for (;;) {
        const QString name = QStringLiteral("Layer %1").arg(n);
        bool used = false;
        for (const Layer &l : doc.layers())
            used |= l.name == name;
        if (!used) return name;
        ++n;
    }
}

} // namespace

void flipHorizontal(Document &doc)
{
    transformAll(doc, doc.size(), [](const QImage &i) { return i.flipped(Qt::Horizontal); });
}

void flipVertical(Document &doc)
{
    transformAll(doc, doc.size(), [](const QImage &i) { return i.flipped(Qt::Vertical); });
}

void rotate90(Document &doc, bool clockwise)
{
    const QSize s = doc.size().transposed();
    transformAll(doc, s, [clockwise](const QImage &i) {
        return i.transformed(QTransform().rotate(clockwise ? 90 : -90));
    });
}

void rotate180(Document &doc)
{
    transformAll(doc, doc.size(), [](const QImage &i) { return i.flipped(Qt::Horizontal | Qt::Vertical); });
}

void resizeCanvas(Document &doc, QSize size, QPoint offset)
{
    size = size.expandedTo(QSize(1, 1));
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    for (size_t i = 0; i < layers.size(); ++i) {
        QImage out = makeLayerImage(size);
        QPainter p(&out);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(offset, layers[i].image);
        p.end();
        layers[i].image = out;
    }
    doc.replaceLayers(std::move(layers), size, doc.activeLayer());
    doc.history()->pushSnapshot(std::move(before));
}

void resample(Document &doc, QSize size)
{
    size = size.expandedTo(QSize(1, 1));
    if (size == doc.size()) return; // si el tamaño no cambia, no hace nada
    transformAll(doc, size, [size](const QImage &i) { return bicubicScaled(i, size); });
}

// --- Diálogos de imagen ---

QImage affineTo(const QImage &src, QSize size, QPointF tl, QPointF tr, QPointF bl)
{
    QImage out = makeLayerImage(size.expandedTo(QSize(1, 1)));
    if (src.isNull() || src.width() < 1 || src.height() < 1) return out;
    const qreal w = src.width(), h = src.height();
    const QTransform t((tr.x() - tl.x()) / w, (tr.y() - tl.y()) / w,
                       (bl.x() - tl.x()) / h, (bl.y() - tl.y()) / h,
                       tl.x(), tl.y());
    QPainter p(&out);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setTransform(t);
    p.drawImage(0, 0, src.convertToFormat(Document::kFormat));
    p.end();
    return out;
}

bool isValidSkew(int deg) { return std::abs(deg) >= 1 && std::abs(deg) <= 89; }

namespace {
int skewOffset(int deg, int length) { return int(std::lround(std::tan(deg * std::numbers::pi / 180.0) * length)); }
} // namespace

QImage skewedH(const QImage &src, int deg)
{
    if (!isValidSkew(deg)) return src;
    const int w = src.width(), h = src.height();
    const int off = skewOffset(deg, h);
    const int nw = w + std::abs(off);
    if (off >= 0) return affineTo(src, QSize(nw, h), QPointF(off, 0), QPointF(nw, 0), QPointF(0, h));
    return affineTo(src, QSize(nw, h), QPointF(0, 0), QPointF(w, 0), QPointF(-off, h));
}

QImage skewedV(const QImage &src, int deg)
{
    if (!isValidSkew(deg)) return src;
    const int w = src.width(), h = src.height();
    const int off = skewOffset(deg, w);
    const int nh = h + std::abs(off);
    // Ángulo positivo: el borde izquierdo baja (simétrico al horizontal).
    if (off >= 0) return affineTo(src, QSize(w, nh), QPointF(0, off), QPointF(w, 0), QPointF(0, nh));
    return affineTo(src, QSize(w, nh), QPointF(0, 0), QPointF(w, -off), QPointF(0, h));
}

namespace {
struct RotGeom { QSize size; QPointF tl, tr, bl; };
RotGeom rotGeom(QSize s, qreal degrees)
{
    const qreal cx = s.width() / 2.0, cy = s.height() / 2.0;
    QTransform t;
    t.translate(cx, cy);
    t.rotate(degrees); // con y hacia abajo: positivo = horario
    t.translate(-cx, -cy);
    auto mapI = [&t](qreal x, qreal y) { const QPointF p = t.map(QPointF(x, y)); return QPoint(int(std::lround(p.x())), int(std::lround(p.y()))); };
    const QPoint a = mapI(0, 0), b = mapI(s.width(), 0), c = mapI(0, s.height()), d = mapI(s.width(), s.height());
    const int minX = std::min({a.x(), b.x(), c.x(), d.x()}), maxX = std::max({a.x(), b.x(), c.x(), d.x()});
    const int minY = std::min({a.y(), b.y(), c.y(), d.y()}), maxY = std::max({a.y(), b.y(), c.y(), d.y()});
    const QPointF o(minX, minY);
    return {QSize(std::max(1, maxX - minX), std::max(1, maxY - minY)), QPointF(a) - o, QPointF(b) - o, QPointF(c) - o};
}
qreal normDeg(qreal d) { d = std::fmod(d, 360.0); return d < 0 ? d + 360.0 : d; }
} // namespace

QSize rotatedSize(QSize size, qreal degrees) { return rotGeom(size, degrees).size; }

QImage rotated(const QImage &src, qreal degrees)
{
    const qreal n = normDeg(degrees);
    if (qFuzzyIsNull(n) || qFuzzyCompare(n, 360.0)) return src;
    if (qFuzzyCompare(n, 90.0)) return src.transformed(QTransform().rotate(90)).convertToFormat(Document::kFormat);
    if (qFuzzyCompare(n, 180.0)) return src.flipped(Qt::Horizontal | Qt::Vertical).convertToFormat(Document::kFormat);
    if (qFuzzyCompare(n, 270.0)) return src.transformed(QTransform().rotate(-90)).convertToFormat(Document::kFormat);
    const RotGeom g = rotGeom(src.size(), degrees);
    return affineTo(src, g.size, g.tl, g.tr, g.bl);
}

void skewHorizontal(Document &doc, int deg)
{
    if (!isValidSkew(deg)) return;
    const QSize s(doc.size().width() + std::abs(skewOffset(deg, doc.size().height())), doc.size().height());
    transformAll(doc, s, [deg](const QImage &i) { return skewedH(i, deg); });
}

void skewVertical(Document &doc, int deg)
{
    if (!isValidSkew(deg)) return;
    const QSize s(doc.size().width(), doc.size().height() + std::abs(skewOffset(deg, doc.size().width())));
    transformAll(doc, s, [deg](const QImage &i) { return skewedV(i, deg); });
}

void rotateBy(Document &doc, qreal degrees)
{
    const qreal n = normDeg(degrees);
    if (qFuzzyIsNull(n) || qFuzzyCompare(n, 360.0)) return;
    if (qFuzzyCompare(n, 90.0)) return rotate90(doc, true);
    if (qFuzzyCompare(n, 180.0)) return rotate180(doc);
    if (qFuzzyCompare(n, 270.0)) return rotate90(doc, false);
    transformAll(doc, rotatedSize(doc.size(), degrees), [degrees](const QImage &i) { return rotated(i, degrees); });
}

namespace {

// Núcleo cúbico de Keys (a = −0,5), bicúbico clásico.
inline double cubic(double x)
{
    x = std::abs(x);
    if (x < 1.0) return (1.5 * x - 2.5) * x * x + 1.0;
    if (x < 2.0) return ((-0.5 * x + 2.5) * x - 4.0) * x + 2.0;
    return 0.0;
}

// Índice con borde en espejo (TileFlipXY: el píxel del borde se repite).
inline int mirror(int i, int n)
{
    if (n == 1) return 0;
    const int period = 2 * n;
    i %= period;
    if (i < 0) i += period;
    return i < n ? i : period - 1 - i;
}

struct Taps { int first = 0; std::vector<int> idx; std::vector<float> w; };

// Pesos por píxel destino para un eje: centros de píxel con desplazamiento de medio píxel (PixelOffsetMode HighQuality).
std::vector<Taps> axisTaps(int srcN, int dstN)
{
    std::vector<Taps> out(static_cast<size_t>(dstN));
    const double scale = double(dstN) / srcN;
    const double support = scale < 1.0 ? 2.0 / scale : 2.0; // al reducir se ensancha el filtro (prefiltrado)
    const double fscale = scale < 1.0 ? scale : 1.0;
    for (int d = 0; d < dstN; ++d) {
        const double center = (d + 0.5) / scale - 0.5;
        const int lo = int(std::floor(center - support)) + 1, hi = int(std::floor(center + support));
        Taps &t = out[size_t(d)];
        double sum = 0;
        for (int s = lo; s <= hi; ++s) {
            const double wgt = cubic((s - center) * fscale);
            if (wgt == 0.0) continue;
            t.idx.push_back(mirror(s, srcN));
            t.w.push_back(float(wgt));
            sum += wgt;
        }
        if (sum != 0.0) for (float &x : t.w) x = float(x / sum);
    }
    return out;
}

} // namespace

QImage bicubicScaled(const QImage &srcIn, QSize size)
{
    size = size.expandedTo(QSize(1, 1));
    const QImage src = srcIn.convertToFormat(Document::kFormat);
    if (src.isNull()) return makeLayerImage(size);
    if (src.size() == size) return src;
    const int sw = src.width(), sh = src.height(), dw = size.width(), dh = size.height();
    const std::vector<Taps> tx = axisTaps(sw, dw), ty = axisTaps(sh, dh);

    // Pasada horizontal: sh filas × dw columnas, 4 canales premultiplicados en float.
    std::vector<float> mid(size_t(sh) * size_t(dw) * 4);
    for (int y = 0; y < sh; ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        float *o = mid.data() + size_t(y) * size_t(dw) * 4;
        for (int x = 0; x < dw; ++x, o += 4) {
            const Taps &t = tx[size_t(x)];
            float a = 0, r = 0, g = 0, b = 0;
            for (size_t k = 0; k < t.idx.size(); ++k) {
                const QRgb p = row[t.idx[k]];
                const float w = t.w[k];
                a += w * qAlpha(p); r += w * qRed(p); g += w * qGreen(p); b += w * qBlue(p);
            }
            o[0] = a; o[1] = r; o[2] = g; o[3] = b;
        }
    }

    QImage out(size, Document::kFormat);
    for (int y = 0; y < dh; ++y) {
        const Taps &t = ty[size_t(y)];
        QRgb *row = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < dw; ++x) {
            float a = 0, r = 0, g = 0, b = 0;
            for (size_t k = 0; k < t.idx.size(); ++k) {
                const float *p = mid.data() + (size_t(t.idx[k]) * size_t(dw) + size_t(x)) * 4;
                const float w = t.w[k];
                a += w * p[0]; r += w * p[1]; g += w * p[2]; b += w * p[3];
            }
            const int ia = std::clamp(int(std::lround(a)), 0, 255);
            auto ch = [ia](float v) { return std::clamp(int(std::lround(v)), 0, ia); }; // premultiplicado: canal ≤ alfa
            row[x] = qRgba(ch(r), ch(g), ch(b), ia);
        }
    }
    return out;
}

void addLayer(Document &doc)
{
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    const int at = doc.activeLayer() + 1;
    layers.insert(layers.begin() + at, Layer{nextLayerName(doc), makeLayerImage(doc.size()), true, 1.0});
    doc.replaceLayers(std::move(layers), doc.size(), at);
    doc.history()->pushSnapshot(std::move(before));
}

void duplicateLayer(Document &doc)
{
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    const int at = doc.activeLayer() + 1;
    Layer copy = layers[size_t(doc.activeLayer())];
    copy.image.detach();
    copy.name = nextLayerName(doc);
    layers.insert(layers.begin() + at, copy);
    doc.replaceLayers(std::move(layers), doc.size(), at);
    doc.history()->pushSnapshot(std::move(before));
}

void deleteLayer(Document &doc)
{
    if (doc.layerCount() <= 1) return;
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    const int at = doc.activeLayer();
    layers.erase(layers.begin() + at);
    doc.replaceLayers(std::move(layers), doc.size(), std::max(0, at - 1));
    doc.history()->pushSnapshot(std::move(before));
}

void moveLayer(Document &doc, int from, int to)
{
    const int n = doc.layerCount();
    if (from == to || from < 0 || to < 0 || from >= n || to >= n) return;
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    Layer l = layers[size_t(from)];
    layers.erase(layers.begin() + from);
    layers.insert(layers.begin() + to, l);
    int active = doc.activeLayer();
    if (active == from) active = to;
    else if (from < active && to >= active) --active;
    else if (from > active && to <= active) ++active;
    doc.replaceLayers(std::move(layers), doc.size(), active);
    doc.history()->pushSnapshot(std::move(before));
}

void mergeDown(Document &doc)
{
    const int at = doc.activeLayer();
    if (at <= 0) return;
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    Layer &below = layers[size_t(at - 1)];
    const Layer &top = layers[size_t(at)];
    below.image.detach();
    if (top.visible) {
        QPainter p(&below.image);
        p.setOpacity(top.opacity);
        p.drawImage(0, 0, top.image);
    }
    layers.erase(layers.begin() + at);
    doc.replaceLayers(std::move(layers), doc.size(), at - 1);
    doc.history()->pushSnapshot(std::move(before));
}

void setLayerVisible(Document &doc, int index, bool visible)
{
    if (index < 0 || index >= doc.layerCount() || doc.layer(index).visible == visible) return;
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    layers[size_t(index)].visible = visible;
    doc.replaceLayers(std::move(layers), doc.size(), doc.activeLayer());
    doc.history()->pushSnapshot(std::move(before));
}

void setLayerOpacity(Document &doc, int index, qreal opacity)
{
    if (index < 0 || index >= doc.layerCount()) return;
    DocumentState before = DocumentState::capture(doc);
    std::vector<Layer> layers = doc.layers();
    layers[size_t(index)].opacity = qBound(0.0, opacity, 1.0);
    doc.replaceLayers(std::move(layers), doc.size(), doc.activeLayer());
    doc.history()->pushSnapshot(std::move(before));
}

void clearLayer(Document &doc, const QColor &fill)
{
    QImage before = doc.active().image;
    before.detach();
    doc.active().image.fill(fill);
    doc.history()->pushLayerEdit(doc.activeLayer(), before);
    doc.notifyChanged(doc.rect());
}

void setBackground(Document &doc, const QColor &color, bool visible)
{
    if (color == doc.background() && visible == doc.backgroundVisible()) return;
    DocumentState before = DocumentState::capture(doc);
    doc.setBackground(color, visible);
    doc.history()->pushSnapshot(std::move(before));
}

} // namespace mp::ops
