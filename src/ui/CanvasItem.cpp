#include "ui/CanvasItem.h"
#include "ui/AppController.h"
#include "core/Document.h"

#include <QCursor>
#include <QInputMethodEvent>
#include <QPainter>
#include <cmath>

CanvasItem::CanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    setActiveFocusOnTab(true);
    setCursor(Qt::CrossCursor);
    setAntialiasing(false);
    setFillColor(Qt::transparent);
}

void CanvasItem::setController(AppController *app)
{
    if (m_app == app) return;
    if (m_app) disconnect(m_app, nullptr, this, nullptr);
    m_app = app;
    if (m_app) {
        mp::Document *doc = m_app->document();
        connect(doc, &mp::Document::changed, this, &CanvasItem::onDocChanged);
        connect(doc, &mp::Document::sizeChanged, this, &CanvasItem::updateGeometryFromDoc);
        connect(m_app, &AppController::zoomChanged, this, &CanvasItem::updateGeometryFromDoc);
        connect(m_app, &AppController::overlayChanged, this, [this] { update(); });
        connect(m_app, &AppController::cursorShapeChanged, this, &CanvasItem::updateCursor);
        updateGeometryFromDoc();
        onDocChanged(doc->rect());
    }
    emit controllerChanged();
}

void CanvasItem::updateCursor()
{
    if (m_app) setCursor(QCursor(Qt::CursorShape(m_app->cursorShape())));
}

void CanvasItem::updateGeometryFromDoc()
{
    if (!m_app) return;
    const QSize s = m_app->document()->size();
    const qreal z = m_app->zoom();
    setImplicitSize(std::round(s.width() * z), std::round(s.height() * z));
    update();
}

void CanvasItem::onDocChanged(const QRect &area)
{
    mp::Document *doc = m_app->document();
    if (m_composite.size() != doc->size())
        m_composite = QImage(doc->size(), mp::Document::kFormat);
    const QRect r = area.isValid() ? area.intersected(doc->rect()) : doc->rect();
    if (!r.isEmpty())
        doc->compose(m_composite, r);
    update();
}

void CanvasItem::paint(QPainter *p)
{
    if (!m_app || m_composite.isNull()) return;
    const qreal z = m_app->zoom();
    const QRectF target(0, 0, m_composite.width() * z, m_composite.height() * z);

    // Cuadrícula de transparencia bajo el contenido.
    static const QImage checker = [] {
        QImage c(16, 16, QImage::Format_RGB32);
        c.fill(Qt::white);
        QPainter cp(&c);
        cp.fillRect(0, 0, 8, 8, QColor(0xCC, 0xCC, 0xCC));
        cp.fillRect(8, 8, 8, 8, QColor(0xCC, 0xCC, 0xCC));
        return c;
    }();
    p->fillRect(target, QBrush(checker));

    // Píxeles nítidos al ampliar; suavizado al reducir.
    p->setRenderHint(QPainter::SmoothPixmapTransform, z < 1.0);
    p->drawImage(target, m_composite);

    p->save();
    p->scale(z, z);
    m_app->paintOverlay(*p, z);
    p->restore();

    if (m_app->showGridlines() && z >= 2.0) {
        p->setPen(QPen(QColor(0, 0, 0, 40), 0));
        for (int x = 1; x < m_composite.width(); ++x)
            p->drawLine(QPointF(x * z, 0), QPointF(x * z, target.height()));
        for (int y = 1; y < m_composite.height(); ++y)
            p->drawLine(QPointF(0, y * z), QPointF(target.width(), y * z));
    }
}

QPointF CanvasItem::toImage(QPointF itemPos) const
{
    const qreal z = m_app->zoom();
    return itemPos / z;
}

void CanvasItem::mousePressEvent(QMouseEvent *e)
{
    if (!m_app || m_dragButton != Qt::NoButton) { e->accept(); return; }
    forceActiveFocus(Qt::MouseFocusReason);
    m_dragButton = e->button();
    m_app->pointerPress(toImage(e->position()), e->button(), e->modifiers());
    e->accept();
}

void CanvasItem::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_app) return;
    m_app->pointerMove(toImage(e->position()), m_dragButton, e->modifiers());
}

void CanvasItem::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_app || e->button() != m_dragButton) return;
    m_app->pointerRelease(toImage(e->position()), m_dragButton, e->modifiers());
    m_dragButton = Qt::NoButton;
}

void CanvasItem::hoverMoveEvent(QHoverEvent *e)
{
    if (m_app) m_app->pointerHover(toImage(e->position()), e->modifiers());
}

void CanvasItem::hoverLeaveEvent(QHoverEvent *)
{
    if (m_app) m_app->pointerLeave();
}

void CanvasItem::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!m_app) return;
    m_app->pointerDoubleClick(toImage(e->position()));
    e->accept();
}

void CanvasItem::keyPressEvent(QKeyEvent *e)
{
    if (m_app && m_app->keyPress(e->key(), int(e->modifiers()), e->text())) { e->accept(); return; }
    e->ignore();
}

void CanvasItem::inputMethodEvent(QInputMethodEvent *e)
{
    if (m_app && !e->commitString().isEmpty()) m_app->inputText(e->commitString());
    e->accept();
}

QVariant CanvasItem::inputMethodQuery(Qt::InputMethodQuery q) const
{
    if (q == Qt::ImEnabled) return m_app && m_app->textEditing();
    return QQuickPaintedItem::inputMethodQuery(q);
}
