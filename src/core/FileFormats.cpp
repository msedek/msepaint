#include "core/FileFormats.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtEndian>
#include <cmath>
#include <unordered_map>

#ifdef MSEPAINT_HAVE_LIBHEIF
#include <libheif/heif.h>
#endif

namespace mp::files {

namespace {

constexpr double kInchPerMeter = 0.0254;

struct FormatRow {
    Format f;
    const char *id;
    const char *label;
    const char *patterns; // separados por ';'
    const char *qtName;   // nombre del códec de Qt (vacío = propio)
    int bpp;
    bool alpha;
};

// Etiquetas de los filtros: BMP por profundidad, códecs con nombre propio.
const FormatRow kRows[] = {
    {Format::Bmp1, "bmp1", "Monochrome Bitmap", "*.bmp;*.dib", "", 1, false},
    {Format::Bmp4, "bmp4", "16 Color Bitmap", "*.bmp;*.dib", "", 4, false},
    {Format::Bmp8, "bmp8", "256 Color Bitmap", "*.bmp;*.dib", "", 8, false},
    {Format::Bmp24, "bmp", "24-bit Bitmap", "*.bmp;*.dib", "", 24, false},
    {Format::Jpeg, "jpg", "JPEG", "*.jpg;*.jpeg;*.jpe;*.jfif", "jpeg", 24, false},
    {Format::Gif, "gif", "GIF", "*.gif", "", 8, false},
    {Format::Tiff, "tif", "TIFF", "*.tif;*.tiff", "tiff", 32, true},
    {Format::Png, "png", "PNG", "*.png", "png", 32, true},
    {Format::Ico, "ico", "ICO", "*.ico", "ico", 32, true},
    {Format::Webp, "webp", "WEBP", "*.webp", "webp", 32, true},
    {Format::Heif, "heic", "HEIF", "*.heic;*.heif;*.hif", "", 32, true},
    {Format::Avif, "avif", "AVIF", "*.avif", "", 32, true},
    {Format::Paint, "paint", "msepaint project", "*.paint", "", 32, true},
};

const FormatRow *row(Format f)
{
    for (const FormatRow &r : kRows)
        if (r.f == f) return &r;
    return nullptr;
}

void setError(QString *error, const QString &msg) { if (error) *error = msg; }

bool qtCanWrite(const char *name) { return QImageWriter::supportedImageFormats().contains(QByteArray(name)); }
bool qtCanRead(const char *name) { return QImageReader::supportedImageFormats().contains(QByteArray(name)); }

QImage withDpi(QImage img, double dpiX, double dpiY)
{
    img.setDotsPerMeterX(int(std::lround(dpiX / kInchPerMeter)));
    img.setDotsPerMeterY(int(std::lround(dpiY / kInchPerMeter)));
    return img;
}

// ---- Exif mínimo con la resolución (etiquetas 282/283/296) ----

QByteArray makeExifResolution(double dpiX, double dpiY)
{
    // TIFF little-endian: cabecera (8) + IFD de 3 entradas (2 + 36 + 4) + dos RATIONAL (16).
    QByteArray b(8 + 2 + 3 * 12 + 4 + 16, '\0');
    auto *p = reinterpret_cast<uchar *>(b.data());
    p[0] = 'I'; p[1] = 'I';
    qToLittleEndian<quint16>(42, p + 2);
    qToLittleEndian<quint32>(8, p + 4);
    qToLittleEndian<quint16>(3, p + 8);
    const quint32 ratX = 8 + 2 + 36 + 4, ratY = ratX + 8;
    auto entry = [&](int i, quint16 tag, quint16 type, quint32 value) {
        uchar *e = p + 10 + i * 12;
        qToLittleEndian<quint16>(tag, e);
        qToLittleEndian<quint16>(type, e + 2);
        qToLittleEndian<quint32>(1, e + 4);
        if (type == 3) qToLittleEndian<quint16>(quint16(value), e + 8);
        else qToLittleEndian<quint32>(value, e + 8);
    };
    entry(0, 282, 5, ratX);
    entry(1, 283, 5, ratY);
    entry(2, 296, 3, 2); // pulgadas
    qToLittleEndian<quint32>(quint32(std::lround(dpiX * 10000)), p + ratX);
    qToLittleEndian<quint32>(10000, p + ratX + 4);
    qToLittleEndian<quint32>(quint32(std::lround(dpiY * 10000)), p + ratY);
    qToLittleEndian<quint32>(10000, p + ratY + 4);
    return b;
}

// Lee 282/283 del IFD0. Devuelve falso si falta alguna o no es positiva.
bool parseExifResolution(const QByteArray &raw, double &dpiX, double &dpiY)
{
    int start = raw.indexOf(QByteArrayLiteral("II*\0"));
    const int mm = raw.indexOf(QByteArrayLiteral("MM\0*"));
    if (start < 0 || (mm >= 0 && mm < start)) start = mm;
    if (start < 0) return false;
    const QByteArray t = raw.mid(start);
    const bool le = t.startsWith("II");
    const auto *d = reinterpret_cast<const uchar *>(t.constData());
    const qsizetype n = t.size();
    auto u16 = [&](qsizetype o) -> quint32 { return o + 2 <= n ? (le ? qFromLittleEndian<quint16>(d + o) : qFromBigEndian<quint16>(d + o)) : 0; };
    auto u32 = [&](qsizetype o) -> quint32 { return o + 4 <= n ? (le ? qFromLittleEndian<quint32>(d + o) : qFromBigEndian<quint32>(d + o)) : 0; };
    const quint32 ifd = u32(4);
    const quint32 count = u16(ifd);
    double x = 0, y = 0;
    for (quint32 i = 0; i < count; ++i) {
        const qsizetype e = ifd + 2 + i * 12;
        const quint32 tag = u16(e), type = u16(e + 2);
        if ((tag != 282 && tag != 283) || type != 5) continue;
        const quint32 off = u32(e + 8);
        const quint32 num = u32(off), den = u32(off + 4);
        const double v = den ? double(num) / den : 0.0;
        (tag == 282 ? x : y) = v;
    }
    if (x <= 0 || y <= 0) return false;
    dpiX = x;
    dpiY = y;
    return true;
}

// ---- XMP propio del .paint de msepaint ----

QByteArray makeXmp(const QList<quint32> &layerIds, const QColor &bg, bool bgVisible)
{
    QStringList ids;
    for (quint32 id : layerIds) ids << QString::number(id);
    const quint32 colorref = quint32(bg.red()) | quint32(bg.green()) << 8 | quint32(bg.blue()) << 16;
    return QStringLiteral(
               "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\"><rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
               "<rdf:Description rdf:about=\"\" xmlns:msepaint=\"https://msepaint.local/ns/1.0/\" msepaint:Layers=\"%1\" "
               "msepaint:BackgroundColor=\"%2\" msepaint:BackgroundVisible=\"%3\"/></rdf:RDF></x:xmpmeta>")
        .arg(ids.join(QLatin1Char(' ')))
        .arg(colorref)
        .arg(bgVisible ? 1 : 0)
        .toUtf8();
}

QString xmpAttr(const QString &xmp, const char *name)
{
    const QRegularExpression re(QStringLiteral("msepaint:%1=\"([^\"]*)\"").arg(QLatin1String(name)));
    const auto m = re.match(xmp);
    return m.hasMatch() ? m.captured(1) : QString();
}

int depthOf(const QImage &img, Format f)
{
    switch (img.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB: return 1;
    case QImage::Format_Indexed8: return img.colorCount() <= 2 ? 1 : img.colorCount() <= 16 && f != Format::Gif ? 4 : 8;
    case QImage::Format_Grayscale8: return 8;
    default: return img.hasAlphaChannel() ? 32 : 24;
    }
}

bool isHeifSignature(const QByteArray &head)
{
    if (head.size() < 12 || head.mid(4, 4) != "ftyp") return false;
    static const char *brands[] = {"heic", "heix", "hevc", "hevx", "heim", "heis", "mif1", "msf1", "mif2", "avif", "avis"};
    const QByteArray major = head.mid(8, 4);
    for (const char *b : brands)
        if (major == b) return true;
    return false;
}

#ifdef MSEPAINT_HAVE_LIBHEIF

struct HeifInit {
    HeifInit() { heif_init(nullptr); }
    ~HeifInit() { heif_deinit(); }
};
void ensureHeif() { static HeifInit init; }

struct CtxGuard { heif_context *c = heif_context_alloc(); ~CtxGuard() { heif_context_free(c); } };
struct HandleGuard { heif_image_handle *h = nullptr; ~HandleGuard() { if (h) heif_image_handle_release(h); } };
struct ImageGuard { heif_image *i = nullptr; ~ImageGuard() { if (i) heif_image_release(i); } };

bool heifFailed(const heif_error &e, QString *error)
{
    if (e.code == heif_error_Ok) return false;
    setError(error, QString::fromUtf8(e.message));
    return true;
}

QImage decodeHandle(heif_image_handle *h, QString *error)
{
    ImageGuard img;
    if (heifFailed(heif_decode_image(h, &img.i, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr), error))
        return {};
    int stride = 0;
    const uint8_t *src = heif_image_get_plane_readonly(img.i, heif_channel_interleaved, &stride);
    const int w = heif_image_get_width(img.i, heif_channel_interleaved);
    const int h2 = heif_image_get_height(img.i, heif_channel_interleaved);
    if (!src || w <= 0 || h2 <= 0) { setError(error, QStringLiteral("empty image")); return {}; }
    QImage out(w, h2, QImage::Format_RGBA8888);
    for (int y = 0; y < h2; ++y) memcpy(out.scanLine(y), src + qsizetype(y) * stride, size_t(w) * 4);
    return out.convertToFormat(Document::kFormat);
}

QByteArray metadataOf(heif_image_handle *h, const char *type, const char *contentType)
{
    heif_item_id ids[16];
    const int n = heif_image_handle_get_list_of_metadata_block_IDs(h, type, ids, 16);
    for (int i = 0; i < n; ++i) {
        if (contentType && QByteArray(heif_image_handle_get_metadata_content_type(h, ids[i])) != contentType) continue;
        QByteArray b(qsizetype(heif_image_handle_get_metadata_size(h, ids[i])), '\0');
        if (heif_image_handle_get_metadata(h, ids[i], b.data()).code == heif_error_Ok) return b;
    }
    return {};
}

bool loadHeif(const QByteArray &data, bool project, ImageData &out, QString *error)
{
    ensureHeif();
    CtxGuard ctx;
    if (heifFailed(heif_context_read_from_memory_without_copy(ctx.c, data.constData(), size_t(data.size()), nullptr), error))
        return false;
    HandleGuard prim;
    if (heifFailed(heif_context_get_primary_image_handle(ctx.c, &prim.h), error)) return false;

    out = ImageData();
    if (!parseExifResolution(metadataOf(prim.h, "Exif", nullptr), out.dpiX, out.dpiY)) out.dpiX = out.dpiY = 96.0;

    const QString xmp = QString::fromUtf8(metadataOf(prim.h, "mime", "application/rdf+xml"));
    const QStringList ids = xmpAttr(xmp, "Layers").split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (!ids.isEmpty()) {
        for (const QString &s : ids) {
            HandleGuard h;
            if (heifFailed(heif_context_get_image_handle(ctx.c, heif_item_id(s.toUInt()), &h.h), error)) return false;
            QImage img = decodeHandle(h.h, error);
            if (img.isNull()) return false;
            out.layers.push_back(Layer{QString(), img, true, 1.0});
        }
        const quint32 cr = xmpAttr(xmp, "BackgroundColor").toUInt();
        out.background = QColor(int(cr & 0xFF), int(cr >> 8 & 0xFF), int(cr >> 16 & 0xFF));
        out.backgroundVisible = xmpAttr(xmp, "BackgroundVisible") != QLatin1String("0");
    } else {
        QImage img = decodeHandle(prim.h, error);
        if (img.isNull()) return false;
        out.layers.push_back(Layer{QString(), img, true, 1.0});
        if (project) { out.background = Qt::black; out.backgroundVisible = false; } //.4 paso 5
    }
    // Todas las capas del mismo tamaño que la primera.
    const QSize sz = out.layers.front().image.size();
    for (Layer &l : out.layers)
        if (l.image.size() != sz) l.image = l.image.copy(QRect(QPoint(0, 0), sz));
    out.bitsPerPixel = 32;
    return true;
}

heif_error writeToArray(heif_context *, const void *data, size_t size, void *userdata)
{
    static_cast<QByteArray *>(userdata)->append(static_cast<const char *>(data), qsizetype(size));
    return heif_error{heif_error_Ok, heif_suberror_Unspecified, "Success"};
}

bool encodeRgba(heif_context *ctx, heif_encoder *enc, const QImage &src, heif_image_handle **handle, QString *error)
{
    const QImage rgba = src.convertToFormat(QImage::Format_RGBA8888); // alfa no premultiplicado
    ImageGuard img;
    if (heifFailed(heif_image_create(rgba.width(), rgba.height(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA, &img.i), error))
        return false;
    if (heifFailed(heif_image_add_plane(img.i, heif_channel_interleaved, rgba.width(), rgba.height(), 8), error)) return false;
    int stride = 0;
    uint8_t *dst = heif_image_get_plane(img.i, heif_channel_interleaved, &stride);
    for (int y = 0; y < rgba.height(); ++y) memcpy(dst + qsizetype(y) * stride, rgba.constScanLine(y), size_t(rgba.width()) * 4);
    return !heifFailed(heif_context_encode_image(ctx, img.i, enc, nullptr, handle), error);
}

// Escritura provisional del .paint. libheif 1.17 no puede escribir el ítem `iovl` ni la compresión
// Deflate de `unci`: cada capa va como imagen `unci` sin pérdida y sin comprimir, el primario es la composición plana
// (lo que verá cualquier otro visor) y la lista de capas y el fondo van en XMP sobre el primario.
bool saveHeifProject(const ImageData &data, QByteArray &bytes, QString *error)
{
    ensureHeif();
    CtxGuard ctx;
    heif_encoder *enc = nullptr;
    if (heifFailed(heif_context_get_encoder_for_format(ctx.c, heif_compression_uncompressed, &enc), error)) return false;
    heif_encoder_set_lossless(enc, 1);
    QList<quint32> ids;
    bool ok = true;
    for (const Layer &l : data.layers) {
        HandleGuard h;
        if (!(ok = encodeRgba(ctx.c, enc, l.image, &h.h, error))) break;
        ids << heif_image_handle_get_item_id(h.h);
    }
    HandleGuard prim;
    if (ok) ok = encodeRgba(ctx.c, enc, flattened(data, true), &prim.h, error);
    heif_encoder_release(enc);
    if (!ok) return false;
    if (heifFailed(heif_context_set_primary_image(ctx.c, prim.h), error)) return false;
    const QByteArray exif = makeExifResolution(data.dpiX, data.dpiY);
    if (heifFailed(heif_context_add_exif_metadata(ctx.c, prim.h, exif.constData(), int(exif.size())), error)) return false;
    const QByteArray xmp = makeXmp(ids, data.background, data.backgroundVisible);
    if (heifFailed(heif_context_add_XMP_metadata(ctx.c, prim.h, xmp.constData(), int(xmp.size())), error)) return false;
    heif_writer writer{1, &writeToArray};
    return !heifFailed(heif_context_write(ctx.c, &writer, &bytes), error);
}

bool saveHeifImage(const QImage &flat, double dpiX, double dpiY, QByteArray &bytes, QString *error)
{
    ensureHeif();
    CtxGuard ctx;
    heif_encoder *enc = nullptr;
    if (heifFailed(heif_context_get_encoder_for_format(ctx.c, heif_compression_HEVC, &enc), error)) return false;
    heif_encoder_set_lossy_quality(enc, 90);
    HandleGuard h;
    const bool ok = encodeRgba(ctx.c, enc, flat, &h.h, error);
    heif_encoder_release(enc);
    if (!ok) return false;
    const QByteArray exif = makeExifResolution(dpiX, dpiY);
    heif_context_add_exif_metadata(ctx.c, h.h, exif.constData(), int(exif.size()));
    heif_writer writer{1, &writeToArray};
    return !heifFailed(heif_context_write(ctx.c, &writer, &bytes), error);
}

#endif // MSEPAINT_HAVE_LIBHEIF

// Paleta VGA de 16 colores.
const QList<QRgb> &vga16()
{
    static const QList<QRgb> p{qRgb(0, 0, 0),       qRgb(128, 0, 0),   qRgb(0, 128, 0),   qRgb(128, 128, 0),
                               qRgb(0, 0, 128),     qRgb(128, 0, 128), qRgb(0, 128, 128), qRgb(192, 192, 192),
                               qRgb(128, 128, 128), qRgb(255, 0, 0),   qRgb(0, 255, 0),   qRgb(255, 255, 0),
                               qRgb(0, 0, 255),     qRgb(255, 0, 255), qRgb(0, 255, 255), qRgb(255, 255, 255)};
    return p;
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------

QString formatId(Format f) { const FormatRow *r = row(f); return r ? QString::fromLatin1(r->id) : QString(); }

Format formatFromId(const QString &id)
{
    for (const FormatRow &r : kRows)
        if (id == QLatin1String(r.id)) return r.f;
    return Format::Unknown;
}

Format formatFromSuffix(const QString &suffix)
{
    const QString s = suffix.toLower().remove(QLatin1Char('.'));
    if (s.isEmpty()) return Format::Unknown;
    if (s == QLatin1String("bmp") || s == QLatin1String("dib") || s == QLatin1String("rle")) return Format::Bmp24;
    const QString pat = QStringLiteral("*.") + s;
    for (const FormatRow &r : kRows)
        if (QString::fromLatin1(r.patterns).split(QLatin1Char(';')).contains(pat)) return r.f;
    return Format::Unknown;
}

QString defaultSuffix(Format f)
{
    switch (f) {
    case Format::Bmp1: case Format::Bmp4: case Format::Bmp8: case Format::Bmp24: return QStringLiteral("bmp");
    case Format::Jpeg: return QStringLiteral("jpg");
    case Format::Tiff: return QStringLiteral("tif");
    default: return formatId(f);
    }
}

QString formatLabel(Format f) { const FormatRow *r = row(f); return r ? QString::fromLatin1(r->label) : QString(); }
QStringList formatPatterns(Format f) { const FormatRow *r = row(f); return r ? QString::fromLatin1(r->patterns).split(QLatin1Char(';')) : QStringList(); }
int bitsPerPixel(Format f) { const FormatRow *r = row(f); return r ? r->bpp : 32; }
bool supportsAlpha(Format f) { const FormatRow *r = row(f); return r && r->alpha; }

bool hasProjectSupport()
{
#ifdef MSEPAINT_HAVE_LIBHEIF
    return true;
#else
    return false;
#endif
}

bool canRead(Format f)
{
    switch (f) {
    case Format::Unknown: return false;
    case Format::Bmp1: case Format::Bmp4: case Format::Bmp8: case Format::Bmp24: return qtCanRead("bmp");
    case Format::Gif: return qtCanRead("gif");
    case Format::Heif: case Format::Avif: case Format::Paint: return hasProjectSupport();
    default: return qtCanRead(row(f)->qtName);
    }
}

bool canWrite(Format f)
{
    switch (f) {
    case Format::Unknown: case Format::Avif: return false;
    case Format::Bmp1: case Format::Bmp4: case Format::Bmp8: case Format::Bmp24: case Format::Gif: return true;
    case Format::Paint: return hasProjectSupport();
    case Format::Heif:
#ifdef MSEPAINT_HAVE_LIBHEIF
        ensureHeif();
        return heif_have_encoder_for_format(heif_compression_HEVC) != 0;
#else
        return false;
#endif
    default: return qtCanWrite(row(f)->qtName);
    }
}

QList<Format> writableFormats()
{
    QList<Format> out;
    for (const FormatRow &r : kRows)
        if (canWrite(r.f)) out << r.f;
    return out;
}

QStringList openSuffixes()
{
    // Lista fija de extensiones, filtrada por lo que se puede leer.
    static const char *all[] = {"avif", "bmp", "dib", "gif", "heic", "heif", "hif", "ico", "jfif", "jpe", "jpeg", "jpg",
                                "paint", "png", "rle", "tif", "tiff", "webp"};
    QStringList out;
    for (const char *s : all)
        if (canRead(formatFromSuffix(QString::fromLatin1(s)))) out << QString::fromLatin1(s);
    return out;
}

QImage flattened(const ImageData &data, bool keepAlpha)
{
    const QSize size = data.size();
    QImage out(size, Document::kFormat);
    out.fill(data.backgroundVisible ? data.background : QColor(Qt::transparent));
    {
        QPainter p(&out);
        for (const Layer &l : data.layers) {
            if (!l.visible || l.opacity <= 0.0) continue;
            p.setOpacity(l.opacity);
            p.drawImage(0, 0, l.image);
        }
    }
    if (!keepAlpha) {
        QImage opaque(size, QImage::Format_RGB32);
        opaque.fill(Qt::white);
        QPainter p(&opaque);
        p.drawImage(0, 0, out);
        p.end();
        out = opaque;
    }
    return withDpi(out, data.dpiX, data.dpiY);
}

ImageData fromDocument(const Document &doc, double dpiX, double dpiY)
{
    ImageData d;
    d.layers = doc.layers();
    d.background = doc.background();
    d.backgroundVisible = doc.backgroundVisible();
    d.dpiX = dpiX;
    d.dpiY = dpiY;
    return d;
}

bool load(const QString &path, ImageData &out, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { setError(error, file.errorString()); return false; }
    const QString suffix = QFileInfo(path).suffix().toLower();
    const QByteArray head = file.peek(16);
    if (suffix == QLatin1String("paint") || isHeifSignature(head)) {
#ifdef MSEPAINT_HAVE_LIBHEIF
        const QByteArray data = file.readAll();
        if (!loadHeif(data, suffix == QLatin1String("paint"), out, error)) return false;
        const QByteArray major = head.mid(8, 4);
        out.format = suffix == QLatin1String("paint") ? Format::Paint
                   : (major == "avif" || major == "avis") ? Format::Avif : Format::Heif;
        return true;
#else
        setError(error, QStringLiteral("HEIF support (libheif) is not available"));
        return false;
#endif
    }
    QImageReader reader(&file);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) { setError(error, reader.errorString()); return false; }
    out = ImageData();
    Format f = formatFromSuffix(suffix);
    if (f == Format::Unknown) f = formatFromSuffix(QString::fromLatin1(reader.format()));
    out.bitsPerPixel = depthOf(img, f);
    if (head.startsWith("BM") && head.size() >= 16) {
        // BMP: profundidad real de la cabecera (Qt amplía o reduce la paleta al leer).
        file.seek(28);
        const QByteArray bc = file.read(2);
        if (bc.size() == 2) out.bitsPerPixel = std::min(32, int(qFromLittleEndian<quint16>(bc.constData())));
    }
    if (f == Format::Bmp24) {
        switch (out.bitsPerPixel) {
        case 1: f = Format::Bmp1; break;
        case 4: f = Format::Bmp4; break;
        case 8: f = Format::Bmp8; break;
        default: break;
        }
    }
    out.format = f;
    if (img.dotsPerMeterX() > 0 && img.dotsPerMeterY() > 0) {
        out.dpiX = img.dotsPerMeterX() * kInchPerMeter;
        out.dpiY = img.dotsPerMeterY() * kInchPerMeter;
    }
    out.layers.push_back(Layer{QStringLiteral("Layer 1"), img.convertToFormat(Document::kFormat), true, 1.0});
    return true;
}

QByteArray encodeBmp(const QImage &flat, int bpp, double dpiX, double dpiY)
{
    const QImage rgb = flat.convertToFormat(QImage::Format_RGB32);
    QImage idx;
    QList<QRgb> palette;
    if (bpp == 1) {
        // Umbral a blanco y negro.
        idx = rgb.convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::ThresholdDither | Qt::AvoidDither);
        palette = idx.colorTable();
    } else if (bpp == 4) {
        idx = rgb.convertToFormat(QImage::Format_Indexed8, vga16(), Qt::DiffuseDither);
        palette = vga16();
    } else if (bpp == 8) {
        // Paleta exacta si caben los colores; si no, cubo de colores con difuminado (paleta de semitono).
        idx = rgb.convertToFormat(QImage::Format_Indexed8, Qt::DiffuseDither);
        palette = idx.colorTable();
    } else {
        bpp = 24;
    }
    const int w = rgb.width(), h = rgb.height();
    const int rowBytes = ((w * bpp + 31) / 32) * 4;
    const int paletteBytes = bpp <= 8 ? int(palette.size()) * 4 : 0;
    const quint32 offBits = 14 + 40 + paletteBytes;
    const quint32 imageBytes = quint32(rowBytes) * quint32(h);
    QByteArray b(qsizetype(offBits + imageBytes), '\0');
    auto *p = reinterpret_cast<uchar *>(b.data());
    p[0] = 'B'; p[1] = 'M';
    qToLittleEndian<quint32>(offBits + imageBytes, p + 2);
    qToLittleEndian<quint32>(offBits, p + 10);
    uchar *ih = p + 14;
    qToLittleEndian<quint32>(40, ih);
    qToLittleEndian<qint32>(w, ih + 4);
    qToLittleEndian<qint32>(h, ih + 8);
    qToLittleEndian<quint16>(1, ih + 12);
    qToLittleEndian<quint16>(quint16(bpp), ih + 14);
    qToLittleEndian<quint32>(0, ih + 16);
    qToLittleEndian<quint32>(imageBytes, ih + 20);
    qToLittleEndian<qint32>(qint32(std::lround(dpiX / kInchPerMeter)), ih + 24);
    qToLittleEndian<qint32>(qint32(std::lround(dpiY / kInchPerMeter)), ih + 28);
    qToLittleEndian<quint32>(bpp <= 8 ? quint32(palette.size()) : 0, ih + 32);
    uchar *pal = ih + 40;
    for (QRgb c : (bpp <= 8 ? palette : QList<QRgb>())) {
        *pal++ = uchar(qBlue(c)); *pal++ = uchar(qGreen(c)); *pal++ = uchar(qRed(c)); *pal++ = 0;
    }
    uchar *bits = p + offBits;
    for (int y = 0; y < h; ++y) {
        uchar *dst = bits + qsizetype(h - 1 - y) * rowBytes; // de abajo arriba
        if (bpp == 24) {
            const auto *src = reinterpret_cast<const QRgb *>(rgb.constScanLine(y));
            for (int x = 0; x < w; ++x) { dst[3 * x] = uchar(qBlue(src[x])); dst[3 * x + 1] = uchar(qGreen(src[x])); dst[3 * x + 2] = uchar(qRed(src[x])); }
        } else if (bpp == 8) {
            memcpy(dst, idx.constScanLine(y), size_t(w));
        } else if (bpp == 4) {
            const uchar *src = idx.constScanLine(y);
            for (int x = 0; x < w; ++x) dst[x / 2] |= (x % 2 == 0) ? uchar(src[x] << 4) : uchar(src[x] & 0x0F);
        } else {
            for (int x = 0; x < w; ++x)
                if (idx.pixelIndex(x, y)) dst[x / 8] |= uchar(0x80 >> (x % 8));
        }
    }
    return b;
}

QByteArray encodeGif(const QImage &flat)
{
    const QImage idx = flat.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, Qt::DiffuseDither);
    QList<QRgb> palette = idx.colorTable();
    palette.resize(256, qRgb(0, 0, 0));
    const int w = idx.width(), h = idx.height();

    QByteArray out("GIF89a");
    auto u16 = [&out](int v) { out.append(char(v & 0xFF)); out.append(char((v >> 8) & 0xFF)); };
    u16(w); u16(h);
    out.append(char(0xF7)); // tabla global de 256 colores, 8 bits
    out.append('\0'); out.append('\0');
    for (QRgb c : palette) { out.append(char(qRed(c))); out.append(char(qGreen(c))); out.append(char(qBlue(c))); }
    out.append(char(0x2C)); u16(0); u16(0); u16(w); u16(h); out.append('\0');
    out.append(char(8)); // tamaño mínimo del código LZW

    // LZW de GIF con códigos de 9 a 12 bits.
    QByteArray data;
    quint32 acc = 0;
    int nbits = 0, codeSize = 9, next = 258;
    const int clearCode = 256, eoi = 257;
    auto emitCode = [&](int code) {
        acc |= quint32(code) << nbits;
        nbits += codeSize;
        while (nbits >= 8) { data.append(char(acc & 0xFF)); acc >>= 8; nbits -= 8; }
    };
    std::unordered_map<quint32, int> dict;
    dict.reserve(4096);
    emitCode(clearCode);
    const qsizetype n = qsizetype(w) * h;
    if (n > 0) {
        int prefix = idx.constScanLine(0)[0];
        for (qsizetype i = 1; i < n; ++i) {
            const int c = idx.constScanLine(int(i / w))[i % w];
            const quint32 key = quint32(prefix) << 8 | quint32(c);
            auto it = dict.find(key);
            if (it != dict.end()) { prefix = it->second; continue; }
            emitCode(prefix);
            if (next < 4096) {
                if (next == (1 << codeSize) && codeSize < 12) ++codeSize;
                dict.emplace(key, next++);
            } else {
                emitCode(clearCode);
                dict.clear();
                next = 258;
                codeSize = 9;
            }
            prefix = c;
        }
        emitCode(prefix);
    }
    emitCode(eoi);
    if (nbits > 0) data.append(char(acc & 0xFF));
    for (qsizetype i = 0; i < data.size(); i += 255) {
        const qsizetype len = std::min<qsizetype>(255, data.size() - i);
        out.append(char(len));
        out.append(data.constData() + i, len);
    }
    out.append('\0');
    out.append(char(0x3B));
    return out;
}

bool save(const QString &path, Format f, const ImageData &data, QString *error)
{
    if (data.layers.empty()) { setError(error, QStringLiteral("no layers")); return false; } // WINCODEC_ERR_FRAMEMISSING
    if (!canWrite(f)) { setError(error, QStringLiteral("format not supported")); return false; }
    QByteArray bytes;
    switch (f) {
    case Format::Bmp1: case Format::Bmp4: case Format::Bmp8: case Format::Bmp24:
        bytes = encodeBmp(flattened(data, false), bitsPerPixel(f), data.dpiX, data.dpiY);
        break;
    case Format::Gif:
        bytes = encodeGif(flattened(data, false));
        break;
#ifdef MSEPAINT_HAVE_LIBHEIF
    case Format::Paint:
        if (!saveHeifProject(data, bytes, error)) return false;
        break;
    case Format::Heif:
        if (!saveHeifImage(flattened(data, true), data.dpiX, data.dpiY, bytes, error)) return false;
        break;
#endif
    default: {
        QImage img = flattened(data, supportsAlpha(f));
        if (f == Format::Ico && (img.width() > 256 || img.height() > 256))
            img = img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (supportsAlpha(f)) img = img.convertToFormat(QImage::Format_ARGB32);
        QBuffer buf(&bytes);
        buf.open(QIODevice::WriteOnly);
        QImageWriter w(&buf, row(f)->qtName);
        if (f == Format::Jpeg) w.setQuality(95); // EncoderQuality = 95
        if (f == Format::Webp) w.setQuality(100);
        if (!w.write(img)) { setError(error, w.errorString()); return false; }
        break;
    }
    }
    // Escritura atómica: temporal y reemplazo.
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(bytes) != bytes.size() || !out.commit()) {
        setError(error, out.errorString());
        return false;
    }
    return true;
}

} // namespace mp::files
