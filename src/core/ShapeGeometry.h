#pragma once

#include <QPainterPath>
#include <QString>
#include <QStringList>

namespace mp::shapes {

// Ids en el orden de la galería.
const QStringList &galleryOrder();

// Formas con plantilla (todas salvo line, curve y polygon).
bool hasTemplate(const QString &id);

// Plantilla en la caja −50…50. Vacía si no existe.
QPainterPath templatePath(const QString &id);

// Plantilla llevada a `box` (escala por eje y traslación al centro), con giro opcional en grados.
QPainterPath fitted(const QString &id, const QRectF &box, qreal angleDeg = 0.0);

// Datos SVG ("M x y L ...") del trayecto, para dibujarlo en QML con PathSvg.
QString toSvg(const QPainterPath &path);

// Restricción con Shift de la línea: horizontal, vertical o 45° (aritmética entera).
QPoint constrainLine(QPoint p0, QPoint p1);

} // namespace mp::shapes
