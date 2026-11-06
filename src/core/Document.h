#pragma once

#include <QColor>
#include <QImage>
#include <QObject>
#include <QRect>
#include <QString>
#include <vector>

namespace mp {

// Una capa de píxeles. La imagen siempre es ARGB32 premultiplicado y del tamaño del documento.
struct Layer {
    QString name;
    QImage image;
    bool visible = true;
    qreal opacity = 1.0;
};

class History;

// Documento: lista de capas (índice 0 = la de más abajo) más la capa activa.
class Document : public QObject {
    Q_OBJECT
public:
    explicit Document(QObject *parent = nullptr);
    ~Document() override;

    static constexpr QImage::Format kFormat = QImage::Format_ARGB32_Premultiplied;

    // Documento nuevo: capa 1 transparente sobre el color de fondo.
    void reset(QSize size, const QColor &background);
    void resetFromImage(const QImage &image);

    QSize size() const { return m_size; }
    QRect rect() const { return QRect(QPoint(0, 0), m_size); }

    int layerCount() const { return int(m_layers.size()); }
    Layer &layer(int i) { return m_layers.at(size_t(i)); }
    const Layer &layer(int i) const { return m_layers.at(size_t(i)); }
    std::vector<Layer> &layers() { return m_layers; }
    const std::vector<Layer> &layers() const { return m_layers; }

    int activeLayer() const { return m_active; }
    void setActiveLayer(int i);
    Layer &active() { return layer(m_active); }

    // Compone fondo (si está visible) y capas visibles en `out` dentro de `area`.
    void compose(QImage &out, const QRect &area) const;
    QImage flattened() const;
    // Igual pero sin el fondo (para copiar capas visibles).
    QImage flattenedLayers() const;

    // Color de fondo del documento (debajo de todas las capas); se puede ocultar.
    QColor background() const { return m_background; }
    bool backgroundVisible() const { return m_backgroundVisible; }
    void setBackground(const QColor &c, bool visible);

    // Resolución de la imagen en DPI.
    qreal dpi() const { return m_dpi; }
    void setDpi(qreal dpi) { m_dpi = dpi > 0 ? dpi : 96.0; }

    History *history() const { return m_history; }

    // Avisar a las vistas de que cambió una zona (coordenadas de imagen).
    void notifyChanged(const QRect &area);
    void notifyStructureChanged();

    // Reemplazos completos (los usa el historial y las operaciones de imagen).
    void replaceLayers(std::vector<Layer> layers, QSize size, int active);

signals:
    void changed(const QRect &area);
    void structureChanged();
    void sizeChanged(QSize size);

private:
    QSize m_size;
    std::vector<Layer> m_layers;
    int m_active = 0;
    QColor m_background = Qt::white;
    bool m_backgroundVisible = true;
    History *m_history = nullptr;
    qreal m_dpi = 96.0;
};

QImage makeLayerImage(QSize size, const QColor &fill = Qt::transparent);

} // namespace mp
