// Desarrollo: MSEPAINT_DEMO=1 dibuja una muestra de todas las herramientas por el mismo camino que el ratón,
// para revisar el resultado en una captura sin pantalla.
#include "ui/AppController.h"

#include <QtMath>

void runDemo(AppController &app)
{
    auto stroke = [&](const QString &tool, const QString &brush, int size, QColor c, qreal y0, qreal x0 = 20) {
        app.setTool(tool);
        if (!brush.isEmpty()) app.setBrush(brush);
        app.setColor1(c);
        app.setToolSize(size);
        QPointF p(x0, y0);
        app.pointerPress(p, Qt::LeftButton, {});
        for (int i = 1; i <= 60; ++i) {
            p = QPointF(x0 + i * 4, y0 + 14 * qSin(i / 7.0));
            app.pointerMove(p, Qt::LeftButton, {});
        }
        app.pointerRelease(p, Qt::LeftButton, {});
    };
    const char *brushes[] = {"brush", "calligraphyBrush", "calligraphyPen", "airbrush", "oilBrush",
                             "crayon", "marker", "naturalPencil", "watercolor"};
    const QColor colors[] = {QColor("#3F48CC"), QColor("#ED1C24"), QColor("#22B14C"), QColor("#A349A4"),
                             QColor("#FF7F27"), QColor("#00A2E8"), QColor("#FFC90E"), QColor("#7F7F7F"), QColor("#3F48CC")};
    stroke(QStringLiteral("pencil"), {}, 1, Qt::black, 20);
    for (int i = 0; i < 9; ++i)
        stroke(QStringLiteral("brush"), QString::fromLatin1(brushes[i]), i < 3 ? 10 : 24, colors[i], 55 + i * 42);

    // Formas
    app.setTool(QStringLiteral("shape"));
    app.setColor1(Qt::black);
    app.setColor2(QColor("#FFC90E"));
    app.setProperty("shapeFill", QStringLiteral("solid"));
    app.setToolSize(3);
    const QStringList ids = app.shapeIds();
    int k = 0;
    for (const QString &id : ids) {
        if (id == QLatin1String("curve") || id == QLatin1String("polygon")) continue;
        app.setProperty("shape", id);
        const qreal x = 300 + (k % 5) * 70, y = 20 + (k / 5) * 70;
        app.pointerPress(QPointF(x, y), Qt::LeftButton, {});
        app.pointerMove(QPointF(x + 56, y + 56), Qt::LeftButton, {});
        app.pointerRelease(QPointF(x + 56, y + 56), Qt::LeftButton, {});
        app.keyPress(Qt::Key_Return, 0, {});
        ++k;
    }

    // Texto
    app.setTool(QStringLiteral("text"));
    app.setColor1(Qt::black);
    app.setTextSize(20);
    app.pointerPress(QPointF(300, 330), Qt::LeftButton, {});
    app.pointerMove(QPointF(640, 380), Qt::LeftButton, {});
    app.pointerRelease(QPointF(640, 380), Qt::LeftButton, {});
    for (const QChar ch : QStringLiteral("msepaint"))
        app.keyPress(0, 0, QString(ch));
    app.setTool(QStringLiteral("pencil"));
}
