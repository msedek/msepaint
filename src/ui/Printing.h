#pragma once

#include <QImage>
#include <QPageLayout>
#include <QSizeF>

class QPrinter;

// Configuración de página de la sesión. Solo vive en memoria.
struct PageSettings {
    QPageLayout layout;      // papel, orientación y márgenes (12,7 mm por defecto)
    bool centerH = false;
    bool centerV = false;
    bool fit = true;         // «Fit to» a × b páginas; si no, «Adjust to» porcentaje
    int percent = 100;
    int fitWide = 1;
    int fitTall = 1;

    PageSettings();
};

namespace printing {

// Reparto de la imagen en páginas: tamaño final y desplazamiento dentro del área total (en unidades del área útil).
struct Placement {
    QSizeF size;
    QPointF offset;
    int pagesX = 1;
    int pagesY = 1;
};
Placement place(QSizeF printable, QSizeF physical, const PageSettings &s);

// Dibuja la imagen en `printer` (una o varias páginas). `image` ya va aplanada.
bool render(QPrinter *printer, const QImage &image, double dpiX, double dpiY, const PageSettings &s);

// Diálogo «Page setup» con papel, orientación, márgenes, centrado y escala. Devuelve true si se aceptó.
bool pageSetupDialog(PageSettings &s, QSize imagePx, double dpi);

} // namespace printing
