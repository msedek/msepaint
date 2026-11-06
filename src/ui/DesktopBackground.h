#pragma once

#include <QString>

// «Set as desktop background» en Linux: GNOME/Cinnamon con gsettings, KDE Plasma con
// plasma-apply-wallpaperimage y XFCE con xfconf-query; en Windows, SystemParametersInfo. Sin ninguno, el submenú se oculta.
namespace desktop {

enum class Kind { None, Gnome, Cinnamon, Kde, Xfce, Windows };

Kind detect();
// mode: fill | tile | center
bool setWallpaper(Kind kind, const QString &path, const QString &mode, QString *error = nullptr);

} // namespace desktop
