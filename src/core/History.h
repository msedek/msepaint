#pragma once

#include "core/Document.h"

#include <QObject>
#include <memory>
#include <vector>

namespace mp {

class Command {
public:
    virtual ~Command() = default;
    virtual void undo(Document &doc) = 0;
    virtual void redo(Document &doc) = 0;
    virtual qint64 cost() const = 0; // bytes aproximados
};

// Estado completo del documento. Copiar es barato: QImage comparte datos implícitamente.
struct DocumentState {
    std::vector<Layer> layers;
    QSize size;
    int active = 0;
    QColor background = Qt::white;
    bool backgroundVisible = true;
    static DocumentState capture(const Document &doc);
    void apply(Document &doc) const;
};

class History : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
public:
    explicit History(Document *doc);

    bool canUndo() const { return m_index > 0; }
    bool canRedo() const { return m_index < int(m_stack.size()); }
    void undo();
    void redo();
    void clear();

    void push(std::unique_ptr<Command> cmd);

    // Junta en un solo paso los comandos agregados desde `mark()` (p. ej. cambiar tamaño + inclinar).
    int mark() const { return m_index; }
    void squashSince(int mark);

    // Cambio de píxeles en una capa: `before` es la capa completa antes de modificarla.
    // Se guarda solo el rectángulo que realmente cambió.
    void pushLayerEdit(int layerIndex, const QImage &before, const QRect &hint = QRect());

    // Cambio estructural (capas, tamaño): `before` capturado antes de la operación.
    void pushSnapshot(DocumentState before);

    int count() const { return int(m_stack.size()); }
    void setLimit(int maxSteps) { m_limit = maxSteps; trim(); }
    bool isClean() const { return m_index == m_cleanIndex; }
    void setClean() { m_cleanIndex = m_index; emit changed(); }

signals:
    void changed();

private:
    void trim();
    Document *m_doc;
    std::vector<std::unique_ptr<Command>> m_stack;
    int m_index = 0;
    int m_cleanIndex = 0;
    int m_limit = 100;
};

// Rectángulo mínimo donde difieren dos imágenes del mismo tamaño (vacío si son iguales).
QRect diffRect(const QImage &a, const QImage &b, const QRect &within = QRect());

} // namespace mp
