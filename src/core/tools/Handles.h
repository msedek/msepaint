#pragma once

#include <QPainter>
#include <QRectF>

namespace mp {

// Caja con 8 asas (esquinas y bordes) para mover y redimensionar: selección, formas y texto.
// Códigos: 1 mover, 2–5 bordes, 6–9 esquinas, 10 girar.
namespace handles {

enum Hit { None = 0, Move = 1, Top = 2, Right = 3, Bottom = 4, Left = 5,
           TopLeft = 6, TopRight = 7, BottomRight = 8, BottomLeft = 9, Rotate = 10 };

// Tamaño del asa en píxeles de pantalla.
constexpr qreal kHandleSize = 8.0;

QPointF handlePos(const QRectF &box, Hit h);
// Asa bajo `p` (coordenadas de imagen). `scale` = zoom. `withRotate` añade el asa de giro sobre el borde superior.
Hit hitTest(const QRectF &box, QPointF p, qreal scale, bool withRotate = false);
// Aplica el arrastre de un asa: `start` = caja al empezar, `delta` = desplazamiento del puntero.
// `keepAspect` mantiene la proporción (Shift). Tamaño mínimo 1 px.
QRectF drag(const QRectF &start, Hit h, QPointF delta, bool keepAspect);
// Dibuja el borde discontinuo y las asas.
void paint(QPainter &p, const QRectF &box, qreal scale, bool withRotate = false, bool dashed = true);
// Cursor sugerido para un asa (valor de Qt::CursorShape).
int cursorFor(Hit h);

} // namespace handles
} // namespace mp
