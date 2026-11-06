#include "core/Document.h"
#include "core/FileFormats.h"

#include <QImageReader>
#include <QTemporaryDir>
#include <QTest>

using namespace mp;
using namespace mp::files;

namespace {

// Imagen de prueba: degradado opaco con pocos colores en la mitad izquierda y degradado continuo en la derecha.
QImage pattern(QSize s, bool alpha)
{
    QImage img(s, Document::kFormat);
    for (int y = 0; y < s.height(); ++y)
        for (int x = 0; x < s.width(); ++x) {
            const int a = alpha ? (x * 255 / std::max(1, s.width() - 1)) : 255;
            img.setPixelColor(x, y, QColor((x * 7) % 256, (y * 11) % 256, (x + y) % 256, a));
        }
    return img;
}

ImageData single(const QImage &img)
{
    ImageData d;
    d.layers.push_back(Layer{QStringLiteral("Layer 1"), img, true, 1.0});
    return d;
}

int maxDiff(const QImage &a, const QImage &b)
{
    // En premultiplicado: con alfa bajo el color no premultiplicado no es significativo.
    const QImage x = a.convertToFormat(QImage::Format_ARGB32_Premultiplied), y = b.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int m = 0;
    for (int j = 0; j < x.height(); ++j)
        for (int i = 0; i < x.width(); ++i) {
            const QRgb p = x.pixel(i, j), q = y.pixel(i, j);
            m = std::max({m, std::abs(qRed(p) - qRed(q)), std::abs(qGreen(p) - qGreen(q)), std::abs(qBlue(p) - qBlue(q)),
                          std::abs(qAlpha(p) - qAlpha(q))});
        }
    return m;
}

} // namespace

class TstFileFormats : public QObject {
    Q_OBJECT
    QTemporaryDir m_dir;
    QString path(const QString &name) const { return m_dir.filePath(name); }

private slots:
    void suffixesMapToFormats()
    {
        QCOMPARE(formatFromSuffix(QStringLiteral("BMP")), Format::Bmp24);
        QCOMPARE(formatFromSuffix(QStringLiteral("jfif")), Format::Jpeg);
        QCOMPARE(formatFromSuffix(QStringLiteral("paint")), Format::Paint);
        QCOMPARE(formatFromId(QStringLiteral("bmp4")), Format::Bmp4);
        QCOMPARE(defaultSuffix(Format::Bmp8), QStringLiteral("bmp"));
        QCOMPARE(writableFormats().mid(0, 4), (QList<Format>{Format::Bmp1, Format::Bmp4, Format::Bmp8, Format::Bmp24}));
    }

    void pngKeepsAlphaAndDpi()
    {
        ImageData d = single(pattern(QSize(40, 30), true));
        d.backgroundVisible = false;
        d.dpiX = 150; d.dpiY = 300;
        QVERIFY(save(path("a.png"), Format::Png, d));
        ImageData r;
        QVERIFY(load(path("a.png"), r));
        QCOMPARE(r.format, Format::Png);
        QCOMPARE(r.size(), QSize(40, 30));
        QVERIFY(maxDiff(r.layers[0].image, d.layers[0].image) <= 1);
        QCOMPARE(qRound(r.dpiX), 150);
        QCOMPARE(qRound(r.dpiY), 300);
    }

    void jpegIsOpaqueAndClose()
    {
        ImageData d = single(pattern(QSize(64, 48), false));
        QVERIFY(save(path("a.jpg"), Format::Jpeg, d));
        ImageData r;
        QVERIFY(load(path("a.jpg"), r));
        QCOMPARE(r.bitsPerPixel, 24);
        QVERIFY(!r.layers[0].image.convertToFormat(QImage::Format_ARGB32).hasAlphaChannel()
                || r.layers[0].image.pixelColor(0, 0).alpha() == 255);
        QVERIFY(maxDiff(r.layers[0].image, d.layers[0].image) < 40); // calidad 95
    }

    void transparentAreaFlattensOverWhiteWhenBackgroundHidden()
    {
        QImage img = makeLayerImage(QSize(4, 4));
        img.setPixelColor(0, 0, QColor(255, 0, 0));
        ImageData d = single(img);
        d.backgroundVisible = false;
        QVERIFY(save(path("t.bmp"), Format::Bmp24, d));
        const QImage r(path("t.bmp"));
        QCOMPARE(r.pixel(0, 0), qRgb(255, 0, 0));
        QCOMPARE(r.pixel(3, 3), qRgb(255, 255, 255));
    }

    void transparentAreaTakesBackgroundColorWhenVisible()
    {
        ImageData d = single(makeLayerImage(QSize(4, 4)));
        d.background = QColor(10, 200, 30);
        QVERIFY(save(path("bg.bmp"), Format::Bmp24, d));
        QCOMPARE(QImage(path("bg.bmp")).pixel(2, 2), qRgb(10, 200, 30));
    }

    void bmpDepths_data()
    {
        QTest::addColumn<int>("fmt");
        QTest::addColumn<int>("bpp");
        QTest::newRow("1") << int(Format::Bmp1) << 1;
        QTest::newRow("4") << int(Format::Bmp4) << 4;
        QTest::newRow("8") << int(Format::Bmp8) << 8;
        QTest::newRow("24") << int(Format::Bmp24) << 24;
    }
    void bmpDepths()
    {
        QFETCH(int, fmt);
        QFETCH(int, bpp);
        // Blanco y negro puros: sobreviven a cualquier profundidad.
        QImage img = makeLayerImage(QSize(13, 7), Qt::white);
        for (int x = 0; x < 13; x += 2) img.setPixelColor(x, 3, Qt::black);
        ImageData d = single(img);
        d.dpiX = d.dpiY = 120;
        const QString p = path(QStringLiteral("d%1.bmp").arg(bpp));
        QVERIFY(save(p, Format(fmt), d));
        QFile f(p);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray b = f.readAll();
        QCOMPARE(int(uchar(b[28])), bpp); // biBitCount
        ImageData r;
        QVERIFY(load(p, r));
        QCOMPARE(r.bitsPerPixel, bpp);
        QCOMPARE(r.size(), QSize(13, 7));
        QCOMPARE(qRound(r.dpiX), 120);
        QCOMPARE(maxDiff(r.layers[0].image, img), 0);
    }

    void gifRoundTripWithFewColorsIsExact()
    {
        QImage img = makeLayerImage(QSize(97, 61), Qt::white);
        for (int y = 0; y < 61; ++y)
            for (int x = 0; x < 97; ++x) img.setPixelColor(x, y, QColor::fromHsv((x * 3 + y) % 200, 255, 255));
        QVERIFY(save(path("a.gif"), Format::Gif, single(img)));
        const QImage r(path("a.gif"));
        QVERIFY(!r.isNull());
        QCOMPARE(maxDiff(r, img), 0);
    }

    void gifManyColorsStillDecodes()
    {
        const QImage img = pattern(QSize(300, 200), false);
        QVERIFY(save(path("b.gif"), Format::Gif, single(img)));
        const QImage r(path("b.gif"));
        QCOMPARE(r.size(), QSize(300, 200));
    }

    void tiffAndWebpRoundTrip()
    {
        ImageData d = single(pattern(QSize(20, 20), true));
        d.backgroundVisible = false; // si no, el fondo blanco tapa la transparencia
        for (Format f : {Format::Tiff, Format::Webp, Format::Ico}) {
            if (!canWrite(f)) continue;
            const QString p = path(QStringLiteral("rt.") + defaultSuffix(f));
            QVERIFY(save(p, f, d));
            ImageData r;
            QVERIFY2(load(p, r), qPrintable(defaultSuffix(f)));
            QCOMPARE(r.size(), QSize(20, 20));
            const int diff = maxDiff(r.layers[0].image, d.layers[0].image);
            QVERIFY2(diff <= 1, qPrintable(defaultSuffix(f) + QStringLiteral(" diff %1").arg(diff)));
        }
    }

    void projectKeepsThreeLayersBackgroundAndDpi()
    {
        if (!hasProjectSupport()) QSKIP("compilado sin libheif");
        ImageData d;
        d.layers.push_back(Layer{QStringLiteral("a"), pattern(QSize(33, 21), true), true, 1.0});
        QImage mid = makeLayerImage(QSize(33, 21));
        mid.setPixelColor(5, 5, QColor(0, 0, 255, 128));
        d.layers.push_back(Layer{QStringLiteral("b"), mid, false, 1.0}); // oculta: se guarda igual
        d.layers.push_back(Layer{QStringLiteral("c"), makeLayerImage(QSize(33, 21), QColor(0, 255, 0, 40)), true, 1.0});
        d.background = QColor(200, 100, 50);
        d.backgroundVisible = false;
        d.dpiX = 72; d.dpiY = 144;
        QString err;
        QVERIFY2(save(path("p.paint"), Format::Paint, d, &err), qPrintable(err));
        ImageData r;
        QVERIFY2(load(path("p.paint"), r, &err), qPrintable(err));
        QCOMPARE(r.format, Format::Paint);
        QCOMPARE(int(r.layers.size()), 3);
        for (int i = 0; i < 3; ++i) {
            QVERIFY(r.layers[size_t(i)].visible);         // la visibilidad no se guarda
            QVERIFY(r.layers[size_t(i)].name.isEmpty());  // ni el nombre
            QVERIFY2(maxDiff(r.layers[size_t(i)].image, d.layers[size_t(i)].image) <= 1, qPrintable(QString::number(i)));
        }
        QCOMPARE(r.background, QColor(200, 100, 50));
        QVERIFY(!r.backgroundVisible);
        QCOMPARE(qRound(r.dpiX), 72);
        QCOMPARE(qRound(r.dpiY), 144);
    }

    void foreignHeifOpensAsSingleLayer()
    {
        if (!hasProjectSupport()) QSKIP("compilado sin libheif");
        // Un .paint sin la lista de capas (como un HEIC normal renombrado) abre con una sola capa y fondo negro oculto.
        ImageData d = single(pattern(QSize(8, 8), false));
        QVERIFY(save(path("one.paint"), Format::Paint, d));
        QFile f(path("one.paint"));
        QVERIFY(f.open(QIODevice::ReadWrite));
        QByteArray b = f.readAll();
        const qsizetype at = b.indexOf("msepaint:Layers=\"");
        QVERIFY(at > 0);
        b.replace(at, 8, "xxxxxxxx"); // rompe el atributo sin cambiar el tamaño
        f.seek(0);
        f.write(b);
        f.close();
        ImageData r;
        QVERIFY(load(path("one.paint"), r));
        QCOMPARE(int(r.layers.size()), 1);
        QVERIFY(!r.backgroundVisible);
        QVERIFY(maxDiff(r.layers[0].image, d.layers[0].image) <= 1);
    }

    void flattenCombinesVisibleLayersOnly()
    {
        ImageData d;
        d.layers.push_back(Layer{QString(), makeLayerImage(QSize(2, 2), Qt::red), true, 1.0});
        d.layers.push_back(Layer{QString(), makeLayerImage(QSize(2, 2), Qt::blue), false, 1.0});
        QCOMPARE(flattened(d, false).pixel(1, 1), qRgb(255, 0, 0));
        d.layers[1].visible = true;
        QCOMPARE(flattened(d, false).pixel(1, 1), qRgb(0, 0, 255));
    }

    void emptyStackFails()
    {
        QString err;
        QVERIFY(!save(path("e.png"), Format::Png, ImageData(), &err));
        QVERIFY(!err.isEmpty());
        ImageData r;
        QVERIFY(!load(path("missing.png"), r));
    }
};

QTEST_MAIN(TstFileFormats)
#include "tst_fileformats.moc"
