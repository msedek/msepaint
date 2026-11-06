#pragma once

#include "core/Brushes.h"
#include "core/Tool.h"

namespace mp {

// Herramienta de trazo genérica: lápiz, pinceles, aerógrafo y borrador.
class StrokeTool : public Tool {
public:
    enum class Kind { Paint, Eraser };
    StrokeTool(QString id, brushes::Media media, Kind kind = Kind::Paint)
        : m_id(std::move(id)), m_media(media), m_kind(kind) {}

    QString id() const override { return m_id; }
    void setMedia(brushes::Media m) { m_media = m; }
    brushes::Media media() const { return m_media; }

    void press(ToolContext &ctx, const ToolEvent &ev) override;
    void move(ToolContext &ctx, const ToolEvent &ev) override;
    void release(ToolContext &ctx, const ToolEvent &ev) override;
    void hover(ToolContext &ctx, const ToolEvent &ev) override;
    void paintOverlay(QPainter &p, qreal scale) const override;

    // Aerógrafo: un pulso de rociado (lo llama un temporizador mientras el botón está apretado).
    void tick(ToolContext &ctx);
    bool wantsTicks() const { return m_media == brushes::Media::Airbrush && m_active; }

private:
    QPointF snap(QPointF p) const;
    void apply(ToolContext &ctx, const QRect &dirty);
    QPointF constrain(QPointF p, Qt::KeyboardModifiers mods);

    QString m_id;
    brushes::Media m_media;
    Kind m_kind;
    bool m_active = false;
    int m_layer = -1;
    QImage m_before;
    brushes::CoverageMask m_mask;
    QPointF m_last, m_start;
    qreal m_arc = 0;
    QColor m_color;
    bool m_secondary = false;
    int m_shiftDir = -1; // 0 horizontal, 1 vertical, 2 «\», 3 «/»
    QPointF m_hover{-1, -1};
    int m_hoverSize = 1;
};

} // namespace mp
