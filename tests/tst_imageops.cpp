#include "core/Document.h"
#include "core/History.h"
#include "core/ImageOps.h"

#include <QTest>

using namespace mp;

class TstImageOps : public QObject {
    Q_OBJECT
private:
    static Document *make(QSize s)
    {
        auto *d = new Document;
        d->reset(s, Qt::white);
        d->active().image.setPixel(0, 0, qRgb(255, 0, 0));
        return d;
    }
private slots:
    void rotateClockwiseMovesCorner()
    {
        std::unique_ptr<Document> d(make(QSize(4, 2)));
        ops::rotate90(*d, true);
        QCOMPARE(d->size(), QSize(2, 4));
        QCOMPARE(d->flattened().pixel(1, 0), qRgb(255, 0, 0));
        d->history()->undo();
        QCOMPARE(d->size(), QSize(4, 2));
        QCOMPARE(d->flattened().pixel(0, 0), qRgb(255, 0, 0));
    }

    void flipHorizontal()
    {
        std::unique_ptr<Document> d(make(QSize(5, 3)));
        ops::flipHorizontal(*d);
        QCOMPARE(d->flattened().pixel(4, 0), qRgb(255, 0, 0));
    }

    void resizeCanvasLeavesNewAreaTransparent()
    {
        std::unique_ptr<Document> d(make(QSize(2, 2)));
        ops::resizeCanvas(*d, QSize(4, 4));
        QCOMPARE(qAlpha(d->layer(0).image.pixel(3, 3)), 0);
        QCOMPARE(d->layer(0).image.pixel(0, 0), qRgb(255, 0, 0));
        QCOMPARE(d->flattened().pixel(3, 3), qRgb(255, 255, 255)); // se ve el fondo blanco
    }

    void resizeCanvasWithOffset()
    {
        std::unique_ptr<Document> d(make(QSize(2, 2)));
        ops::resizeCanvas(*d, QSize(4, 4), QPoint(2, 2));
        QCOMPARE(d->layer(0).image.pixel(2, 2), qRgb(255, 0, 0));
    }

    void backgroundUndo()
    {
        std::unique_ptr<Document> d(make(QSize(2, 2)));
        d->active().image.fill(Qt::transparent);
        ops::setBackground(*d, Qt::green, true);
        QCOMPARE(d->flattened().pixel(1, 1), qRgb(0, 255, 0));
        ops::setBackground(*d, Qt::green, false);
        QCOMPARE(qAlpha(d->flattened().pixel(1, 1)), 0);
        d->history()->undo();
        d->history()->undo();
        QCOMPARE(d->flattened().pixel(1, 1), qRgb(255, 255, 255));
    }

    void layerAddMoveMergeUndo()
    {
        std::unique_ptr<Document> d(make(QSize(3, 3)));
        ops::addLayer(*d);
        QCOMPARE(d->layerCount(), 2);
        QCOMPARE(d->activeLayer(), 1);
        d->active().image.setPixel(1, 1, qRgb(0, 255, 0));
        ops::mergeDown(*d);
        QCOMPARE(d->layerCount(), 1);
        QCOMPARE(d->layer(0).image.pixel(1, 1), qRgb(0, 255, 0));
        d->history()->undo();
        QCOMPARE(d->layerCount(), 2);
        ops::moveLayer(*d, 1, 0);
        QCOMPARE(d->activeLayer(), 0);
    }

    // --- Diálogos de imagen ---
    static Document *opaque(QSize s)
    {
        auto *d = new Document;
        d->reset(s, Qt::white);
        d->active().image.fill(qRgb(0, 0, 255));
        return d;
    }

    void skewValidRange()
    {
        QVERIFY(!ops::isValidSkew(0));
        QVERIFY(ops::isValidSkew(1));
        QVERIFY(ops::isValidSkew(-89));
        QVERIFY(!ops::isValidSkew(90));
        std::unique_ptr<Document> d(opaque(QSize(10, 10)));
        ops::skewHorizontal(*d, 0);
        ops::skewHorizontal(*d, 90);
        QCOMPARE(d->size(), QSize(10, 10));
        QVERIFY(!d->history()->canUndo());
    }

    void skewHorizontalPositive()
    {
        std::unique_ptr<Document> d(opaque(QSize(10, 10)));
        ops::skewHorizontal(*d, 45); // off = round(tan 45° · 10) = 10
        QCOMPARE(d->size(), QSize(20, 10));
        const QImage &img = d->layer(0).image;
        QCOMPARE(qAlpha(img.pixel(1, 0)), 0);    // arriba a la izquierda queda transparente
        QVERIFY(qAlpha(img.pixel(18, 0)) > 200); // el borde superior se corrió a la derecha
        QVERIFY(qAlpha(img.pixel(1, 9)) > 200);
        QCOMPARE(qAlpha(img.pixel(18, 9)), 0);
        d->history()->undo();
        QCOMPARE(d->size(), QSize(10, 10));
    }

    void skewHorizontalNegativeAndRounding()
    {
        std::unique_ptr<Document> d(opaque(QSize(7, 3)));
        ops::skewHorizontal(*d, -30); // round(tan(-30°) · 3) = round(-1.732) = -2
        QCOMPARE(d->size(), QSize(9, 3));
        const QImage &img = d->layer(0).image;
        QVERIFY(qAlpha(img.pixel(0, 0)) > 200);
        QCOMPARE(qAlpha(img.pixel(0, 2)), 0);
    }

    void skewVertical()
    {
        std::unique_ptr<Document> d(opaque(QSize(4, 6)));
        ops::skewVertical(*d, 45);
        QCOMPARE(d->size(), QSize(4, 10));
        ops::skewVertical(*d, -10); // round(tan(-10°) · 4) = round(-0.705) = -1
        QCOMPARE(d->size(), QSize(4, 11));
    }

    void rotateCustomGrowsCanvas()
    {
        std::unique_ptr<Document> d(opaque(QSize(10, 10)));
        ops::rotateBy(*d, 45);
        // Esquinas en ±7,07 del centro, redondeadas: −2,07→−2 y 12,07→12 → caja de 14.
        QCOMPARE(d->size(), QSize(14, 14));
        const QImage &img = d->layer(0).image;
        QCOMPARE(qAlpha(img.pixel(0, 0)), 0);   // esquinas descubiertas: transparentes
        QCOMPARE(qAlpha(img.pixel(13, 13)), 0);
        QCOMPARE(img.pixel(7, 7), qRgb(0, 0, 255));
        d->history()->undo();
        QCOMPARE(d->size(), QSize(10, 10));
    }

    void rotateCustomMultiplesOf90AreExact()
    {
        std::unique_ptr<Document> d(make(QSize(4, 2)));
        ops::rotateBy(*d, 90);
        QCOMPARE(d->size(), QSize(2, 4));
        QCOMPARE(d->flattened().pixel(1, 0), qRgb(255, 0, 0));
        ops::rotateBy(*d, -90);
        QCOMPARE(d->size(), QSize(4, 2));
        QCOMPARE(d->flattened().pixel(0, 0), qRgb(255, 0, 0));
        ops::rotateBy(*d, 360);
        QCOMPARE(d->history()->count(), 2);
        QCOMPARE(ops::rotatedSize(QSize(4, 2), 180), QSize(4, 2));
    }

    void bicubicKeepsFlatColorAndSize()
    {
        QImage src(9, 5, Document::kFormat);
        src.fill(qRgb(10, 200, 30));
        const QImage up = ops::bicubicScaled(src, QSize(31, 17));
        QCOMPARE(up.size(), QSize(31, 17));
        QCOMPARE(up.pixel(0, 0), qRgb(10, 200, 30)); // borde en espejo: sin halo
        QCOMPARE(up.pixel(30, 16), qRgb(10, 200, 30));
        const QImage down = ops::bicubicScaled(src, QSize(2, 1));
        QCOMPARE(down.pixel(1, 0), qRgb(10, 200, 30));
    }

    void bicubicDownscaleAverages()
    {
        QImage src(64, 1, Document::kFormat);
        for (int x = 0; x < 64; ++x) src.setPixel(x, 0, x % 2 ? qRgb(255, 255, 255) : qRgb(0, 0, 0));
        const QImage down = ops::bicubicScaled(src, QSize(4, 1));
        // El damero fino se promedia (prefiltrado); lejos de los bordes en espejo queda gris medio.
        QVERIFY(std::abs(qRed(down.pixel(1, 0)) - 128) <= 8);
        QVERIFY(std::abs(qRed(down.pixel(2, 0)) - 128) <= 8);
    }

    void resampleUndoAndNoop()
    {
        std::unique_ptr<Document> d(make(QSize(4, 4)));
        ops::resample(*d, QSize(4, 4));
        QVERIFY(!d->history()->canUndo());
        ops::resample(*d, QSize(8, 2));
        QCOMPARE(d->size(), QSize(8, 2));
        d->history()->undo();
        QCOMPARE(d->size(), QSize(4, 4));
    }

    void dpiDefault()
    {
        Document d;
        QCOMPARE(d.dpi(), 96.0);
        d.setDpi(300);
        QCOMPARE(d.dpi(), 300.0);
        d.setDpi(0);
        QCOMPARE(d.dpi(), 96.0);
    }

    void deleteKeepsLastLayer()
    {
        std::unique_ptr<Document> d(make(QSize(2, 2)));
        ops::deleteLayer(*d);
        QCOMPARE(d->layerCount(), 1);
    }

    void squashJoinsSteps()
    {
        std::unique_ptr<Document> d(make(QSize(4, 4)));
        const int m = d->history()->mark();
        ops::flipHorizontal(*d);
        ops::rotate90(*d, true);
        d->history()->squashSince(m);
        QCOMPARE(d->history()->count(), 1);
        d->history()->undo();
        QCOMPARE(d->flattened().pixel(0, 0), qRgb(255, 0, 0));
        d->history()->redo();
        QCOMPARE(d->history()->canRedo(), false);
    }
};

QTEST_GUILESS_MAIN(TstImageOps)
#include "tst_imageops.moc"
