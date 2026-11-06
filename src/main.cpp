#include "ui/AppController.h"
#include "ui/ImageProviders.h"
#include "ui/Printing.h"       // --- Archivos e impresión ---
#include "core/FileFormats.h"

#include <QFileInfo>
#include <QIcon>
#include <QPrinter>

#include <QDirIterator>
#include <QFontDatabase>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char *argv[])
{
    // QApplication (no QGuiApplication): los diálogos de impresión de Qt necesitan widgets.
    // Dentro de un AppImage, libheif debe cargar sus propios plugins y no los del sistema (otra versión).
    if (const QByteArray appdir = qgetenv("APPDIR"); !appdir.isEmpty())
        qputenv("LIBHEIF_PLUGIN_PATH", appdir + "/usr/lib/libheif-plugins");
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("msedek"));
    app.setApplicationName(QStringLiteral("msepaint"));
    app.setApplicationDisplayName(QStringLiteral("msepaint"));
    app.setDesktopFileName(QStringLiteral("msepaint")); // app_id en Wayland = lanzador msepaint.desktop
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("msepaint")));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Fuentes incluidas: Selawik (interfaz) y Fluent System Icons (íconos).
    QDirIterator fonts(QStringLiteral(":/qt/qml/MsePaint/assets/fonts"), {QStringLiteral("*.ttf")});
    while (fonts.hasNext()) QFontDatabase::addApplicationFont(fonts.next());

    // --- Archivos e impresión --- línea de comandos: [archivo], /p archivo, /pt archivo impresora [...].
    // El modificador empieza con «/» o «-» y no distingue mayúsculas; cualquier otra cosa es la ruta.
    QString fileArg, printerArg;
    bool printMode = false;
    const QStringList args = app.arguments().mid(1);
    for (qsizetype i = 0; i < args.size(); ++i) {
        const QString a = args[i].toLower();
        if (a == QLatin1String("/p") || a == QLatin1String("-p")) { printMode = true; continue; }
        if (a == QLatin1String("/pt") || a == QLatin1String("-pt")) {
            printMode = true;
            if (i + 1 < args.size()) fileArg = args[++i];
            if (i + 1 < args.size()) printerArg = args[++i];
            i = args.size(); // controlador y puerto no se usan en Linux
            break;
        }
        if (fileArg.isEmpty()) fileArg = args[i];
    }
    if (printMode) {
        // Imprime en la impresora predeterminada (o la indicada) sin mostrar la ventana y termina.
        mp::files::ImageData data;
        if (fileArg.isEmpty() || !mp::files::load(fileArg, data)) return 1;
        QPrinter printer(QPrinter::HighResolution);
        if (!printerArg.isEmpty()) printer.setPrinterName(printerArg);
        const PageSettings page;
        printer.setPageLayout(page.layout);
        printer.setDocName(QFileInfo(fileArg).fileName());
        return printing::render(&printer, mp::files::flattened(data, true), data.dpiX, data.dpiY, page) ? 0 : 1;
    }

    AppController controller;
    if (!controller.devMode()) controller.loadSettings();
    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("swatch"), new SwatchProvider);
    engine.addImageProvider(QStringLiteral("layers"), new LayerThumbProvider(&controller));
    engine.addImageProvider(QStringLiteral("composite"), new CompositeProvider(&controller));
    engine.setInitialProperties({{QStringLiteral("app"), QVariant::fromValue(&controller)}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("MsePaint", "Main");

    if (!fileArg.isEmpty())
        controller.openFile(QUrl::fromLocalFile(QFileInfo(fileArg).absoluteFilePath()));

    // Desarrollo: MSEPAINT_DEMO=1 dibuja una muestra de todas las herramientas (src/ui/Demo.cpp).
    if (qEnvironmentVariableIsSet("MSEPAINT_DEMO")) {
        void runDemo(AppController &);
        controller.document()->reset(QSize(720, 420), Qt::white);
        runDemo(controller);
    }

    // Desarrollo: MSEPAINT_SCREENSHOT=ruta.png guarda la ventana y sale (sirve sin pantalla con QT_QPA_PLATFORM=offscreen).
    const QString shot = qEnvironmentVariable("MSEPAINT_SCREENSHOT");
    if (!shot.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        QTimer::singleShot(qEnvironmentVariableIntValue("MSEPAINT_SCREENSHOT_DELAY") ?: 800, window, [window, shot] {
            window->grabWindow().save(shot);
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
