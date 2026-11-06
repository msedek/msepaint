#pragma once

#include "core/Tool.h"

namespace mp {

// Relleno por inundación, 4-conexo, por tramos. `tolerance` 0..100 (%), comparado contra el píxel semilla.
QRect floodFill(QImage &img, QPoint seed, QRgb premultipliedColor, int tolerance);

class FillTool : public Tool {
public:
    QString id() const override { return QStringLiteral("fill"); }
    void press(ToolContext &ctx, const ToolEvent &ev) override;
};

class ColorPickerTool : public Tool {
public:
    QString id() const override { return QStringLiteral("picker"); }
    void press(ToolContext &ctx, const ToolEvent &ev) override;
    void move(ToolContext &ctx, const ToolEvent &ev) override { press(ctx, ev); }
    // Herramienta a la que volver después de elegir el color (la fija el controlador).
    QString returnTo;
};

} // namespace mp
