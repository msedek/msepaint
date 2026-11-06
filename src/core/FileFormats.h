#pragma once

#include "core/Document.h"

#include <QColor>
#include <QImage>
#include <QList>
#include <QString>
#include <QStringList>
#include <vector>

// Abrir y guardar imágenes y el proyecto .paint.
namespace mp::files {

enum class Format {
    Unknown,
    Png,
    Jpeg,
    Bmp1,   // Monochrome Bitmap
    Bmp4,   // 16 Color Bitmap
    Bmp8,   // 256 Color Bitmap
    Bmp24,  // 24-bit Bitmap
    Gif,
    Tiff,
    Ico,
    Webp,
    Heif,   // .heic/.heif (solo lectura salvo que libheif tenga codificador HEVC)
    Avif,   // solo lectura
    Paint,  // proyecto con capas
};

// Contenido de un archivo: capas (índice 0 = abajo), fondo y DPI. Pila de capas.
struct ImageData {
    std::vector<Layer> layers;
    QColor background = Qt::white;
    bool backgroundVisible = true;
    double dpiX = 96.0;
    double dpiY = 96.0;
    int bitsPerPixel = 32; // profundidad del archivo abierto (para el aviso de calidad de color)
    Format format = Format::Unknown;

    QSize size() const { return layers.empty() ? QSize() : layers.front().image.size(); }
};

// Identificadores estables (los usa QML): png, jpg, bmp1, bmp4, bmp8, bmp, gif, tif, ico, webp, heic, avif, paint.
QString formatId(Format f);
Format formatFromId(const QString &id);
// Formato según la extensión (".bmp" → 24 bits). Unknown si no se reconoce.
Format formatFromSuffix(const QString &suffix);
QString defaultSuffix(Format f);
QString formatLabel(Format f);           // texto del filtro sin patrones ("24-bit Bitmap")
QStringList formatPatterns(Format f);    // {"*.bmp", "*.dib"}
int bitsPerPixel(Format f);
bool supportsAlpha(Format f);
inline bool isProject(Format f) { return f == Format::Paint; }

bool hasProjectSupport();                // compilado con libheif
bool canRead(Format f);
bool canWrite(Format f);
// Formatos del diálogo de guardar, BMP 1/4/8/24, códecs, proyecto.
QList<Format> writableFormats();
// Extensiones que se aceptan al abrir o soltar (sin punto, minúsculas).
QStringList openSuffixes();

// Composición plana para guardar: capas visibles sobre el fondo si está visible. En formatos sin alfa la
// transparencia que queda se compone sobre blanco.
QImage flattened(const ImageData &data, bool keepAlpha);

ImageData fromDocument(const Document &doc, double dpiX = 96.0, double dpiY = 96.0);

bool load(const QString &path, ImageData &out, QString *error = nullptr);
bool save(const QString &path, Format f, const ImageData &data, QString *error = nullptr);

// Expuestos para pruebas.
QByteArray encodeBmp(const QImage &flat, int bpp, double dpiX, double dpiY);
QByteArray encodeGif(const QImage &flat);

} // namespace mp::files
