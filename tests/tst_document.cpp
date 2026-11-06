#include "core/Document.h"
#include "core/History.h"
#include "core/tools/Stroke.h"

#include <QTest>

using namespace mp;

class TstDocument : public QObject {
    Q_OBJECT
private slots:
    void resetCreatesOneLayerOverWhiteBackground()
    {
        Document d;
        d.reset(QSize(10, 8), Qt::white);
        QCOMPARE(d.layerCount(), 1);
        QCOMPARE(d.size(), QSize(10, 8));
        QCOMPARE(d.flattened().pixel(3, 3), qRgb(255, 255, 255));
    }

    void diffRectFindsMinimalBox()
    {
        QImage a = makeLayerImage(QSize(20, 20), Qt::white);
        QImage b = a.copy();
        b.setPixel(4, 7, qRgb(0, 0, 0));
        b.setPixel(11, 9, qRgb(0, 0, 0));
        QCOMPARE(diffRect(a, b), QRect(QPoint(4, 7), QPoint(11, 9)));
        QVERIFY(diffRect(a, a.copy()).isEmpty());
    }

    void pencilStrokeUndoRedo()
    {
        Document d;
        d.reset(QSize(16, 16), Qt::white);
        ToolContext ctx;
        ctx.doc = &d;
        StrokeTool t(QStringLiteral("pencil"), brushes::Media::Pencil);
        ToolEvent ev;
        ev.pos = QPointF(1.5, 1.5);
        t.press(ctx, ev);
        ev.pos = QPointF(6.5, 1.5);
        t.move(ctx, ev);
        t.release(ctx, ev);

        QCOMPARE(d.history()->count(), 1);
        for (int x = 1; x <= 6; ++x)
            QCOMPARE(d.flattened().pixel(x, 1), qRgb(0, 0, 0));
        QCOMPARE(d.flattened().pixel(7, 1), qRgb(255, 255, 255));

        d.history()->undo();
        QCOMPARE(d.flattened().pixel(3, 1), qRgb(255, 255, 255));
        d.history()->redo();
        QCOMPARE(d.flattened().pixel(3, 1), qRgb(0, 0, 0));
    }

    void rightButtonUsesColor2()
    {
        Document d;
        d.reset(QSize(4, 4), Qt::white);
        ToolContext ctx;
        ctx.doc = &d;
        ctx.color2 = Qt::red;
        StrokeTool t(QStringLiteral("pencil"), brushes::Media::Pencil);
        ToolEvent ev;
        ev.pos = QPointF(2, 2);
        ev.button = Qt::RightButton;
        t.press(ctx, ev);
        t.release(ctx, ev);
        QCOMPARE(d.flattened().pixel(2, 2), qRgb(255, 0, 0));
    }
};

QTEST_GUILESS_MAIN(TstDocument)
#include "tst_document.moc"
