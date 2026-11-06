#pragma once

#include "core/Document.h"

namespace mp::ops {

// Todas registran un paso en el historial.
void flipHorizontal(Document &doc);
void flipVertical(Document &doc);
void rotate90(Document &doc, bool clockwise);
void rotate180(Document &doc);

// Lienzo nuevo de `size`. La zona nueva queda transparente en todas las capas (se ve el color de fondo).
// `offset` = desplazamiento de la imagen.
void resizeCanvas(Document &doc, QSize size, QPoint offset = QPoint());

// Escala todas las capas a `size` con bicúbico de alta calidad.
void resample(Document &doc, QSize size);

// --- Diálogos de imagen ---
// Inclinar en grados enteros; válido si 1 ≤ |deg| ≤ 89 (0 o fuera de rango no hace nada).
bool isValidSkew(int deg);
void skewHorizontal(Document &doc, int deg);
void skewVertical(Document &doc, int deg);
// Girar a un ángulo arbitrario (positivo = horario). El lienzo pasa a la caja envolvente; lo descubierto queda
// transparente. Los múltiplos de 90 usan las rutas exactas.
void rotateBy(Document &doc, qreal degrees);

// Versiones sobre una sola imagen (sirven también para la selección flotante).
// Transformación afín genérica: (0,0)→tl, (w,0)→tr, (0,h)→bl sobre un destino transparente de `size`.
QImage affineTo(const QImage &src, QSize size, QPointF tl, QPointF tr, QPointF bl);
QImage skewedH(const QImage &src, int deg);
QImage skewedV(const QImage &src, int deg);
QImage rotated(const QImage &src, qreal degrees);
// Tamaño destino de un giro arbitrario (caja envolvente de las esquinas redondeadas).
QSize rotatedSize(QSize size, qreal degrees);
// Remuestreo bicúbico separable (Catmull-Rom con soporte ampliado al reducir), bordes en espejo.
QImage bicubicScaled(const QImage &src, QSize size);

// Capas.
void addLayer(Document &doc);            // encima de la activa
void duplicateLayer(Document &doc);
void deleteLayer(Document &doc);         // no borra la última
void moveLayer(Document &doc, int from, int to);
void mergeDown(Document &doc);
void setLayerVisible(Document &doc, int index, bool visible);
void setLayerOpacity(Document &doc, int index, qreal opacity);
void clearLayer(Document &doc, const QColor &fill);
void setBackground(Document &doc, const QColor &color, bool visible);

} // namespace mp::ops
