#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

namespace mp { class Document; }
class AppController;

// Lienzo: dibuja el documento compuesto con el zoom actual y pasa el ratón a la herramienta activa.
// Su tamaño implícito es el del documento por el zoom; el contenedor QML se encarga del desplazamiento.
class CanvasItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(AppController *controller READ controller WRITE setController NOTIFY controllerChanged)
public:
    explicit CanvasItem(QQuickItem *parent = nullptr);

    AppController *controller() const { return m_app; }
    void setController(AppController *app);

    void paint(QPainter *painter) override;

signals:
    void controllerChanged();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void hoverMoveEvent(QHoverEvent *e) override;
    void hoverLeaveEvent(QHoverEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void inputMethodEvent(QInputMethodEvent *e) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery q) const override;

private:
    void onDocChanged(const QRect &area);
    void updateGeometryFromDoc();
    void updateCursor();
    QPointF toImage(QPointF itemPos) const;

    AppController *m_app = nullptr;
    QImage m_composite;
    Qt::MouseButton m_dragButton = Qt::NoButton;
};
