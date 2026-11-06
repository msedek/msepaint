#include "core/Document.h"
#include "core/History.h"

#include <QPainter>

namespace mp {

QImage makeLayerImage(QSize size, const QColor &fill)
{
    QImage img(size, Document::kFormat);
    img.fill(fill);
    return img;
}

Document::Document(QObject *parent)
    : QObject(parent)
    , m_history(new History(this))
{
}

Document::~Document() = default;

void Document::reset(QSize size, const QColor &background)
{
    m_size = size;
    m_background = background;
    m_backgroundVisible = true;
    m_layers.clear();
    m_layers.push_back(Layer{QStringLiteral("Layer 1"), makeLayerImage(size), true, 1.0});
    m_active = 0;
    m_history->clear();
    emit sizeChanged(m_size);
    notifyStructureChanged();
}

void Document::resetFromImage(const QImage &image)
{
    m_size = image.size();
    m_background = Qt::white;
    m_backgroundVisible = true;
    m_layers.clear();
    m_layers.push_back(Layer{QStringLiteral("Layer 1"), image.convertToFormat(kFormat), true, 1.0});
    m_active = 0;
    m_history->clear();
    emit sizeChanged(m_size);
    notifyStructureChanged();
}

void Document::setActiveLayer(int i)
{
    if (i < 0 || i >= layerCount() || i == m_active)
        return;
    m_active = i;
    emit structureChanged();
}

void Document::compose(QImage &out, const QRect &area) const
{
    QPainter p(&out);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(area, m_backgroundVisible ? m_background : QColor(Qt::transparent));
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (const Layer &l : m_layers) {
        if (!l.visible || l.opacity <= 0.0)
            continue;
        p.setOpacity(l.opacity);
        p.drawImage(area.topLeft(), l.image, area);
    }
}

QImage Document::flattened() const
{
    QImage out(m_size, kFormat);
    compose(out, rect());
    return out;
}

QImage Document::flattenedLayers() const
{
    QImage out = makeLayerImage(m_size);
    QPainter p(&out);
    for (const Layer &l : m_layers) {
        if (!l.visible) continue;
        p.setOpacity(l.opacity);
        p.drawImage(0, 0, l.image);
    }
    return out;
}

void Document::setBackground(const QColor &c, bool visible)
{
    if (c == m_background && visible == m_backgroundVisible) return;
    m_background = c;
    m_backgroundVisible = visible;
    notifyStructureChanged();
}

void Document::notifyChanged(const QRect &area)
{
    emit changed(area.intersected(rect()));
}

void Document::notifyStructureChanged()
{
    emit structureChanged();
    emit changed(rect());
}

void Document::replaceLayers(std::vector<Layer> layers, QSize size, int active)
{
    const bool resized = size != m_size;
    m_layers = std::move(layers);
    m_size = size;
    m_active = qBound(0, active, layerCount() - 1);
    if (resized)
        emit sizeChanged(m_size);
    notifyStructureChanged();
}

} // namespace mp
