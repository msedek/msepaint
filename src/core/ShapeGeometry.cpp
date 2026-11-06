#include "core/ShapeGeometry.h"

#include <QHash>
#include <QPolygonF>
#include <QTransform>
#include <QtMath>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <initializer_list>

namespace mp::shapes {

namespace {

// Los ángulos de las plantillas se miden sobre la elipse real (ángulo geométrico); Qt usa el paramétrico.
qreal toParametric(qreal deg, qreal a, qreal b)
{
    if (a <= 0 || b <= 0 || a == b) return deg;
    const qreal r = qDegreesToRadians(deg);
    const qreal t = std::atan2(std::sin(r) * a, std::cos(r) * b);
    qreal out = qRadiansToDegrees(t);
    // Mantener la vuelta completa en la que estaba el ángulo original.
    while (out - deg > 180) out -= 360;
    while (deg - out > 180) out += 360;
    return out;
}

// Arco de la caja (izq, arr, der, abj), ángulos en grados en sentido horario; se une al punto actual con una recta.
void arcBox(QPainterPath &p, qreal l, qreal t, qreal r, qreal b, qreal start, qreal sweep)
{
    const QRectF box(l, t, r - l, b - t);
    const qreal a = box.width() / 2, bb = box.height() / 2;
    const qreal ps = toParametric(start, a, bb);
    const qreal pe = toParametric(start + sweep, a, bb);
    if (p.elementCount() == 0) p.arcMoveTo(box, -ps);
    p.arcTo(box, -ps, -(pe - ps));
}

QPainterPath polygon(std::initializer_list<QPointF> pts)
{
    QPainterPath p;
    p.addPolygon(QPolygonF(QList<QPointF>(pts)));
    p.closeSubpath();
    return p;
}

QPainterPath build(const QString &id)
{
    QPainterPath p;
    if (id == QLatin1String("rectangle")) {
        p.addRect(-50, -50, 100, 100);
    } else if (id == QLatin1String("oval")) {
        p.addEllipse(QRectF(-50, -50, 100, 100));
    } else if (id == QLatin1String("roundedRectangle")) {
        arcBox(p, -50, -50, -18, -18, 180, 90);
        arcBox(p, 16, -50, 49, -18, 270, 90);
        arcBox(p, 16, 17, 49, 49, 0, 90);
        arcBox(p, -50, 17, -18, 49, 90, 90);
        p.lineTo(-50, -35);
        p.lineTo(-50, -34);
        p.closeSubpath();
    } else if (id == QLatin1String("triangle")) {
        p = polygon({{0, -50}, {50, 50}, {-50, 50}});
    } else if (id == QLatin1String("rightTriangle")) {
        p = polygon({{-50, -50}, {50, 50}, {-50, 50}});
    } else if (id == QLatin1String("diamond")) {
        p = polygon({{0, -50}, {50, 0}, {0, 50}, {-50, 0}});
    } else if (id == QLatin1String("pentagon")) {
        p = polygon({{0, -50}, {-50, -12}, {-31, 50}, {31, 50}, {50, -12}});
    } else if (id == QLatin1String("hexagon")) {
        p = polygon({{0, -50}, {-50, -25}, {-50, 25}, {0, 50}, {50, 25}, {50, -25}});
    } else if (id == QLatin1String("rightArrow")) {
        p = polygon({{0, -50}, {0, -25}, {-50, -25}, {-50, 25}, {0, 25}, {0, 50}, {50, 0}});
    } else if (id == QLatin1String("leftArrow")) {
        p = polygon({{-50, 0}, {0, 50}, {0, 25}, {50, 25}, {50, -25}, {0, -25}, {0, -50}});
    } else if (id == QLatin1String("upArrow")) {
        p = polygon({{0, -50}, {-50, 0}, {-25, 0}, {-25, 50}, {25, 50}, {25, 0}, {50, 0}});
    } else if (id == QLatin1String("downArrow")) {
        p = polygon({{-50, 0}, {0, 50}, {50, 0}, {25, 0}, {25, -50}, {-25, -50}, {-25, 0}});
    } else if (id == QLatin1String("fourPointStar")) {
        p = polygon({{-50, 0}, {-12, -12}, {0, -50}, {12, -12}, {50, 0}, {12, 12}, {0, 50}, {-12, 12}});
    } else if (id == QLatin1String("fivePointStar")) {
        p = polygon({{0, -50}, {-12, -12}, {-50, -11}, {-20, 12}, {-31, 50}, {0, 28}, {31, 50}, {20, 12},
                     {50, -11}, {12, -12}});
    } else if (id == QLatin1String("sixPointStar")) {
        p = polygon({{0, -50}, {18, -25}, {50, -25}, {34, 0}, {50, 25}, {18, 25}, {0, 50}, {-18, 25},
                     {-50, 25}, {-34, 0}, {-50, -25}, {-18, -25}});
    } else if (id == QLatin1String("lightning")) {
        p = polygon({{-11, -50}, {9, -22}, {2, -18}, {26, 5}, {18, 9}, {50, 50}, {-3, 19}, {6, 14},
                     {-26, -5}, {-15, -11}, {-50, -31}});
    } else if (id == QLatin1String("heart")) {
        p.moveTo(0, 50);
        p.cubicTo(0, 50, -32, 35, -45, 2);
        p.cubicTo(-65, -48, -17, -71, 0, -24);
        p.cubicTo(17, -71, 65, -48, 45, 2);
        p.cubicTo(32, 35, 0, 50, 0, 50);
        p.closeSubpath();
    } else if (id == QLatin1String("ovalCallout")) {
        p.moveTo(-28, 50);
        p.lineTo(-30, 29);
        arcBox(p, -50, -50, 50, 38, 131, 330);
        p.lineTo(-10, 37);
        p.lineTo(-28, 50);
        p.closeSubpath();
    } else if (id == QLatin1String("roundedRectangularCallout")) {
        p.moveTo(-34, -50);
        p.lineTo(34, -50);
        arcBox(p, 18, -50, 50, -18, 270, 90);
        p.lineTo(50, -34);
        p.lineTo(50, 22);
        arcBox(p, 18, 4, 50, 38, 0, 90);
        p.lineTo(34, 38);
        p.lineTo(-10, 38);
        p.lineTo(-22, 50);
        p.lineTo(-34, 38);
        arcBox(p, -50, 4, -18, 38, 90, 90);
        p.lineTo(-50, 22);
        p.lineTo(-50, -34);
        arcBox(p, -50, -50, -18, -18, 180, 90);
        p.closeSubpath();
    } else if (id == QLatin1String("cloudCallout")) {
        struct Box { qreal l, t, r, b; };
        const QHash<char, Box> boxes{
            {'A', {-40, -42, -10, -5}}, {'B', {-19, -47, 6, -14}}, {'C', {-1, -50, 22, -17}},
            {'D', {15, -50, 39, -17}},  {'E', {19, -40, 47, -10}}, {'F', {22, -25, 49, 9}},
            {'G', {10, -6, 36, 25}},    {'H', {-14, 2, 17, 36}},   {'I', {-39, 0, -7, 31}},
            {'J', {-47, -3, -27, 21}},  {'K', {-50, -21, -30, 2}}, {'1', {-36, 30, -23, 42}},
            {'2', {-39, 40, -32, 47}},  {'3', {-43, 45, -37, 50}},
        };
        struct Seg { char box; qreal start, sweep; };
        const Seg segs[] = {
            {'I', 103, 56}, {'J', 97, -30}, {'J', 67, 160}, {'K', 110, -20}, {'K', 90, 180},
            {'A', 162, 145}, {'A', 307, -10}, {'B', 220, 81}, {'C', 220, -4}, {'C', 216, 88},
            {'D', 229, -5}, {'D', 224, 120}, {'E', 291, 118}, {'E', 49, -24}, {'F', 320, 128},
            {'G', 356, -55}, {'G', 291, 184}, {'H', 0, 160}, {'H', 160, -10}, {'I', 51, 52},
            {'1', 280, 210}, {'2', 320, 190}, {'3', 300, 359.9}, {'2', 150, 170}, {'1', 120, 160},
        };
        for (const Seg &s : segs) {
            const Box b = boxes.value(s.box);
            arcBox(p, b.l, b.t, b.r, b.b, s.start, s.sweep);
        }
        p.closeSubpath();
    }
    return p;
}

} // namespace

const QStringList &galleryOrder()
{
    static const QStringList ids{
        QStringLiteral("line"), QStringLiteral("curve"), QStringLiteral("oval"), QStringLiteral("rectangle"),
        QStringLiteral("roundedRectangle"), QStringLiteral("polygon"), QStringLiteral("triangle"),
        QStringLiteral("rightTriangle"), QStringLiteral("diamond"), QStringLiteral("pentagon"),
        QStringLiteral("hexagon"), QStringLiteral("rightArrow"), QStringLiteral("leftArrow"),
        QStringLiteral("upArrow"), QStringLiteral("downArrow"), QStringLiteral("fourPointStar"),
        QStringLiteral("fivePointStar"), QStringLiteral("sixPointStar"),
        QStringLiteral("roundedRectangularCallout"), QStringLiteral("ovalCallout"),
        QStringLiteral("cloudCallout"), QStringLiteral("heart"), QStringLiteral("lightning"),
    };
    return ids;
}

bool hasTemplate(const QString &id)
{
    return galleryOrder().contains(id) && id != QLatin1String("line") && id != QLatin1String("curve")
        && id != QLatin1String("polygon");
}

QPainterPath templatePath(const QString &id)
{
    static QHash<QString, QPainterPath> cache;
    auto it = cache.find(id);
    if (it == cache.end()) it = cache.insert(id, build(id));
    return *it;
}

QPainterPath fitted(const QString &id, const QRectF &box, qreal angleDeg)
{
    const QRectF r = box.normalized();
    QTransform t = QTransform::fromScale(r.width() / 100.0, r.height() / 100.0);
    if (angleDeg != 0.0) t *= QTransform().rotate(angleDeg);
    t *= QTransform::fromTranslate(r.center().x(), r.center().y());
    return t.map(templatePath(id));
}

QString toSvg(const QPainterPath &path)
{
    QString s;
    s.reserve(path.elementCount() * 16);
    auto num = [](qreal v) { return QString::number(v, 'f', 2); };
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element e = path.elementAt(i);
        switch (e.type) {
        case QPainterPath::MoveToElement: s += QLatin1String("M ") + num(e.x) + ' ' + num(e.y) + ' '; break;
        case QPainterPath::LineToElement: s += QLatin1String("L ") + num(e.x) + ' ' + num(e.y) + ' '; break;
        case QPainterPath::CurveToElement: {
            const auto c2 = path.elementAt(i + 1), end = path.elementAt(i + 2);
            s += QLatin1String("C ") + num(e.x) + ' ' + num(e.y) + ' ' + num(c2.x) + ' ' + num(c2.y) + ' '
                + num(end.x) + ' ' + num(end.y) + ' ';
            i += 2;
            break;
        }
        default: break;
        }
    }
    return s.trimmed();
}

QPoint constrainLine(QPoint p0, QPoint p1)
{
    const int dx = std::abs(p1.x() - p0.x());
    const int dy = std::abs(p1.y() - p0.y());
    if (dx == 0) return QPoint(p0.x(), p1.y());
    const int q = (dy * 1000) / dx;
    if (q >= 2414) return QPoint(p0.x(), p1.y());
    if (q < 414) return QPoint(p1.x(), p0.y());
    const int d = (dy + dx) / 2;
    return QPoint(p0.x() + (p1.x() < p0.x() ? -d : d), p0.y() + (p1.y() < p0.y() ? -d : d));
}

} // namespace mp::shapes
