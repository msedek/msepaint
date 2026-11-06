#include "core/Tool.h"
#include "core/Document.h"
#include "core/History.h"

namespace mp {

void LayerEdit::begin(Document &doc)
{
    m_layer = doc.activeLayer();
    m_before = doc.active().image;
    m_before.detach();
}

void LayerEdit::end(Document &doc, const QRect &hint)
{
    if (m_layer < 0)
        return;
    doc.history()->pushLayerEdit(m_layer, m_before, hint);
    m_before = QImage();
    m_layer = -1;
}

} // namespace mp
