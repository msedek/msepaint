#include "core/tools/Text.h"
#include "core/Document.h"
#include "core/History.h"

#include <QAbstractTextDocumentLayout>
#include <QGuiApplication>
#include <QTextBlock>
#include <cmath>

namespace mp {

namespace {
constexpr qreal kDpi = 96.0;
qreal pxFromPt(qreal pt) { return pt * kDpi / 72.0; }
}

QTextCharFormat TextTool::charFormat() const
{
    QTextCharFormat f;
    QFont font = m_format.family.isEmpty() ? QGuiApplication::font() : QFont(m_format.family);
    font.setPixelSize(std::max(1, int(std::lround(pxFromPt(m_format.pointSize)))));
    font.setBold(m_format.bold);
    font.setItalic(m_format.italic);
    font.setUnderline(m_format.underline);
    font.setStrikeOut(m_format.strike);
    f.setFont(font);
    f.setForeground(m_color);
    return f;
}

void TextTool::applyCharFormat(const QTextCharFormat &f, bool whole)
{
    if (!m_doc) return;
    if (whole || !m_text.hasSelection()) {
        // Sin texto seleccionado, el formato se aplica a toda la caja y a lo que se escriba después.
        QTextCursor all(m_doc.get());
        all.select(QTextCursor::Document);
        all.mergeCharFormat(f);
        QTextCharFormat typing = m_text.charFormat();
        typing.merge(f);
        m_text.setCharFormat(typing);
    } else {
        m_text.mergeCharFormat(f);
    }
    QTextBlockFormat bf;
    bf.setAlignment(m_format.align);
    QTextCursor all(m_doc.get());
    all.select(QTextCursor::Document);
    all.mergeBlockFormat(bf);
    layoutDoc();
}

void TextTool::layoutDoc()
{
    if (!m_doc) return;
    m_doc->setTextWidth(std::max(1.0, m_box.width()));
    // La caja crece hacia abajo con el texto.
    const qreal h = m_doc->size().height();
    if (h > m_box.height()) m_box.setHeight(std::ceil(h));
}

void TextTool::startEditing(ToolContext &ctx)
{
    m_doc = std::make_unique<QTextDocument>();
    m_doc->setDocumentMargin(2);
    m_text = QTextCursor(m_doc.get());
    m_color = ctx.color1;
    m_background = ctx.color2;
    m_text.setCharFormat(charFormat());
    QTextBlockFormat bf;
    bf.setAlignment(m_format.align);
    m_text.setBlockFormat(bf);
    m_state = State::Editing;
    layoutDoc();
}

int TextTool::hitPosition(QPointF p) const
{
    if (!m_doc) return 0;
    return m_doc->documentLayout()->hitTest(p - m_box.topLeft(), Qt::FuzzyHit);
}

void TextTool::press(ToolContext &ctx, const ToolEvent &ev)
{
    if (ev.button != Qt::LeftButton) return;
    if (m_state == State::Editing) {
        const handles::Hit h = handles::hitTest(m_box, ev.pos, m_scale);
        const bool onHandle = h != handles::None && h != handles::Move;
        if (onHandle) {
            m_hit = h;
            m_dragStart = ev.pos;
            m_dragBox = m_box;
            return;
        }
        if (m_box.contains(ev.pos)) {
            const int pos = hitPosition(ev.pos);
            m_text.setPosition(pos, (ev.modifiers & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
            m_selecting = true;
            return;
        }
        // Borde de la caja: mover.
        const qreal r = 6.0 / m_scale;
        if (m_box.adjusted(-r, -r, r, r).contains(ev.pos)) {
            m_hit = handles::Move;
            m_dragStart = ev.pos;
            m_dragBox = m_box;
            return;
        }
        commit(ctx);
    }
    m_state = State::Creating;
    m_anchor = QPointF(std::floor(ev.pos.x()), std::floor(ev.pos.y()));
    m_box = QRectF(m_anchor, QSizeF(0, 0));
}

void TextTool::move(ToolContext &ctx, const ToolEvent &ev)
{
    if (m_state == State::Creating) {
        m_box = QRectF(m_anchor, QPointF(std::floor(ev.pos.x()), std::floor(ev.pos.y()))).normalized();
        return;
    }
    if (m_state == State::Editing && m_hit != handles::None) {
        m_box = handles::drag(m_dragBox, m_hit, ev.pos - m_dragStart, false);
        m_box = QRectF(std::round(m_box.x()), std::round(m_box.y()), std::round(m_box.width()), std::round(m_box.height()));
        layoutDoc();
        return;
    }
    if (m_state == State::Editing && m_selecting) {
        m_text.setPosition(hitPosition(ev.pos), QTextCursor::KeepAnchor);
        return;
    }
    hover(ctx, ev);
}

void TextTool::release(ToolContext &ctx, const ToolEvent &)
{
    if (m_state == State::Creating) {
        // Un clic simple crea una caja de tamaño por defecto.
        if (m_box.width() < 8 || m_box.height() < 8) {
            const qreal lineH = pxFromPt(m_format.pointSize) * 1.4 + 4;
            m_box = QRectF(m_anchor, QSizeF(std::max(160.0, lineH * 8), lineH));
        }
        m_box = m_box.intersected(QRectF(QPointF(0, 0), QSizeF(ctx.doc->size()) + QSizeF(4096, 4096)));
        startEditing(ctx);
        return;
    }
    m_hit = handles::None;
    m_selecting = false;
}

void TextTool::hover(ToolContext &, const ToolEvent &ev)
{
    if (m_state != State::Editing) { m_cursor = Qt::IBeamCursor; return; }
    const handles::Hit h = handles::hitTest(m_box, ev.pos, m_scale);
    if (h != handles::None && h != handles::Move) m_cursor = handles::cursorFor(h);
    else if (m_box.contains(ev.pos)) m_cursor = Qt::IBeamCursor;
    else {
        const qreal r = 6.0 / m_scale;
        m_cursor = m_box.adjusted(-r, -r, r, r).contains(ev.pos) ? Qt::SizeAllCursor : Qt::IBeamCursor;
    }
}

bool TextTool::key(ToolContext &ctx, int k, Qt::KeyboardModifiers mods, const QString &text)
{
    if (m_state != State::Editing) return false;
    const auto mode = (mods & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
    const bool ctrl = mods & Qt::ControlModifier;
    switch (k) {
    case Qt::Key_Escape: commit(ctx); return true;
    case Qt::Key_Backspace: m_text.deletePreviousChar(); break;
    case Qt::Key_Delete: m_text.deleteChar(); break;
    case Qt::Key_Return:
    case Qt::Key_Enter: m_text.insertBlock(); break;
    case Qt::Key_Left: m_text.movePosition(ctrl ? QTextCursor::WordLeft : QTextCursor::Left, mode); break;
    case Qt::Key_Right: m_text.movePosition(ctrl ? QTextCursor::WordRight : QTextCursor::Right, mode); break;
    case Qt::Key_Up: m_text.movePosition(QTextCursor::Up, mode); break;
    case Qt::Key_Down: m_text.movePosition(QTextCursor::Down, mode); break;
    case Qt::Key_Home: m_text.movePosition(ctrl ? QTextCursor::Start : QTextCursor::StartOfLine, mode); break;
    case Qt::Key_End: m_text.movePosition(ctrl ? QTextCursor::End : QTextCursor::EndOfLine, mode); break;
    case Qt::Key_A:
        if (ctrl) { m_text.select(QTextCursor::Document); break; }
        [[fallthrough]];
    default:
        if (ctrl || text.isEmpty() || !text.at(0).isPrint()) return false;
        m_text.insertText(text);
        break;
    }
    layoutDoc();
    return true;
}

void TextTool::insertText(const QString &text)
{
    if (m_state != State::Editing) return;
    m_text.insertText(text);
    layoutDoc();
}

void TextTool::setFormat(ToolContext &ctx, const TextFormat &f)
{
    const TextFormat old = m_format;
    m_format = f;
    m_color = ctx.color1;
    m_background = ctx.color2;
    if (m_state != State::Editing) return;
    QTextCharFormat cf;
    if (f.family != old.family) cf.setFontFamilies({f.family.isEmpty() ? QGuiApplication::font().family() : f.family});
    if (f.pointSize != old.pointSize) cf.setProperty(QTextFormat::FontPixelSize, int(std::lround(pxFromPt(f.pointSize))));
    if (f.bold != old.bold) cf.setFontWeight(f.bold ? QFont::Bold : QFont::Normal);
    if (f.italic != old.italic) cf.setFontItalic(f.italic);
    if (f.underline != old.underline) cf.setFontUnderline(f.underline);
    if (f.strike != old.strike) cf.setFontStrikeOut(f.strike);
    cf.setForeground(m_color);
    applyCharFormat(cf, false);
}

void TextTool::paintOverlay(QPainter &p, qreal scale) const
{
    if (m_state == State::Creating) {
        handles::paint(p, m_box, scale, false);
        return;
    }
    if (m_state != State::Editing || !m_doc) return;
    p.save();
    if (m_format.fillBackground) p.fillRect(m_box, m_background);
    p.translate(m_box.topLeft());
    p.setClipRect(QRectF(QPointF(0, 0), m_box.size()));
    QAbstractTextDocumentLayout::PaintContext pc;
    pc.cursorPosition = m_text.position();
    if (m_text.hasSelection()) {
        QAbstractTextDocumentLayout::Selection sel;
        sel.cursor = m_text;
        sel.format.setBackground(QColor(0, 120, 215));
        sel.format.setForeground(Qt::white);
        pc.selections << sel;
    }
    m_doc->documentLayout()->draw(&p, pc);
    p.restore();
    handles::paint(p, m_box, scale, false);
}

void TextTool::commit(ToolContext &ctx)
{
    if (m_state == State::Editing && m_doc && !m_doc->isEmpty()) {
        Document &doc = *ctx.doc;
        QImage before = doc.active().image;
        before.detach();
        {
            QPainter p(&doc.active().image);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setRenderHint(QPainter::TextAntialiasing, true);
            if (m_format.fillBackground) p.fillRect(m_box, m_background);
            p.translate(m_box.topLeft());
            p.setClipRect(QRectF(QPointF(0, 0), m_box.size()));
            QAbstractTextDocumentLayout::PaintContext pc;
            pc.cursorPosition = -1;
            m_doc->documentLayout()->draw(&p, pc);
        }
        doc.history()->pushLayerEdit(doc.activeLayer(), before, m_box.toAlignedRect().adjusted(-1, -1, 1, 1));
        doc.notifyChanged(m_box.toAlignedRect().adjusted(-1, -1, 1, 1));
    }
    m_state = State::None;
    m_doc.reset();
    m_hit = handles::None;
}

void TextTool::cancel(ToolContext &)
{
    m_state = State::None;
    m_doc.reset();
}

} // namespace mp
