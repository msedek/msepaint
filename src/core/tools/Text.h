#pragma once

#include "core/Tool.h"
#include "core/tools/Handles.h"

#include <QTextCursor>
#include <QTextDocument>
#include <memory>

namespace mp {

struct TextFormat {
    QString family;        // vacío = fuente por defecto
    qreal pointSize = 11;
    bool bold = false, italic = false, underline = false, strike = false;
    Qt::Alignment align = Qt::AlignLeft;
    bool fillBackground = false;
};

// Caja de texto editable que se rasteriza sobre la capa al confirmar.
class TextTool : public Tool {
public:
    QString id() const override { return QStringLiteral("text"); }

    void press(ToolContext &ctx, const ToolEvent &ev) override;
    void move(ToolContext &ctx, const ToolEvent &ev) override;
    void release(ToolContext &ctx, const ToolEvent &ev) override;
    void hover(ToolContext &ctx, const ToolEvent &ev) override;
    void commit(ToolContext &ctx) override;
    void cancel(ToolContext &ctx) override;
    bool hasPending() const override { return m_state != State::None; }
    void paintOverlay(QPainter &p, qreal scale) const override;

    bool editing() const { return m_state == State::Editing; }
    // Teclado. Devuelve true si consumió la tecla.
    bool key(ToolContext &ctx, int key, Qt::KeyboardModifiers mods, const QString &text);
    void insertText(const QString &text);

    TextFormat format() const { return m_format; }
    void setFormat(ToolContext &ctx, const TextFormat &f); // aplica a la selección o a todo el texto
    void setScale(qreal s) { m_scale = s; }
    int cursorShape() const { return m_cursor; }
    QRectF box() const { return m_box; }

private:
    enum class State { None, Creating, Editing };
    void startEditing(ToolContext &ctx);
    void layoutDoc();
    int hitPosition(QPointF p) const;
    QTextCharFormat charFormat() const;
    void applyCharFormat(const QTextCharFormat &f, bool whole);

    State m_state = State::None;
    QRectF m_box;
    QPointF m_anchor;
    std::unique_ptr<QTextDocument> m_doc;
    QTextCursor m_text;
    TextFormat m_format;
    QColor m_color, m_background;
    handles::Hit m_hit = handles::None;
    QPointF m_dragStart;
    QRectF m_dragBox;
    bool m_selecting = false;
    qreal m_scale = 1.0;
    int m_cursor = Qt::IBeamCursor;
};

} // namespace mp
