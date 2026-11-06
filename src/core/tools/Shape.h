#pragma once

#include "core/Tool.h"
#include "core/tools/Handles.h"

#include <QPainterPath>
#include <QPolygonF>

namespace mp {

// Formas, línea, curva y polígono.
class ShapeTool : public Tool {
public:
    QString id() const override { return QStringLiteral("shape"); }

    QString shape = QStringLiteral("line");
    QString outline = QStringLiteral("solid");  // none, solid, crayon, marker, oilBrush, naturalPencil, watercolor
    QString fill = QStringLiteral("none");

    void press(ToolContext &ctx, const ToolEvent &ev) override;
    void move(ToolContext &ctx, const ToolEvent &ev) override;
    void release(ToolContext &ctx, const ToolEvent &ev) override;
    void hover(ToolContext &ctx, const ToolEvent &ev) override;
    void commit(ToolContext &ctx) override;
    void cancel(ToolContext &ctx) override;
    bool hasPending() const override { return m_state != State::None; }
    void paintOverlay(QPainter &p, qreal scale) const override;

    // Cambió contorno, relleno, grosor o colores: redibujar la forma en edición.
    void restyle(ToolContext &ctx);
    void setScale(qreal s) { m_scale = s; }
    int cursorShape() const { return m_cursor; }
    void doubleClick(ToolContext &ctx);

private:
    enum class State { None, Creating, Editing, CurveCtrl1, CurveCtrl2, Polygon };
    bool isLine() const { return shape == QLatin1String("line"); }
    bool isCurve() const { return shape == QLatin1String("curve"); }
    bool isPolygon() const { return shape == QLatin1String("polygon"); }
    QPainterPath currentPath() const;
    void begin(ToolContext &ctx, const ToolEvent &ev);
    void render(ToolContext &ctx);
    QPointF toLocal(QPointF p) const; // deshace el giro alrededor del centro

    State m_state = State::None;
    int m_layer = -1;
    QImage m_before;
    QRect m_lastDirty;
    QColor m_outlineColor, m_fillColor;
    // Plantillas
    QPointF m_anchor;
    QRectF m_box;
    qreal m_angle = 0;
    // Línea / curva / polígono
    QPointF m_p0, m_p1, m_c1, m_c2;
    QPolygonF m_poly;
    bool m_closed = false;
    // Arrastre de asas
    handles::Hit m_hit = handles::None;
    int m_endpoint = -1; // línea: 0 = P0, 1 = P1
    QPointF m_dragStart;
    QRectF m_dragBox;
    qreal m_dragAngle = 0;
    qreal m_scale = 1.0;
    int m_cursor = Qt::CrossCursor;
};

} // namespace mp
