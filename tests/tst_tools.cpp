#include "core/Brushes.h"
#include "core/Document.h"
#include "core/History.h"
#include "core/ShapeGeometry.h"
#include "core/tools/Select.h"
#include "core/tools/Shape.h"
#include "core/tools/Stroke.h"
#include "core/tools/Text.h"

#include <QGuiApplication>
#include <QPainter>
#include <QTest>

using namespace mp;

namespace {
ToolEvent ev(qreal x, qreal y, Qt::MouseButton b = Qt::LeftButton, Qt::KeyboardModifiers m = {})
{
    ToolEvent e;
    e.pos = QPointF(x, y);
    e.button = b;
    e.modifiers = m;
    return e;
}
struct Fixture {
    Document doc;
    ToolContext ctx;
    explicit Fixture(QSize s = QSize(40, 40))
    {
        doc.reset(s, Qt::white);
        ctx.doc = &doc;
    }
    QRgb at(int x, int y) { return doc.flattened().pixel(x, y); }
};
const QRgb kWhite = qRgb(255, 255, 255), kBlack = qRgb(0, 0, 0), kRed = qRgb(255, 0, 0);
}

class TstTools : public QObject {
    Q_OBJECT
private slots:
    // --- Trazos ---
    void brushCapsuleHasSoftEdgeAndSolidCore()
    {
        Fixture f;
        f.ctx.size = 9; // radio 4,5 y borde suave de 0,9 px
        StrokeTool t(QStringLiteral("brush"), brushes::Media::Brush);
        t.press(f.ctx, ev(10, 20));
        t.move(f.ctx, ev(30, 20));
        t.release(f.ctx, ev(30, 20));
        QCOMPARE(f.at(20, 20), kBlack);              // centro
        QCOMPARE(f.at(20, 25), kWhite);              // fuera del radio
        const QRgb edge = f.at(20, 24);              // dentro del borde suave
        QVERIFY(qRed(edge) > 0 && qRed(edge) < 255);
        QCOMPARE(f.doc.history()->count(), 1);
    }

    void eraserMakesTransparentAndRightClickRecolors()
    {
        Fixture f;
        f.doc.active().image.fill(Qt::black);
        f.ctx.size = 4;
        StrokeTool e(QStringLiteral("eraser"), brushes::Media::SquareEraser, StrokeTool::Kind::Eraser);
        e.press(f.ctx, ev(5, 5));
        e.release(f.ctx, ev(5, 5));
        QCOMPARE(qAlpha(f.doc.active().image.pixel(5, 5)), 0);
        QCOMPARE(f.at(5, 5), kWhite); // se ve el fondo

        f.ctx.color1 = Qt::black;
        f.ctx.color2 = Qt::red;
        e.press(f.ctx, ev(20, 20, Qt::RightButton));
        e.release(f.ctx, ev(20, 20, Qt::RightButton));
        QCOMPARE(f.at(20, 20), kRed);
    }

    void pencilShiftLocksHorizontal()
    {
        Fixture f;
        StrokeTool t(QStringLiteral("pencil"), brushes::Media::Pencil);
        t.press(f.ctx, ev(5, 5));
        t.move(f.ctx, ev(15, 6, Qt::LeftButton, Qt::ShiftModifier));
        t.move(f.ctx, ev(20, 12, Qt::LeftButton, Qt::ShiftModifier));
        t.release(f.ctx, ev(20, 12));
        QCOMPARE(f.at(20, 5), kBlack);
        QCOMPARE(f.at(20, 12), kWhite);
    }

    // --- Selección ---
    void moveSelectionLeavesTransparentHole()
    {
        Fixture f;
        f.doc.active().image.fill(Qt::transparent);
        QPainter(&f.doc.active().image).fillRect(QRect(0, 0, 16, 16), Qt::red);
        SelectTool s;
        s.press(f.ctx, ev(0, 0));
        s.move(f.ctx, ev(15.5, 15.5));
        s.release(f.ctx, ev(15.5, 15.5));
        QVERIFY(s.hasSelection());
        QCOMPARE(s.selectionSize(), QSize(16, 16));
        s.press(f.ctx, ev(8, 8));        // centro: mover
        s.move(f.ctx, ev(28, 28));
        s.release(f.ctx, ev(28, 28));
        s.commit(f.ctx);
        QCOMPARE(f.at(2, 2), kWhite);    // huella transparente: se ve el fondo
        QCOMPARE(f.at(22, 22), kRed);
        f.doc.history()->undo();
        QCOMPARE(f.at(2, 2), kRed);
    }

    void transparentSelectionDropsColor2()
    {
        Fixture f;
        f.doc.active().image.fill(Qt::white);
        QPainter(&f.doc.active().image).fillRect(QRect(6, 6, 4, 4), Qt::red);
        SelectTool s;
        s.transparent = true;
        f.ctx.color2 = Qt::white;
        s.press(f.ctx, ev(0, 0)); s.move(f.ctx, ev(15.5, 15.5)); s.release(f.ctx, ev(15.5, 15.5));
        s.press(f.ctx, ev(8, 8)); s.move(f.ctx, ev(28, 8)); s.release(f.ctx, ev(28, 8));
        // Antes de confirmar pintamos debajo para ver que el blanco del flotante no lo tapa.
        QPainter(&f.doc.active().image).fillRect(QRect(20, 0, 16, 16), Qt::blue);
        s.commit(f.ctx);
        QCOMPARE(f.at(21, 1), qRgb(0, 0, 255));
        QCOMPARE(f.at(27, 7), kRed);
    }

    void cropToSelection()
    {
        Fixture f;
        SelectTool s;
        s.press(f.ctx, ev(5, 5)); s.move(f.ctx, ev(14.5, 9.5)); s.release(f.ctx, ev(14.5, 9.5));
        s.crop(f.ctx);
        QCOMPARE(f.doc.size(), QSize(10, 5));
        f.doc.history()->undo();
        QCOMPARE(f.doc.size(), QSize(40, 40));
    }

    void pasteBiggerThanCanvasGrowsIt()
    {
        Fixture f(QSize(10, 10));
        SelectTool s;
        QImage img(20, 15, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::red);
        s.paste(f.ctx, img, QPoint(0, 0));
        QCOMPARE(f.doc.size(), QSize(20, 15));
        s.commit(f.ctx);
        QCOMPARE(f.at(19, 14), kRed);
    }

    // --- Formas ---
    void templatesFillTheirBox()
    {
        for (const QString &id : shapes::galleryOrder()) {
            if (!shapes::hasTemplate(id)) continue;
            const QRectF b = shapes::fitted(id, QRectF(0, 0, 100, 100)).boundingRect();
            QVERIFY2(b.width() > 80 && b.height() > 80, qPrintable(id));
        }
    }

    void lineShiftConstraint()
    {
        QCOMPARE(shapes::constrainLine(QPoint(0, 0), QPoint(10, 1)), QPoint(10, 0));
        QCOMPARE(shapes::constrainLine(QPoint(0, 0), QPoint(1, 10)), QPoint(0, 10));
        QCOMPARE(shapes::constrainLine(QPoint(0, 0), QPoint(10, 8)), QPoint(9, 9));
    }

    void rectangleOutlineAndFill()
    {
        Fixture f;
        ShapeTool t;
        t.shape = QStringLiteral("rectangle");
        t.fill = QStringLiteral("solid");
        f.ctx.size = 2;
        f.ctx.color1 = Qt::black;
        f.ctx.color2 = Qt::red;
        t.press(f.ctx, ev(5, 5));
        t.move(f.ctx, ev(30, 30));
        t.release(f.ctx, ev(30, 30));
        QVERIFY(t.hasPending());
        t.commit(f.ctx);
        QCOMPARE(f.at(17, 17), kRed);           // relleno con color 2
        QCOMPARE(f.at(17, 5), kBlack);          // contorno con color 1
        QCOMPARE(f.at(2, 2), kWhite);
        QCOMPARE(f.doc.history()->count(), 1);
    }

    void shapeCancelRestores()
    {
        Fixture f;
        ShapeTool t;
        t.shape = QStringLiteral("oval");
        t.press(f.ctx, ev(5, 5)); t.move(f.ctx, ev(30, 30)); t.release(f.ctx, ev(30, 30));
        t.cancel(f.ctx);
        QCOMPARE(f.doc.flattened(), [] { Fixture g; return g.doc.flattened(); }());
    }

    void polygonClosesOnFirstVertex()
    {
        Fixture f;
        ShapeTool t;
        t.shape = QStringLiteral("polygon");
        t.fill = QStringLiteral("solid");
        f.ctx.color2 = Qt::red;
        t.press(f.ctx, ev(5, 5)); t.move(f.ctx, ev(30, 5)); t.release(f.ctx, ev(30, 5));
        t.press(f.ctx, ev(30, 30)); t.release(f.ctx, ev(30, 30));
        t.press(f.ctx, ev(5, 30)); t.release(f.ctx, ev(5, 30));
        t.press(f.ctx, ev(5, 5)); t.release(f.ctx, ev(5, 5)); // clic sobre el primero: cierra
        QVERIFY(!t.hasPending());
        QCOMPARE(f.at(17, 17), kRed);
    }

    // --- Texto ---
    void textCommitsToLayer()
    {
        Fixture f(QSize(200, 60));
        TextTool t;
        t.press(f.ctx, ev(5, 5));
        t.release(f.ctx, ev(5, 5));
        QVERIFY(t.editing());
        for (const QChar c : QStringLiteral("HHHH")) t.key(f.ctx, 0, {}, QString(c));
        t.commit(f.ctx);
        const QImage img = f.doc.flattened();
        bool dark = false;
        for (int y = 5; y < 30 && !dark; ++y)
            for (int x = 5; x < 60 && !dark; ++x) dark = qRed(img.pixel(x, y)) < 100;
        QVERIFY(dark);
        QCOMPARE(f.doc.history()->count(), 1);
    }
};

QTEST_MAIN(TstTools)
#include "tst_tools.moc"
