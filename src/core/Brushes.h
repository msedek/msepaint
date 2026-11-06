#pragma once

#include <QColor>
#include <QImage>
#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QString>
#include <vector>

// Motor de trazos. Todo trazo acumula una máscara de cobertura (máximo por píxel)
// y la capa se recompone desde su copia previa: así el trazo no se oscurece al cruzarse consigo mismo.
namespace mp::brushes {

enum class Media {
    Pencil,          // duro, sello redondo sin AA
    Brush,           // cápsula con borde suave
    CalligraphyBrush,// plumilla 45° «/»
    CalligraphyPen,  // plumilla 135° «\»
    Airbrush,        // rociado
    Oil,
    Crayon,
    Marker,
    NaturalPencil,
    Watercolor,
    SquareEraser,    // barrido cuadrado sin AA
    RoundEraser,     // cápsula con borde suave
};

Media mediaFromId(const QString &id);
bool isArtistic(Media m);

// Cobertura 0..255 del tamaño del documento.
class CoverageMask {
public:
    void reset(QSize size);
    QSize size() const { return m_size; }
    QRect dirty() const { return m_dirty; }
    void clearDirty() { m_dirty = QRect(); }
    inline void maxAt(int x, int y, int v)
    {
        if (x < 0 || y < 0 || x >= m_size.width() || y >= m_size.height() || v <= 0) return;
        uint8_t &a = m_data[size_t(y) * size_t(m_size.width()) + size_t(x)];
        if (v > a) a = uint8_t(v > 255 ? 255 : v);
    }
    uint8_t at(int x, int y) const { return m_data[size_t(y) * size_t(m_size.width()) + size_t(x)]; }
    void touch(const QRect &r) { m_dirty |= r.intersected(QRect(QPoint(0, 0), m_size)); }
    QRect bounds() const { return m_bounds; }
    void grow(const QRect &r) { m_bounds |= r.intersected(QRect(QPoint(0, 0), m_size)); }
private:
    QSize m_size;
    std::vector<uint8_t> m_data;
    QRect m_dirty, m_bounds;
};

// Dibuja en la máscara un segmento del trazo. `arc` = longitud de arco acumulada al inicio del segmento
// (la usan las texturas de los pinceles artísticos). Un punto aislado es un segmento p → p.
void segment(CoverageMask &mask, Media media, QPointF p0, QPointF p1, qreal width, qreal arc = 0.0);

// Rociado del aerógrafo: 10 píxeles al azar dentro del disco de diámetro max(4, par(width+1)).
void airbrushPulse(CoverageMask &mask, QPointF center, qreal width);

// Recompone `layer` en `area` a partir de `before` y la máscara.
void composePaint(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area,
                  const QColor &color, qreal opacity);
void composeErase(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area, qreal opacity);
// Borrador con botón derecho: dentro de la máscara, los píxeles exactamente iguales a `from` pasan a `to`.
void composeRecolor(QImage &layer, const QImage &before, const CoverageMask &mask, const QRect &area,
                    const QColor &from, const QColor &to);

// Traza un trayecto (contorno de forma o muestra) aplanado con planitud 0.25.
void strokePath(CoverageMask &mask, Media media, const QPainterPath &path, qreal width);
// Relleno texturizado de una región con un medio artístico.
void fillPath(CoverageMask &mask, Media media, const QPainterPath &path);

// Muestras para los menús.
void renderSwatch(QImage &img, const QString &mediaId, const QPainterPath &path, const QColor &ink, qreal width);
void renderFillSwatch(QImage &img, const QString &mediaId, const QColor &ink);

} // namespace mp::brushes
