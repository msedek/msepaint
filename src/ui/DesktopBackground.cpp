#include "ui/DesktopBackground.h"

#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#include <QDir>
#include <QSettings>
#include <windows.h>
#endif

namespace desktop {

namespace {

bool run(const QString &program, const QStringList &args, QString *error, QString *output = nullptr)
{
    QProcess p;
    p.start(program, args);
    if (!p.waitForFinished(15000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(p.readAllStandardError()).trimmed();
        return false;
    }
    if (output) *output = QString::fromLocal8Bit(p.readAllStandardOutput());
    return true;
}

bool has(const char *program) { return !QStandardPaths::findExecutable(QString::fromLatin1(program)).isEmpty(); }

} // namespace

Kind detect()
{
#ifdef Q_OS_WIN
    return Kind::Windows;
#endif
    const QStringList names = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower().split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &n : names) {
        if (n == QLatin1String("kde") && has("plasma-apply-wallpaperimage")) return Kind::Kde;
        if (n == QLatin1String("xfce") && has("xfconf-query")) return Kind::Xfce;
        if ((n == QLatin1String("cinnamon") || n == QLatin1String("x-cinnamon")) && has("gsettings")) return Kind::Cinnamon;
        if ((n == QLatin1String("gnome") || n == QLatin1String("unity") || n == QLatin1String("budgie")
             || n == QLatin1String("pop") || n == QLatin1String("ubuntu")) && has("gsettings"))
            return Kind::Gnome;
    }
    return Kind::None;
}

bool setWallpaper(Kind kind, const QString &path, const QString &mode, QString *error)
{
    const QString uri = QUrl::fromLocalFile(path).toString();
#ifdef Q_OS_WIN
    if (kind == Kind::Windows) {
        // Estilo: Llenar = 10, Mosaico = 0 + TileWallpaper 1, Centrar = 0.
        QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Control Panel\\Desktop"), QSettings::NativeFormat);
        reg.setValue(QStringLiteral("WallpaperStyle"), mode == QLatin1String("fill") ? QStringLiteral("10") : QStringLiteral("0"));
        reg.setValue(QStringLiteral("TileWallpaper"), mode == QLatin1String("tile") ? QStringLiteral("1") : QStringLiteral("0"));
        reg.sync();
        const std::wstring w = QDir::toNativeSeparators(path).toStdWString();
        if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, const_cast<wchar_t *>(w.c_str()),
                                   SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            if (error) *error = QStringLiteral("SystemParametersInfo failed");
            return false;
        }
        return true;
    }
#endif
    switch (kind) {
    case Kind::Gnome:
    case Kind::Cinnamon: {
        const QString schema = kind == Kind::Gnome ? QStringLiteral("org.gnome.desktop.background")
                                                   : QStringLiteral("org.cinnamon.desktop.background");
        const QString option = mode == QLatin1String("tile") ? QStringLiteral("wallpaper")
                             : mode == QLatin1String("center") ? QStringLiteral("centered") : QStringLiteral("zoom");
        if (!run(QStringLiteral("gsettings"), {QStringLiteral("set"), schema, QStringLiteral("picture-options"), option}, error))
            return false;
        if (!run(QStringLiteral("gsettings"), {QStringLiteral("set"), schema, QStringLiteral("picture-uri"), uri}, error))
            return false;
        if (kind == Kind::Gnome) // la clave existe desde GNOME 42; si falta no es un error
            run(QStringLiteral("gsettings"), {QStringLiteral("set"), schema, QStringLiteral("picture-uri-dark"), uri}, nullptr);
        return true;
    }
    case Kind::Kde: {
        const QString fill = mode == QLatin1String("tile") ? QStringLiteral("tile")
                           : mode == QLatin1String("center") ? QStringLiteral("pad") : QStringLiteral("preserveAspectCrop");
        if (run(QStringLiteral("plasma-apply-wallpaperimage"), {QStringLiteral("--fill-mode"), fill, path}, error)) return true;
        // Plasma 5 no tiene --fill-mode: al menos se fija la imagen.
        return run(QStringLiteral("plasma-apply-wallpaperimage"), {path}, error);
    }
    case Kind::Xfce: {
        const QString style = mode == QLatin1String("tile") ? QStringLiteral("2")
                            : mode == QLatin1String("center") ? QStringLiteral("1") : QStringLiteral("5");
        QString list;
        if (!run(QStringLiteral("xfconf-query"), {QStringLiteral("-c"), QStringLiteral("xfce4-desktop"), QStringLiteral("-l")}, error, &list))
            return false;
        bool any = false;
        for (const QString &prop : list.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            if (!prop.endsWith(QLatin1String("/last-image"))) continue;
            const QString base = prop.left(prop.size() - int(qstrlen("/last-image")));
            any |= run(QStringLiteral("xfconf-query"),
                       {QStringLiteral("-c"), QStringLiteral("xfce4-desktop"), QStringLiteral("-p"), prop, QStringLiteral("-s"), path}, error);
            run(QStringLiteral("xfconf-query"),
                {QStringLiteral("-c"), QStringLiteral("xfce4-desktop"), QStringLiteral("-p"), base + QStringLiteral("/image-style"),
                 QStringLiteral("--create"), QStringLiteral("-t"), QStringLiteral("int"), QStringLiteral("-s"), style},
                nullptr);
        }
        return any;
    }
    case Kind::Windows:
    case Kind::None:
        break;
    }
    if (error) *error = QStringLiteral("unsupported desktop");
    return false;
}

} // namespace desktop
