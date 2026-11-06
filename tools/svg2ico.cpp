// Convierte un SVG en un .ico de Windows con imágenes PNG de 16 a 256 px.
// Uso: svg2ico entrada.svg salida.ico
#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QtEndian>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (argc != 3) return 2;
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    if (!svg.isValid()) return 1;
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    QList<QByteArray> pngs;
    for (int s : sizes) {
        QImage img(s, s, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        svg.render(&p);
        p.end();
        QByteArray png;
        QBuffer b(&png);
        b.open(QIODevice::WriteOnly);
        img.save(&b, "PNG");
        pngs << png;
    }
    QFile out(QString::fromLocal8Bit(argv[2]));
    if (!out.open(QIODevice::WriteOnly)) return 1;
    auto u16 = [&](quint16 v) { v = qToLittleEndian(v); out.write(reinterpret_cast<const char *>(&v), 2); };
    auto u32 = [&](quint32 v) { v = qToLittleEndian(v); out.write(reinterpret_cast<const char *>(&v), 4); };
    u16(0); u16(1); u16(quint16(pngs.size()));
    quint32 offset = 6 + 16 * quint32(pngs.size());
    for (int i = 0; i < pngs.size(); ++i) {
        const int s = sizes[i];
        out.putChar(char(s >= 256 ? 0 : s));
        out.putChar(char(s >= 256 ? 0 : s));
        out.putChar(0); out.putChar(0);
        u16(1); u16(32);
        u32(quint32(pngs[i].size())); u32(offset);
        offset += quint32(pngs[i].size());
    }
    for (const QByteArray &p : pngs) out.write(p);
    return 0;
}
