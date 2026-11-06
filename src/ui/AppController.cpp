#include "ui/AppController.h"
// --- Archivos e impresión ---
#include "ui/DesktopBackground.h"
#include "ui/Printing.h"
#include <QDir>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QStandardPaths>
// ---
#include "core/History.h"
#include "core/ImageOps.h"
#include "core/ShapeGeometry.h"
#include "core/tools/Fill.h"
#include "core/tools/Select.h"
#include "core/tools/Shape.h"
#include "core/tools/Stroke.h"
#include "core/tools/Text.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QTransform>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QLocale>
#include <QDateTime>
#include <QPainter>
#include <QScreen>
#include <algorithm>
#include <cmath>

namespace {

// Pasos de zoom: 12,5 % … 1000 %; mínimo absoluto 10 %.
const qreal kZoomSteps[] = {0.125, 0.25, 0.5, 0.75, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
constexpr qreal kZoomMin = 0.10;
constexpr qreal kZoomMax = 10.0;

// Grosor por defecto y rango de cada herramienta o variante.
struct SizeSpec { int def, min, max; };
const std::map<QString, SizeSpec> &sizeSpecs()
{
    static const std::map<QString, SizeSpec> m{
        {QStringLiteral("pencil"), {1, 1, 248}},
        {QStringLiteral("brush"), {3, 1, 248}},
        {QStringLiteral("calligraphyBrush"), {5, 1, 248}},
        {QStringLiteral("calligraphyPen"), {5, 1, 248}},
        {QStringLiteral("airbrush"), {8, 4, 496}},
        {QStringLiteral("oilBrush"), {30, 4, 496}},
        {QStringLiteral("crayon"), {30, 4, 496}},
        {QStringLiteral("marker"), {30, 4, 496}},
        {QStringLiteral("naturalPencil"), {4, 1, 248}},
        {QStringLiteral("watercolor"), {30, 4, 496}},
        {QStringLiteral("eraser"), {8, 1, 248}},
        {QStringLiteral("shape"), {5, 1, 50}},
    };
    return m;
}

// Valores del slider de tamaño por tramos con paso que se duplica.
QList<int> sizeSteps(int min, int max)
{
    struct Seg { int from, to, step; };
    std::vector<Seg> segs;
    if (min == 1 && max == 248) segs = {{1, 24, 1}, {24, 104, 2}, {104, 172, 4}, {172, 248, 8}};
    else if (min == 4 && max == 496) segs = {{4, 24, 1}, {24, 104, 2}, {104, 184, 4}, {184, 384, 8}, {384, 496, 16}};
    QList<int> out;
    if (segs.empty()) { for (int v = min; v <= max; ++v) out << v; return out; }
    out << segs.front().from;
    for (const Seg &sg : segs) {
        int v = out.last();
        while (v < sg.to) { v = std::min(v + sg.step, sg.to); out << v; }
    }
    return out;
}

QSize defaultCanvasSize()
{
    // Lienzo nuevo: 60 % de la pantalla.
    if (QScreen *s = QGuiApplication::primaryScreen()) {
        const QSize full = s->size();
        if (!full.isEmpty()) return QSize(int(full.width() * 0.6), int(full.height() * 0.6));
    }
    return QSize(1152, 648);
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_doc(new mp::Document(this))
{
    using mp::brushes::Media;
    m_ctx.doc = m_doc;
    m_tools.emplace(QStringLiteral("pencil"), std::make_unique<mp::StrokeTool>(QStringLiteral("pencil"), Media::Pencil));
    m_tools.emplace(QStringLiteral("brush"), std::make_unique<mp::StrokeTool>(QStringLiteral("brush"), Media::Brush));
    m_tools.emplace(QStringLiteral("eraser"), std::make_unique<mp::StrokeTool>(QStringLiteral("eraser"), Media::SquareEraser,
                                                                               mp::StrokeTool::Kind::Eraser));
    m_tools.emplace(QStringLiteral("fill"), std::make_unique<mp::FillTool>());
    m_tools.emplace(QStringLiteral("picker"), std::make_unique<mp::ColorPickerTool>());
    // --- Herramientas ---
    m_tools.emplace(QStringLiteral("select"), std::make_unique<mp::SelectTool>());
    m_tools.emplace(QStringLiteral("shape"), std::make_unique<mp::ShapeTool>());
    m_tools.emplace(QStringLiteral("text"), std::make_unique<mp::TextTool>());
    connect(this, &AppController::toolOptionsChanged, this, &AppController::syncToolOptions);
    connect(this, &AppController::colorsChanged, this, &AppController::syncToolOptions);
    connect(this, &AppController::toolSizeChanged, this, &AppController::syncToolOptions);
    connect(this, &AppController::zoomChanged, this, &AppController::syncToolOptions);
    m_tool = QStringLiteral("pencil");
    for (const auto &[k, spec] : sizeSpecs()) m_sizes[k] = spec.def;
    m_ctx.size = m_sizes[sizeKey()];

    m_airTimer = new QTimer(this);
    m_airTimer->setInterval(30); // intervalo entre pulsos del aerógrafo
    connect(m_airTimer, &QTimer::timeout, this, [this] {
        if (auto *t = dynamic_cast<mp::StrokeTool *>(currentTool()); t && t->wantsTicks()) t->tick(m_ctx);
        else m_airTimer->stop();
    });

    connect(m_doc->history(), &mp::History::changed, this, &AppController::historyChanged);
    connect(m_doc, &mp::Document::structureChanged, this, [this] { ++m_layersRevision; emit layersChanged(); });
    connect(m_doc, &mp::Document::changed, this, [this] { ++m_layersRevision; emit layersChanged(); });
    connect(m_doc, &mp::Document::sizeChanged, this, &AppController::canvasSizeChanged);
    m_doc->reset(defaultCanvasSize(), Qt::white);
}

AppController::~AppController() = default;

mp::Tool *AppController::currentTool() const
{
    auto it = m_tools.find(m_tool);
    return it == m_tools.end() ? nullptr : it->second.get();
}

QString AppController::sizeKey() const { return m_tool == QLatin1String("brush") ? m_brush : m_tool; }
QString AppController::opacityKey() const { return sizeKey(); }

void AppController::setTool(const QString &id)
{
    if (id == m_tool) return;
    if (auto *t = currentTool()) t->commit(m_ctx);
    m_sizes[sizeKey()] = m_ctx.size;
    m_opacities[opacityKey()] = m_ctx.opacity;
    if (id == QLatin1String("picker")) m_previousTool = m_tool;
    m_tool = id;
    m_ctx.size = m_sizes.count(sizeKey()) ? m_sizes[sizeKey()] : 1;
    m_ctx.opacity = m_opacities.count(opacityKey()) ? m_opacities[opacityKey()] : 1.0;
    emit toolChanged();
    emit toolSizeChanged();
    emit toolOptionsChanged();
    emit overlayChanged();
    afterToolEvent();
}

void AppController::setBrush(const QString &id)
{
    if (id == m_brush) return;
    const bool active = m_tool == QLatin1String("brush");
    if (active) { m_sizes[sizeKey()] = m_ctx.size; m_opacities[opacityKey()] = m_ctx.opacity; }
    m_brush = id;
    if (auto *t = dynamic_cast<mp::StrokeTool *>(m_tools[QStringLiteral("brush")].get()))
        t->setMedia(mp::brushes::mediaFromId(id));
    if (active) {
        m_ctx.size = m_sizes.count(sizeKey()) ? m_sizes[sizeKey()] : 1;
        m_ctx.opacity = m_opacities.count(opacityKey()) ? m_opacities[opacityKey()] : 1.0;
        emit toolSizeChanged();
    }
    emit toolOptionsChanged();
}

int AppController::toolSizeMin() const { auto it = sizeSpecs().find(sizeKey()); return it == sizeSpecs().end() ? 1 : it->second.min; }
int AppController::toolSizeMax() const { auto it = sizeSpecs().find(sizeKey()); return it == sizeSpecs().end() ? 1 : it->second.max; }
bool AppController::toolHasSize() const { return sizeSpecs().count(sizeKey()) > 0; }
bool AppController::toolHasOpacity() const
{
    // Opacidad: pinceles y lápiz (atributos 0x78–0x81); el borrador solo con una función experimental.
    return m_tool == QLatin1String("brush") || m_tool == QLatin1String("pencil");
}

QVariantList AppController::toolSizeSteps() const
{
    QVariantList out;
    for (int v : sizeSteps(toolSizeMin(), toolSizeMax())) out << v;
    return out;
}

void AppController::stepToolSize(int delta)
{
    const QList<int> steps = sizeSteps(toolSizeMin(), toolSizeMax());
    int i = 0;
    while (i < steps.size() - 1 && steps[i] < m_ctx.size) ++i;
    setToolSize(steps[std::clamp(i + delta, 0, int(steps.size()) - 1)]);
}

void AppController::setColor1(const QColor &c) { if (c != m_ctx.color1) { m_ctx.color1 = c; emit colorsChanged(); } }
void AppController::setColor2(const QColor &c) { if (c != m_ctx.color2) { m_ctx.color2 = c; emit colorsChanged(); } }
void AppController::setActiveColor(int w) { if (w != m_activeColor && (w == 1 || w == 2)) { m_activeColor = w; emit colorsChanged(); } }
void AppController::pickPaletteColor(const QColor &c) { m_activeColor == 1 ? setColor1(c) : setColor2(c); }

void AppController::setToolSize(int s)
{
    s = std::clamp(s, toolSizeMin(), toolSizeMax());
    if (s == m_ctx.size) return;
    m_ctx.size = s;
    m_sizes[sizeKey()] = s;
    emit toolSizeChanged();
}

void AppController::setZoom(qreal z)
{
    z = std::clamp(z, kZoomMin, kZoomMax);
    if (qFuzzyCompare(z, m_zoom)) return;
    m_zoom = z;
    emit zoomChanged();
}

void AppController::zoomIn()
{
    for (qreal s : kZoomSteps)
        if (s > m_zoom + 1e-6) { setZoom(s); return; }
}

void AppController::zoomOut()
{
    for (auto it = std::rbegin(kZoomSteps); it != std::rend(kZoomSteps); ++it)
        if (*it < m_zoom - 1e-6) { setZoom(*it); return; }
}

bool AppController::canUndo() const { return m_doc->history()->canUndo(); }
bool AppController::canRedo() const { return m_doc->history()->canRedo(); }
bool AppController::modified() const { return !m_doc->history()->isClean(); }

QString AppController::fileName() const
{
    return m_fileUrl.isEmpty() ? tr("Untitled") : QFileInfo(m_fileUrl.toLocalFile()).fileName();
}

void AppController::setShowGridlines(bool v) { if (v != m_gridlines) { m_gridlines = v; emit viewChanged(); emit overlayChanged(); } }
void AppController::setShowRulers(bool v) { if (v != m_rulers) { m_rulers = v; emit viewChanged(); } }
void AppController::setShowStatusBar(bool v) { if (v != m_statusBar) { m_statusBar = v; emit viewChanged(); } }

void AppController::newImage()
{
    m_doc->reset(defaultCanvasSize(), Qt::white);
    m_fileUrl = QUrl();
    m_doc->setDpi(96.0);                         // --- Archivos e impresión ---
    m_fileFormat = mp::files::Format::Unknown;
    m_docBpp = 32;
    emit fileChanged();
    emit historyChanged();
}

bool AppController::openFile(const QUrl &url)
{
    // --- Archivos e impresión ---
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    mp::files::ImageData data;
    QString err;
    if (!mp::files::load(path, data, &err)) {
        qWarning("open %s: %s", qPrintable(path), qPrintable(err));
        emit errorMessage(tr("This file can't be opened."));
        return false;
    }
    loadIntoDocument(data);
    m_fileUrl = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
    m_fileFormat = data.format;
    m_docBpp = data.bitsPerPixel;
    addRecentFile(m_fileUrl.toLocalFile());
    emit fileChanged();
    emit historyChanged();
    return true;
}

bool AppController::importFile(const QUrl &url)
{
    // Provisorio hasta tener la selección flotante: entra como capa nueva en la esquina superior izquierda.
    QImageReader r(url.toLocalFile());
    r.setAutoTransform(true);
    const QImage img = r.read();
    if (img.isNull()) return false;
    cancelPending();
    mp::ops::addLayer(*m_doc);
    QImage before = m_doc->active().image;
    before.detach();
    QPainter p(&m_doc->active().image);
    p.drawImage(0, 0, img);
    p.end();
    m_doc->history()->pushLayerEdit(m_doc->activeLayer(), before);
    m_doc->notifyChanged(m_doc->rect());
    return true;
}

bool AppController::saveFile(const QUrl &url)
{
    // --- Archivos e impresión --- sin avisos; el flujo con avisos lo arma Main.qml.
    const QUrl target = url.isEmpty() ? m_fileUrl : url;
    if (target.isEmpty()) return false;
    const QString fmt = url.isEmpty() ? fileFormat() : formatForSave(target, fileFormat());
    return saveDocumentAs(target, fmt);
}

void AppController::setOpacityPercent(int v)
{
    v = std::clamp(v, 0, 100);
    if (v == opacityPercent()) return;
    m_ctx.opacity = v / 100.0;
    emit toolOptionsChanged();
}

void AppController::setTolerance(int v)
{
    v = std::clamp(v, 0, 100);
    if (v == m_ctx.tolerance) return;
    m_ctx.tolerance = v;
    emit toolOptionsChanged();
}

void AppController::addCustomColor(const QColor &c)
{
    m_customColors.prepend(c);
    while (m_customColors.size() > 10) m_customColors.removeLast();
    emit colorsChanged();
}

void AppController::swapColors()
{
    std::swap(m_ctx.color1, m_ctx.color2);
    emit colorsChanged();
}

// --- Herramientas: selección ---
static mp::SelectTool *asSelect(mp::Tool *t) { return dynamic_cast<mp::SelectTool *>(t); }

bool AppController::hasSelection() const
{
    auto *s = asSelect(m_tools.at(QStringLiteral("select")).get());
    return m_tool == QLatin1String("select") && s->hasSelection();
}

QSize AppController::selectionSize() const
{
    return hasSelection() ? asSelect(m_tools.at(QStringLiteral("select")).get())->selectionSize() : QSize();
}

// Con selección, girar/voltear/cambiar tamaño actúan solo sobre el contenido seleccionado.
#define MSEPAINT_ON_SELECTION(expr)                                                   \
    if (hasSelection()) {                                                             \
        auto *sel = asSelect(currentTool());                                          \
        expr;                                                                         \
        afterToolEvent();                                                             \
        return;                                                                       \
    }

void AppController::rotateRight() { MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().rotate(90))) cancelPending(); mp::ops::rotate90(*m_doc, true); }
void AppController::rotateLeft() { MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().rotate(-90))) cancelPending(); mp::ops::rotate90(*m_doc, false); }
void AppController::rotate180() { MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().rotate(180))) cancelPending(); mp::ops::rotate180(*m_doc); }
void AppController::flipHorizontal() { MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().scale(-1, 1))) cancelPending(); mp::ops::flipHorizontal(*m_doc); }
void AppController::flipVertical() { MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().scale(1, -1))) cancelPending(); mp::ops::flipVertical(*m_doc); }
void AppController::resizeImage(int w, int h) { MSEPAINT_ON_SELECTION(sel->resizeFloat(m_ctx, QSize(w, h))) cancelPending(); mp::ops::resample(*m_doc, QSize(w, h)); }
void AppController::resizeCanvas(int w, int h) { cancelPending(); mp::ops::resizeCanvas(*m_doc, QSize(w, h)); }

// --- Diálogos de imagen ---

void AppController::setUnits(int u)
{
    u = std::clamp(u, 0, 2);
    if (u == m_units) return;
    m_units = u;
    emit unitsChanged();
}

QString AppController::appVersion() const { return QStringLiteral(MSEPAINT_VERSION); }

void AppController::resizeAndSkew(int width, int height, int skewH, int skewV)
{
    // Con selección: el tamaño se aplica al contenido flotante (la inclinación de la selección queda pendiente).
    MSEPAINT_ON_SELECTION(sel->resizeFloat(m_ctx, QSize(std::clamp(width, 1, 99999), std::clamp(height, 1, 99999))))
    cancelPending();
    const QSize target(std::clamp(width, 1, 99999), std::clamp(height, 1, 99999));
    const int mark = m_doc->history()->mark();
    if (target != m_doc->size()) mp::ops::resample(*m_doc, target);
    if (mp::ops::isValidSkew(skewH)) mp::ops::skewHorizontal(*m_doc, skewH);
    if (mp::ops::isValidSkew(skewV)) mp::ops::skewVertical(*m_doc, skewV);
    m_doc->history()->squashSince(mark); // un solo paso de deshacer
}

void AppController::rotateCustom(qreal degrees)
{
    MSEPAINT_ON_SELECTION(sel->transformFloat(m_ctx, QTransform().rotate(degrees)))
    cancelPending();
    mp::ops::rotateBy(*m_doc, degrees);
}

QVariantMap AppController::imageFileInfo() const
{
    QVariantMap m;
    m.insert(QStringLiteral("lastSaved"), QString());
    m.insert(QStringLiteral("sizeOnDisk"), QString());
    if (m_fileUrl.isEmpty()) return m;
    const QFileInfo fi(m_fileUrl.toLocalFile());
    if (!fi.exists()) return m;
    const QLocale loc;
    const QDateTime t = fi.lastModified();
    m.insert(QStringLiteral("lastSaved"), loc.toString(t.date(), QLocale::ShortFormat) + QLatin1Char(' ')
                                              + loc.toString(t.time(), QLocale::ShortFormat));
    m.insert(QStringLiteral("sizeOnDisk"), fileSizeText());
    return m;
}

QSize AppController::defaultImageSize() const { return defaultCanvasSize(); }

void AppController::applyImageProperties(int width, int height, int units)
{
    setUnits(units);
    const QSize s(std::max(1, width), std::max(1, height));
    if (s == m_doc->size()) return;
    cancelPending();
    mp::ops::resizeCanvas(*m_doc, s);
}

void AppController::invertColors()
{
    MSEPAINT_ON_SELECTION(sel->invertColors(m_ctx))
    cancelPending();
    QImage before = m_doc->active().image;
    before.detach();
    QImage &img = m_doc->active().image;
    img.invertPixels(QImage::InvertRgb);
    m_doc->history()->pushLayerEdit(m_doc->activeLayer(), before);
    m_doc->notifyChanged(m_doc->rect());
}

void AppController::selectAll()
{
    setTool(QStringLiteral("select"));
    asSelect(currentTool())->selectAll(m_ctx);
    afterToolEvent();
}

void AppController::invertSelection()
{
    setTool(QStringLiteral("select"));
    asSelect(currentTool())->invert(m_ctx);
    afterToolEvent();
}

void AppController::deleteSelection()
{
    if (!hasSelection()) return;
    asSelect(currentTool())->deleteContent(m_ctx);
    afterToolEvent();
}

void AppController::crop()
{
    if (!hasSelection()) return;
    asSelect(currentTool())->crop(m_ctx);
    afterToolEvent();
}

void AppController::cut()
{
    if (!hasSelection()) return;
    QImage img;
    asSelect(currentTool())->cut(m_ctx, &img);
    if (!img.isNull()) QGuiApplication::clipboard()->setImage(img);
    afterToolEvent();
}

void AppController::copy()
{
    if (!hasSelection()) return;
    const QImage img = asSelect(currentTool())->copyContent(m_ctx);
    if (!img.isNull()) QGuiApplication::clipboard()->setImage(img);
}

void AppController::paste()
{
    const QImage img = QGuiApplication::clipboard()->image();
    if (img.isNull()) return;
    setTool(QStringLiteral("select"));
    // Esquina superior izquierda de la parte visible del lienzo.
    const QPoint at(std::max(0, int(std::floor(m_viewOrigin.x()))), std::max(0, int(std::floor(m_viewOrigin.y()))));
    asSelect(currentTool())->paste(m_ctx, img, at);
    afterToolEvent();
}

void AppController::copyVisibleLayers()
{
    QGuiApplication::clipboard()->setImage(m_doc->flattenedLayers());
}

void AppController::cancelPending()
{
    if (auto *t = currentTool(); t && t->hasPending()) { t->commit(m_ctx); emit overlayChanged(); }
}

void AppController::addLayer() { cancelPending(); mp::ops::addLayer(*m_doc); }
void AppController::duplicateLayer() { cancelPending(); mp::ops::duplicateLayer(*m_doc); }
void AppController::deleteLayer() { cancelPending(); mp::ops::deleteLayer(*m_doc); }
void AppController::mergeDown() { cancelPending(); mp::ops::mergeDown(*m_doc); }
void AppController::moveLayerUp() { mp::ops::moveLayer(*m_doc, m_doc->activeLayer(), m_doc->activeLayer() + 1); }
void AppController::moveLayerDown() { mp::ops::moveLayer(*m_doc, m_doc->activeLayer(), m_doc->activeLayer() - 1); }
void AppController::toggleLayerVisible(int i)
{
    if (i < 0 || i >= m_doc->layerCount()) return;
    mp::ops::setLayerVisible(*m_doc, i, !m_doc->layer(i).visible);
}

void AppController::toggleBackgroundVisible()
{
    mp::ops::setBackground(*m_doc, m_doc->background(), !m_doc->backgroundVisible());
}

void AppController::setBackgroundColor(const QColor &c)
{
    mp::ops::setBackground(*m_doc, c, m_doc->backgroundVisible());
}

QString AppController::fileSizeText() const
{
    // 1 decimal; KB hasta 1 MiB, MB hasta 1 GiB, GB encima.
    if (m_fileUrl.isEmpty()) return {};
    const QFileInfo fi(m_fileUrl.toLocalFile());
    if (!fi.exists()) return {};
    const double b = double(fi.size());
    if (b < 1024.0 * 1024.0) return QString::number(b / 1024.0, 'f', 1) + QStringLiteral("KB");
    if (b < 1024.0 * 1024.0 * 1024.0) return QString::number(b / (1024.0 * 1024.0), 'f', 1) + QStringLiteral("MB");
    return QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 1) + QStringLiteral("GB");
}

QStringList AppController::shapeIds() const { return mp::shapes::galleryOrder(); }

QString AppController::shapeIconPath(const QString &id, qreal size) const
{
    const qreal m = 1.0, e = size - 1.0;
    QPainterPath p;
    if (id == QLatin1String("line")) {
        p.moveTo(m, e);
        p.lineTo(e, m);
    } else if (id == QLatin1String("curve")) {
        p.moveTo(m, e);
        p.cubicTo(size * 0.2, m, size * 0.8, e, e, m);
    } else if (id == QLatin1String("polygon")) {
        p.moveTo(size * 0.35, m);
        p.lineTo(e, size * 0.3);
        p.lineTo(size * 0.7, e);
        p.lineTo(size * 0.2, size * 0.8);
        p.lineTo(m, size * 0.4);
        p.closeSubpath();
    } else {
        p = mp::shapes::fitted(id, QRectF(m, m, size - 2 * m, size - 2 * m));
    }
    return mp::shapes::toSvg(p);
}

void AppController::fitToWindow(qreal vw, qreal vh)
{
    const QSize s = m_doc->size();
    if (s.isEmpty() || vw <= 0 || vh <= 0) return;
    setZoom(std::min(1.0, std::min(vw / s.width(), vh / s.height())));
}

void AppController::undo() { if (auto *t = currentTool(); t && t->hasPending()) { t->cancel(m_ctx); emit overlayChanged(); return; } m_doc->history()->undo(); }
void AppController::redo() { m_doc->history()->redo(); }

void AppController::setCursor(QPointF p, bool inside)
{
    const QPoint c(int(std::floor(p.x())), int(std::floor(p.y())));
    if (c == m_cursor && inside == m_cursorInside) return;
    m_cursor = c;
    m_cursorInside = inside;
    emit cursorChanged();
}

static mp::ToolEvent makeEvent(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m)
{
    mp::ToolEvent ev;
    ev.pos = p;
    ev.button = b;
    ev.modifiers = m;
    return ev;
}

void AppController::pointerPress(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m)
{
    setCursor(p, m_doc->rect().contains(p.toPoint()));
    // Lupa: izquierdo acerca, derecho aleja, centrando en el punto.
    if (m_tool == QLatin1String("magnifier")) {
        if (b == Qt::LeftButton) zoomIn(); else if (b == Qt::RightButton) zoomOut();
        emit centerViewOn(p);
        return;
    }
    if (m_tool == QLatin1String("select") && b == Qt::RightButton) {
        emit contextMenuRequested();
        return;
    }
    const QColor c1 = m_ctx.color1, c2 = m_ctx.color2;
    if (auto *t = currentTool()) { t->press(m_ctx, makeEvent(p, b, m)); emit overlayChanged(); }
    if (c1 != m_ctx.color1 || c2 != m_ctx.color2) emit colorsChanged();
    if (auto *st = dynamic_cast<mp::StrokeTool *>(currentTool()); st && st->wantsTicks()) m_airTimer->start();
    afterToolEvent();
}

void AppController::pointerMove(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m)
{
    setCursor(p, m_doc->rect().contains(p.toPoint()));
    if (auto *t = currentTool()) {
        const QColor c1 = m_ctx.color1, c2 = m_ctx.color2;
        if (b == Qt::NoButton) t->hover(m_ctx, makeEvent(p, b, m));
        else t->move(m_ctx, makeEvent(p, b, m));
        if (c1 != m_ctx.color1 || c2 != m_ctx.color2) emit colorsChanged();
        emit overlayChanged();
    }
    afterToolEvent();
}

void AppController::pointerRelease(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m)
{
    m_airTimer->stop();
    if (auto *t = currentTool()) { t->release(m_ctx, makeEvent(p, b, m)); emit overlayChanged(); }
    // El selector de color vuelve a la herramienta anterior.
    if (m_tool == QLatin1String("picker") && !m_previousTool.isEmpty() && m_previousTool != m_tool)
        setTool(m_previousTool);
    afterToolEvent();
}

void AppController::pointerHover(QPointF p, Qt::KeyboardModifiers m)
{
    setCursor(p, m_doc->rect().contains(p.toPoint()));
    if (auto *t = currentTool()) { t->hover(m_ctx, makeEvent(p, Qt::NoButton, m)); emit overlayChanged(); }
    afterToolEvent();
}

void AppController::pointerDoubleClick(QPointF)
{
    if (auto *sh = dynamic_cast<mp::ShapeTool *>(currentTool())) { sh->doubleClick(m_ctx); emit overlayChanged(); }
    afterToolEvent();
}

// --- Herramientas: estado común ---
void AppController::afterToolEvent()
{
    int shape = Qt::CrossCursor;
    if (auto *s = dynamic_cast<mp::SelectTool *>(currentTool())) shape = s->cursorShape();
    else if (auto *sh = dynamic_cast<mp::ShapeTool *>(currentTool())) shape = sh->cursorShape();
    else if (auto *tx = dynamic_cast<mp::TextTool *>(currentTool())) shape = tx->cursorShape();
    if (shape != m_cursorShape) { m_cursorShape = shape; emit cursorShapeChanged(); }
    emit selectionChanged();
    emit textChanged();
}

int AppController::cursorShape() const { return m_cursorShape; }

void AppController::syncToolOptions()
{
    auto *sel = asSelect(m_tools.at(QStringLiteral("select")).get());
    sel->freeForm = m_selectMode == QLatin1String("freeform");
    sel->transparent = m_transparentSelection;
    sel->setScale(m_zoom);
    auto *sh = dynamic_cast<mp::ShapeTool *>(m_tools.at(QStringLiteral("shape")).get());
    const bool restyle = sh->shape == m_shape && (sh->outline != m_shapeOutline || sh->fill != m_shapeFill);
    if (sh->shape != m_shape && sh->hasPending()) sh->commit(m_ctx);
    sh->shape = m_shape;
    sh->outline = m_shapeOutline;
    sh->fill = m_shapeFill;
    sh->setScale(m_zoom);
    if (m_tool == QLatin1String("shape") && sh->hasPending()) { sh->restyle(m_ctx); Q_UNUSED(restyle); }
    auto *tx = dynamic_cast<mp::TextTool *>(m_tools.at(QStringLiteral("text")).get());
    tx->setScale(m_zoom);
    emit overlayChanged();
}

static mp::TextTool *asText(const std::map<QString, std::unique_ptr<mp::Tool>> &tools)
{
    return dynamic_cast<mp::TextTool *>(tools.at(QStringLiteral("text")).get());
}

bool AppController::textEditing() const { return m_tool == QLatin1String("text") && asText(m_tools)->editing(); }
mp::TextFormat AppController::textFormat() const { return asText(m_tools)->format(); }
void AppController::setTextFormat(const mp::TextFormat &f)
{
    asText(m_tools)->setFormat(m_ctx, f);
    emit textChanged();
    emit overlayChanged();
}
QString AppController::textFamily() const { return textFormat().family.isEmpty() ? QGuiApplication::font().family() : textFormat().family; }
void AppController::setTextFamily(const QString &v) { auto f = textFormat(); f.family = v; setTextFormat(f); }
qreal AppController::textSize() const { return textFormat().pointSize; }
void AppController::setTextSize(qreal v) { auto f = textFormat(); f.pointSize = std::clamp(v, 1.0, 1638.0); setTextFormat(f); }
bool AppController::textBold() const { return textFormat().bold; }
void AppController::setTextBold(bool v) { auto f = textFormat(); f.bold = v; setTextFormat(f); }
bool AppController::textItalic() const { return textFormat().italic; }
void AppController::setTextItalic(bool v) { auto f = textFormat(); f.italic = v; setTextFormat(f); }
bool AppController::textUnderline() const { return textFormat().underline; }
void AppController::setTextUnderline(bool v) { auto f = textFormat(); f.underline = v; setTextFormat(f); }
bool AppController::textStrike() const { return textFormat().strike; }
void AppController::setTextStrike(bool v) { auto f = textFormat(); f.strike = v; setTextFormat(f); }
int AppController::textAlign() const
{
    const auto a = textFormat().align;
    return (a & Qt::AlignHCenter) ? 1 : (a & Qt::AlignRight) ? 2 : 0;
}
void AppController::setTextAlign(int v)
{
    auto f = textFormat();
    f.align = v == 1 ? Qt::AlignHCenter : v == 2 ? Qt::AlignRight : Qt::AlignLeft;
    setTextFormat(f);
}
bool AppController::textFill() const { return textFormat().fillBackground; }
void AppController::setTextFill(bool v) { auto f = textFormat(); f.fillBackground = v; setTextFormat(f); }

QStringList AppController::fontFamilies() const { return QFontDatabase::families(); }

bool AppController::keyPress(int key, int modifiers, const QString &text)
{
    const auto mods = Qt::KeyboardModifiers(modifiers);
    if (auto *tx = dynamic_cast<mp::TextTool *>(currentTool()); tx && tx->editing()) {
        const bool used = tx->key(m_ctx, key, mods, text);
        emit overlayChanged();
        afterToolEvent();
        return used;
    }
    // Ctrl + «+»/«−» del teclado numérico: un paso de la lista de grosores.
    if ((mods & Qt::ControlModifier) && (mods & Qt::KeypadModifier) && toolHasSize()) {
        if (key == Qt::Key_Plus) { stepToolSize(+1); return true; }
        if (key == Qt::Key_Minus) { stepToolSize(-1); return true; }
    }
    // Flechas: mueven la selección 1 px.
    if (hasSelection() && (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down)) {
        const QPoint d(key == Qt::Key_Left ? -1 : key == Qt::Key_Right ? 1 : 0, key == Qt::Key_Up ? -1 : key == Qt::Key_Down ? 1 : 0);
        asSelect(currentTool())->nudge(m_ctx, d);
        emit overlayChanged();
        afterToolEvent();
        return true;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (auto *t = currentTool(); t && t->hasPending()) { t->commit(m_ctx); emit overlayChanged(); afterToolEvent(); return true; }
    }
    return false;
}

void AppController::inputText(const QString &text)
{
    if (auto *tx = dynamic_cast<mp::TextTool *>(currentTool()); tx && tx->editing()) {
        tx->insertText(text);
        emit overlayChanged();
    }
}

void AppController::pointerLeave() { setCursor(QPointF(-1, -1), false); }

void AppController::paintOverlay(QPainter &p, qreal scale) const
{
    if (auto *t = currentTool()) t->paintOverlay(p, scale);
}

// --- Archivos e impresión ---------------------------------------------------------------------------------------------
// Formatos, avisos al guardar, recientes, ajustes persistentes, impresión y fondo de escritorio.

namespace {

constexpr int kMaxRecent = 9; // LoadStdProfileSettings(9)

// Un solo QSettings en INI, organización y aplicación «msepaint».
std::unique_ptr<QSettings> openSettings()
{
    return std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("msepaint"),
                                       QStringLiteral("msepaint"));
}

QString filterText(mp::files::Format f)
{
    return mp::files::formatLabel(f) + QStringLiteral(" (") + mp::files::formatPatterns(f).join(QLatin1Char(' ')) + QLatin1Char(')');
}

} // namespace

QString AppController::windowTitle() const
{
    // «<documento> - msepaint».
    return fileName() + QStringLiteral(" - msepaint");
}

QString AppController::fileFormat() const { return mp::files::formatId(m_fileFormat); }
bool AppController::hasProjectSupport() const { return mp::files::hasProjectSupport(); }
bool AppController::canSetDesktopBackground() const { return desktop::detect() != desktop::Kind::None; }

QVariantList AppController::saveFormats() const
{
    QVariantList out;
    for (mp::files::Format f : mp::files::writableFormats())
        out << QVariantMap{{QStringLiteral("id"), mp::files::formatId(f)}, {QStringLiteral("filter"), filterText(f)}};
    return out;
}

QStringList AppController::openNameFilters() const
{
    // Orden: Bitmap, un filtro por códec, proyecto, todas las imágenes, todos los archivos.
    using mp::files::Format;
    QStringList out{tr("Bitmap Files") + QStringLiteral(" (*.bmp *.dib *.rle)")};
    for (Format f : {Format::Jpeg, Format::Gif, Format::Tiff, Format::Png, Format::Ico, Format::Webp, Format::Heif, Format::Avif})
        if (mp::files::canRead(f)) out << filterText(f);
    if (mp::files::hasProjectSupport()) out << tr("msepaint project") + QStringLiteral(" (*.paint)");
    QStringList all;
    for (const QString &s : mp::files::openSuffixes()) all << QStringLiteral("*.") + s;
    out << tr("All Picture Files") + QStringLiteral(" (") + all.join(QLatin1Char(' ')) + QLatin1Char(')');
    out << tr("All Files") + QStringLiteral(" (*)");
    return out;
}

QString AppController::formatForSave(const QUrl &url, const QString &filterFormat) const
{
    using mp::files::Format;
    const Format filter = mp::files::formatFromId(filterFormat);
    const Format bySuffix = mp::files::formatFromSuffix(QFileInfo(url.toLocalFile()).suffix());
    auto isBmp = [](Format f) { return f == Format::Bmp1 || f == Format::Bmp4 || f == Format::Bmp8 || f == Format::Bmp24; };
    Format f = filter;
    if (bySuffix != Format::Unknown && mp::files::canWrite(bySuffix) && !(isBmp(bySuffix) && isBmp(filter))) f = bySuffix;
    if (f == Format::Unknown || !mp::files::canWrite(f)) f = Format::Png; // extensión por defecto .png
    return mp::files::formatId(f);
}

int AppController::saveFilterIndex(const QString &format) const
{
    const QList<mp::files::Format> list = mp::files::writableFormats();
    const mp::files::Format f = mp::files::formatFromId(format.isEmpty() ? fileFormat() : format);
    const qsizetype i = list.indexOf(f);
    return int(i >= 0 ? i : list.indexOf(mp::files::Format::Png));
}

QUrl AppController::suggestedSaveUrl(const QString &format) const
{
    const QString suffix = mp::files::defaultSuffix(mp::files::formatFromId(format));
    if (!m_fileUrl.isEmpty()) {
        const QFileInfo fi(m_fileUrl.toLocalFile());
        return QUrl::fromLocalFile(fi.absoluteDir().filePath(fi.completeBaseName() + QLatin1Char('.') + suffix));
    }
    QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (dir.isEmpty() || !QDir(dir).exists()) dir = QDir::homePath();
    return QUrl::fromLocalFile(QDir(dir).filePath(tr("Untitled") + QLatin1Char('.') + suffix));
}

bool AppController::needsFlattenPrompt(const QString &format) const
{
    return m_doc->layerCount() >= 2 && !mp::files::isProject(mp::files::formatFromId(format));
}

bool AppController::needsColorWarning(const QString &format) const
{
    const int bpp = mp::files::bitsPerPixel(mp::files::formatFromId(format));
    return bpp < 24 && bpp < m_docBpp;
}

bool AppController::saveDocumentAs(const QUrl &url, const QString &format, bool copy)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.isEmpty()) return false;
    mp::files::Format f = mp::files::formatFromId(format);
    if (f == mp::files::Format::Unknown) f = mp::files::formatFromId(formatForSave(QUrl::fromLocalFile(path), QString()));
    cancelPending(); // confirma el trazo, texto o selección en curso antes de guardar
    const double dpi = m_doc->dpi();
    QString err;
    if (!mp::files::save(path, f, mp::files::fromDocument(*m_doc, dpi, dpi), &err)) {
        qWarning("save %s: %s", qPrintable(path), qPrintable(err));
        emit errorMessage(tr("This file can't be saved."));
        return false;
    }
    addRecentFile(QFileInfo(path).absoluteFilePath());
    if (copy) return true;
    m_fileUrl = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
    m_fileFormat = f;
    m_docBpp = mp::files::bitsPerPixel(f);
    m_doc->history()->setClean();
    emit fileChanged();
    emit historyChanged();
    return true;
}

bool AppController::isOpenableFile(const QUrl &url) const
{
    return url.isLocalFile() && mp::files::openSuffixes().contains(QFileInfo(url.toLocalFile()).suffix().toLower());
}

QString AppController::recentDisplayName(const QString &path) const { return QFileInfo(path).fileName(); }

void AppController::addRecentFile(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > kMaxRecent) m_recentFiles.removeLast();
    if (!devMode()) openSettings()->setValue(QStringLiteral("RecentFiles/files"), m_recentFiles);
    emit recentFilesChanged();
}

void AppController::loadIntoDocument(const mp::files::ImageData &data)
{
    if (auto *t = currentTool(); t && t->hasPending()) { t->cancel(m_ctx); emit overlayChanged(); }
    std::vector<mp::Layer> layers = data.layers;
    for (size_t i = 0; i < layers.size(); ++i) // el .paint no guarda nombres: «Layer {0}»
        if (layers[i].name.isEmpty()) layers[i].name = QStringLiteral("Layer %1").arg(i + 1);
    m_doc->replaceLayers(std::move(layers), data.size(), int(data.layers.size()) - 1);
    m_doc->setBackground(data.background, data.backgroundVisible);
    m_doc->setDpi(data.dpiX);
    m_doc->history()->clear();
}

QVariant AppController::settingValue(const QString &key, const QVariant &def) const { return openSettings()->value(key, def); }
void AppController::setSettingValue(const QString &key, const QVariant &value) { openSettings()->setValue(key, value); }

void AppController::loadSettings()
{
    auto s = openSettings();
    setShowRulers(s->value(QStringLiteral("View/showRulers"), m_rulers).toBool());
    setShowGridlines(s->value(QStringLiteral("View/showGrid"), m_gridlines).toBool());
    setShowStatusBar(s->value(QStringLiteral("View/showStatusBar"), m_statusBar).toBool());
    setUnits(s->value(QStringLiteral("View/unit"), m_units).toInt());
    const QColor c1(s->value(QStringLiteral("Colors/color1")).toString());
    const QColor c2(s->value(QStringLiteral("Colors/color2")).toString());
    if (c1.isValid()) setColor1(c1);
    if (c2.isValid()) setColor2(c2);
    if (s->contains(QStringLiteral("Colors/custom"))) {
        m_customColors.clear();
        for (const QString &n : s->value(QStringLiteral("Colors/custom")).toStringList())
            if (QColor c(n); c.isValid()) m_customColors << c;
        emit colorsChanged();
    }
    s->beginGroup(QStringLiteral("Tools"));
    for (const QString &k : s->childKeys()) m_sizes[k] = s->value(k).toInt();
    s->endGroup();
    if (m_sizes.count(sizeKey())) { m_ctx.size = m_sizes[sizeKey()]; emit toolSizeChanged(); }
    m_recentFiles = s->value(QStringLiteral("RecentFiles/files")).toStringList().mid(0, kMaxRecent);
    emit recentFilesChanged();
}

void AppController::saveSettings()
{
    auto s = openSettings();
    s->setValue(QStringLiteral("View/showRulers"), m_rulers);
    s->setValue(QStringLiteral("View/showGrid"), m_gridlines);
    s->setValue(QStringLiteral("View/showStatusBar"), m_statusBar);
    s->setValue(QStringLiteral("View/unit"), m_units);
    s->setValue(QStringLiteral("Colors/color1"), m_ctx.color1.name(QColor::HexArgb));
    s->setValue(QStringLiteral("Colors/color2"), m_ctx.color2.name(QColor::HexArgb));
    QStringList custom;
    for (const QVariant &v : m_customColors) custom << v.value<QColor>().name(QColor::HexArgb);
    s->setValue(QStringLiteral("Colors/custom"), custom);
    m_sizes[sizeKey()] = m_ctx.size;
    s->beginGroup(QStringLiteral("Tools"));
    for (const auto &[k, v] : m_sizes) s->setValue(k, v);
    s->endGroup();
    s->setValue(QStringLiteral("RecentFiles/files"), m_recentFiles);
}

bool AppController::print(bool showDialog)
{
    if (!m_page) m_page = std::make_unique<PageSettings>();
    cancelPending();
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageLayout(m_page->layout);
    printer.setDocName(fileName());
    if (showDialog) {
        QPrintDialog dlg(&printer);
        dlg.setWindowTitle(tr("Print"));
        if (dlg.exec() != QDialog::Accepted) return false;
    }
    if (!printing::render(&printer, m_doc->flattened(), m_doc->dpi(), m_doc->dpi(), *m_page)) {
        emit errorMessage(tr("The document couldn't be printed. Check the printer and try again."));
        return false;
    }
    return true;
}

void AppController::pageSetup()
{
    if (!m_page) m_page = std::make_unique<PageSettings>();
    printing::pageSetupDialog(*m_page, m_doc->size(), m_doc->dpi());
}

void AppController::printPreview()
{
    if (!m_page) m_page = std::make_unique<PageSettings>();
    cancelPending();
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageLayout(m_page->layout);
    printer.setDocName(fileName());
    const QImage image = m_doc->flattened();
    QPrintPreviewDialog dlg(&printer);
    dlg.setWindowTitle(tr("Print preview"));
    connect(&dlg, &QPrintPreviewDialog::paintRequested, &dlg, [&](QPrinter *p) {
        printing::render(p, image, m_doc->dpi(), m_doc->dpi(), *m_page);
    });
    dlg.exec();
}

bool AppController::setDesktopBackground(const QString &mode)
{
    if (m_fileUrl.isEmpty() || modified()) return false; // QML pide guardar antes (MustSaveWallpaper)
    QString path = m_fileUrl.toLocalFile();
    if (m_fileFormat == mp::files::Format::Paint) {
        // El escritorio no lee .paint: se usa una copia PNG aplanada.
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QDir().mkpath(dir);
        path = QDir(dir).filePath(QStringLiteral("wallpaper.png"));
        if (!mp::files::save(path, mp::files::Format::Png, mp::files::fromDocument(*m_doc, m_doc->dpi(), m_doc->dpi()))) {
            emit errorMessage(tr("This file can't be saved."));
            return false;
        }
    }
    QString err;
    if (!desktop::setWallpaper(desktop::detect(), path, mode, &err)) {
        qWarning("wallpaper: %s", qPrintable(err));
        emit errorMessage(tr("Set as desktop background") + QStringLiteral(": ") + err);
        return false;
    }
    return true;
}
