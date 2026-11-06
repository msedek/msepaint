#include "core/History.h"

#include <cstring>

namespace mp {

namespace {

class LayerPatch final : public Command {
public:
    LayerPatch(int layer, QPoint at, QImage before, QImage after)
        : m_layer(layer), m_at(at), m_before(std::move(before)), m_after(std::move(after)) {}

    void undo(Document &doc) override { put(doc, m_before); }
    void redo(Document &doc) override { put(doc, m_after); }
    qint64 cost() const override { return m_before.sizeInBytes() + m_after.sizeInBytes(); }

private:
    void put(Document &doc, const QImage &patch)
    {
        if (m_layer >= doc.layerCount())
            return;
        QImage &dst = doc.layer(m_layer).image;
        const int bpl = patch.width() * 4;
        for (int y = 0; y < patch.height(); ++y)
            std::memcpy(dst.scanLine(m_at.y() + y) + m_at.x() * 4, patch.constScanLine(y), size_t(bpl));
        doc.notifyChanged(QRect(m_at, patch.size()));
    }
    int m_layer;
    QPoint m_at;
    QImage m_before, m_after;
};

class Snapshot final : public Command {
public:
    Snapshot(DocumentState before, DocumentState after)
        : m_before(std::move(before)), m_after(std::move(after)) {}
    void undo(Document &doc) override { m_before.apply(doc); }
    void redo(Document &doc) override { m_after.apply(doc); }
    qint64 cost() const override
    {
        qint64 c = 0;
        for (const auto &l : m_before.layers) c += l.image.sizeInBytes();
        for (const auto &l : m_after.layers) c += l.image.sizeInBytes();
        return c;
    }
private:
    DocumentState m_before, m_after;
};

class Group final : public Command {
public:
    std::vector<std::unique_ptr<Command>> items;
    void undo(Document &doc) override { for (auto it = items.rbegin(); it != items.rend(); ++it) (*it)->undo(doc); }
    void redo(Document &doc) override { for (auto &c : items) c->redo(doc); }
    qint64 cost() const override { qint64 c = 0; for (auto &i : items) c += i->cost(); return c; }
};

} // namespace

DocumentState DocumentState::capture(const Document &doc)
{
    return DocumentState{doc.layers(), doc.size(), doc.activeLayer(), doc.background(), doc.backgroundVisible()};
}

void DocumentState::apply(Document &doc) const
{
    doc.setBackground(background, backgroundVisible);
    doc.replaceLayers(layers, size, active);
}

QRect diffRect(const QImage &a, const QImage &b, const QRect &within)
{
    if (a.size() != b.size())
        return QRect(QPoint(0, 0), a.size());
    const QRect r = within.isValid() ? within.intersected(a.rect()) : a.rect();
    int top = -1, bottom = -1, left = r.right() + 1, right = r.left() - 1;
    for (int y = r.top(); y <= r.bottom(); ++y) {
        const auto *pa = reinterpret_cast<const quint32 *>(a.constScanLine(y));
        const auto *pb = reinterpret_cast<const quint32 *>(b.constScanLine(y));
        if (std::memcmp(pa + r.left(), pb + r.left(), size_t(r.width()) * 4) == 0)
            continue;
        if (top < 0) top = y;
        bottom = y;
        for (int x = r.left(); x < left; ++x)
            if (pa[x] != pb[x]) { left = x; break; }
        for (int x = r.right(); x > right; --x)
            if (pa[x] != pb[x]) { right = x; break; }
    }
    if (top < 0)
        return {};
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

History::History(Document *doc)
    : QObject(doc), m_doc(doc) {}

void History::undo()
{
    if (!canUndo()) return;
    m_stack[size_t(--m_index)]->undo(*m_doc);
    emit changed();
}

void History::redo()
{
    if (!canRedo()) return;
    m_stack[size_t(m_index++)]->redo(*m_doc);
    emit changed();
}

void History::clear()
{
    m_stack.clear();
    m_index = 0;
    m_cleanIndex = 0;
    emit changed();
}

void History::push(std::unique_ptr<Command> cmd)
{
    m_stack.resize(size_t(m_index));
    if (m_cleanIndex > m_index) m_cleanIndex = -1;
    m_stack.push_back(std::move(cmd));
    m_index = int(m_stack.size());
    trim();
    emit changed();
}

void History::pushLayerEdit(int layerIndex, const QImage &before, const QRect &hint)
{
    const QImage &after = m_doc->layer(layerIndex).image;
    const QRect r = diffRect(before, after, hint);
    if (r.isEmpty())
        return;
    push(std::make_unique<LayerPatch>(layerIndex, r.topLeft(), before.copy(r), after.copy(r)));
}

void History::pushSnapshot(DocumentState before)
{
    push(std::make_unique<Snapshot>(std::move(before), DocumentState::capture(*m_doc)));
}

void History::squashSince(int mark)
{
    if (mark < 0 || m_index - mark < 2 || m_index != int(m_stack.size())) return;
    auto group = std::make_unique<Group>();
    for (int i = mark; i < m_index; ++i) group->items.push_back(std::move(m_stack[size_t(i)]));
    m_stack.resize(size_t(mark));
    m_stack.push_back(std::move(group));
    m_index = int(m_stack.size());
    emit changed();
}

void History::trim()
{
    while (int(m_stack.size()) > m_limit && m_index > 0) {
        m_stack.erase(m_stack.begin());
        --m_index;
        --m_cleanIndex;
    }
}

} // namespace mp
