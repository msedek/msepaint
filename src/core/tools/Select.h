#pragma once

#include "core/Tool.h"
#include "core/tools/Handles.h"

#include <QPainterPath>
#include <QPolygonF>

namespace mp {

// Selección rectangular o libre con contenido flotante.
class SelectTool : public Tool {
public:
    QString id() const override { return QStringLiteral("select"); }

    bool freeForm = false;           // modo de creación
    bool transparent = false;        // «Transparent selection»: el color 2 del flotante no se compone

    void press(ToolContext &ctx, const ToolEvent &ev) override;
    void move(ToolContext &ctx, const ToolEvent &ev) override;
    void release(ToolContext &ctx, const ToolEvent &ev) override;
    void hover(ToolContext &ctx, const ToolEvent &ev) override;
    void commit(ToolContext &ctx) override;
    void cancel(ToolContext &ctx) override;
    bool hasPending() const override { return hasSelection(); }
    void paintOverlay(QPainter &p, qreal scale) const override;

    bool hasSelection() const { return m_state != State::None; }
    QSize selectionSize() const { return m_target.toAlignedRect().size(); }
    QRectF target() const { return m_target; }
    int cursorShape() const { return m_cursor; }
    void setScale(qreal s) { m_scale = s; }

    // Operaciones del menú y la cinta.
    void selectAll(ToolContext &ctx);
    void invert(ToolContext &ctx);
    void deleteContent(ToolContext &ctx);
    QImage copyContent(ToolContext &ctx) const;   // contenido de la selección (caja; fuera de la región, transparente)
    void cut(ToolContext &ctx, QImage *out);
    void paste(ToolContext &ctx, const QImage &img, QPoint at);
    void crop(ToolContext &ctx);
    void transformFloat(ToolContext &ctx, const QTransform &t); // giro/volteo del contenido flotante
    void resizeFloat(ToolContext &ctx, QSize size);
    void invertColors(ToolContext &ctx);
    void nudge(ToolContext &ctx, QPoint delta); // flechas del teclado

private:
    enum class State { None, Creating, Marquee, Floating };
    void lift(ToolContext &ctx);
    QImage regionMask(const QRect &box) const; // máscara 0/255 de la región en `box`
    QImage preparedFloat(const QColor &color2) const;
    void clear();

    State m_state = State::None;
    QPainterPath m_region;     // región seleccionada (coordenadas de imagen)
    QPolygonF m_lasso;         // puntos durante la creación libre
    QPointF m_anchor;          // esquina inicial al crear
    QRectF m_target;           // caja donde se dibuja la selección / el flotante
    QImage m_float;            // contenido levantado (tamaño de la caja inicial)
    QImage m_beforeLift;       // capa antes de levantar (para historial y cancelar)
    int m_layer = -1;
    handles::Hit m_hit = handles::None;
    QPointF m_dragStart;
    QRectF m_dragBox;
    qreal m_scale = 1.0;
    int m_cursor = Qt::CrossCursor;
};

} // namespace mp
