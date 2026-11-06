#pragma once

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QString>

namespace mp {

class Document;

// Datos que comparten todas las herramientas: colores, tamaño y el documento.
struct ToolContext {
    Document *doc = nullptr;
    QColor color1 = Qt::black; // botón izquierdo
    QColor color2 = Qt::white; // botón derecho
    int size = 1;              // grosor en píxeles de la herramienta activa
    qreal opacity = 1.0;
    int tolerance = 0;         // relleno, 0..100
};

struct ToolEvent {
    QPointF pos;               // coordenadas de imagen (sub-píxel)
    Qt::MouseButton button = Qt::LeftButton;
    Qt::KeyboardModifiers modifiers;
    qreal pressure = 1.0;
};

class Tool {
public:
    virtual ~Tool() = default;
    virtual QString id() const = 0;

    virtual void press(ToolContext &, const ToolEvent &) {}
    virtual void move(ToolContext &, const ToolEvent &) {}
    virtual void release(ToolContext &, const ToolEvent &) {}
    virtual void hover(ToolContext &, const ToolEvent &) {}
    // Confirma lo pendiente (forma editable, texto) o lo descarta.
    virtual void commit(ToolContext &) {}
    virtual void cancel(ToolContext &) {}
    virtual bool hasPending() const { return false; }

    // Dibujo temporal encima del lienzo, en coordenadas de imagen; `scale` = zoom actual.
    virtual void paintOverlay(QPainter &, qreal /*scale*/) const {}

    // Color con que pinta según el botón.
    static QColor colorFor(const ToolContext &ctx, const ToolEvent &ev)
    {
        return ev.button == Qt::RightButton ? ctx.color2 : ctx.color1;
    }
};

// Edición de píxeles con deshacer: copia la capa al empezar y registra el cambio al terminar.
class LayerEdit {
public:
    void begin(Document &doc);
    void end(Document &doc, const QRect &hint = QRect());
    bool active() const { return m_layer >= 0; }
    const QImage &before() const { return m_before; }
private:
    int m_layer = -1;
    QImage m_before;
};

} // namespace mp
