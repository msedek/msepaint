#include "ui/ImageProviders.h"
#include "ui/AppController.h"
#include "core/Brushes.h"

#include <QPainter>
#include <QPainterPath>

QImage SwatchProvider::requestImage(const QString &id, QSize *size, const QSize &requested)
{
    const QSize s = requested.isValid() ? requested : QSize(240, 80);
    if (size) *size = s;
    const QString media = id.section(QLatin1Char('?'), 0, 0);
    const QString opts = id.section(QLatin1Char('?'), 1);
    const bool dark = opts.contains(QLatin1String("dark"));
    const bool fill = opts.startsWith(QLatin1String("fill"));
    const bool outline = opts.startsWith(QLatin1String("outline"));
    const QColor ink = dark ? QColor(255, 255, 255) : QColor(0, 0, 0);

    QImage img(s, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    if (media == QLatin1String("none")) {
        // "Sin contorno/relleno": rectángulo de trazo fino discontinuo.
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(ink, s.height() / 40.0, Qt::DashLine));
        p.drawRect(QRectF(s.width() * 0.1, s.height() * 0.2, s.width() * 0.8, s.height() * 0.6));
        return img;
    }
    if (fill) {
        mp::brushes::renderFillSwatch(img, media, ink);
        return img;
    }
    // Curva en S que cruza la muestra.
    QPainterPath path;
    const qreal w = s.width(), h = s.height();
    path.moveTo(w * 0.08, h * 0.7);
    path.cubicTo(w * 0.35, h * 0.05, w * 0.6, h * 0.95, w * 0.92, h * 0.3);
    mp::brushes::renderSwatch(img, outline && media == QLatin1String("solid") ? QStringLiteral("brush") : media,
                              path, ink, h / 6.0);
    return img;
}

QImage LayerThumbProvider::requestImage(const QString &id, QSize *size, const QSize &requested)
{
    const int index = id.section(QLatin1Char('/'), 0, 0).toInt();
    mp::Document *doc = m_app->document();
    if (index < 0 || index >= doc->layerCount()) {
        if (size) *size = QSize(1, 1);
        return QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
    }
    const QSize box = requested.isValid() ? requested : QSize(80, 80);
    QImage thumb = doc->layer(index).image.scaled(box, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size) *size = thumb.size();
    return thumb;
}

QImage CompositeProvider::requestImage(const QString &, QSize *size, const QSize &requested)
{
    QImage img = m_app->document()->flattened();
    if (requested.isValid() && !requested.isEmpty())
        img = img.scaled(requested, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size) *size = img.size();
    return img;
}
