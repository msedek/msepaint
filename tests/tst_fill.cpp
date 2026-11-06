#include "core/Document.h"
#include "core/History.h"
#include "core/tools/Fill.h"

#include <QTest>

using namespace mp;

class TstFill : public QObject {
    Q_OBJECT
private slots:
    void fillsEnclosedRegionOnly()
    {
        QImage img = makeLayerImage(QSize(10, 10), Qt::white);
        for (int i = 0; i < 10; ++i) img.setPixel(5, i, qRgb(0, 0, 0)); // pared vertical
        const QRect r = floodFill(img, QPoint(1, 1), qRgb(255, 0, 0), 0);
        QCOMPARE(r, QRect(0, 0, 5, 10));
        QCOMPARE(img.pixel(4, 9), qRgb(255, 0, 0));
        QCOMPARE(img.pixel(6, 0), qRgb(255, 255, 255));
    }

    void diagonalGapDoesNotLeak()
    {
        QImage img = makeLayerImage(QSize(4, 4), Qt::white);
        img.setPixel(1, 0, qRgb(0, 0, 0));
        img.setPixel(0, 1, qRgb(0, 0, 0));
        floodFill(img, QPoint(0, 0), qRgb(255, 0, 0), 0);
        QCOMPARE(img.pixel(2, 2), qRgb(255, 255, 255));
    }

    void toleranceAcceptsNearColors()
    {
        QImage img = makeLayerImage(QSize(3, 1), Qt::white);
        img.setPixel(1, 0, qRgb(250, 250, 250));
        img.setPixel(2, 0, qRgb(100, 100, 100));
        // 5 %: límite 0,05² × 510² = 650; (5,5,5) da 75 → entra; (155,155,155) no.
        floodFill(img, QPoint(0, 0), qRgb(255, 0, 0), 5);
        QCOMPARE(img.pixel(1, 0), qRgb(255, 0, 0));
        QCOMPARE(img.pixel(2, 0), qRgb(100, 100, 100));
    }

    void fillToolRecordsUndo()
    {
        Document d;
        d.reset(QSize(5, 5), Qt::white);
        ToolContext ctx;
        ctx.doc = &d;
        ctx.color1 = Qt::green;
        FillTool t;
        ToolEvent ev;
        ev.pos = QPointF(2, 2);
        t.press(ctx, ev);
        QCOMPARE(d.flattened().pixel(0, 0), qRgb(0, 255, 0));
        d.history()->undo();
        QCOMPARE(d.flattened().pixel(0, 0), qRgb(255, 255, 255));
    }
};

QTEST_GUILESS_MAIN(TstFill)
#include "tst_fill.moc"
