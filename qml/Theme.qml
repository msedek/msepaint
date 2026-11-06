pragma Singleton
import QtQuick

// Colores y medidas de la interfaz (temas claro y oscuro).
QtObject {
    id: t
    // Tema elegido en Ajustes: "light", "dark" o "system" (Use system setting, por defecto).
    property string themeMode: "system"
    property bool dark: themeMode === "dark"
                        || (themeMode === "system" && Qt.styleHints.colorScheme === Qt.ColorScheme.Dark)

    // Fuentes
    readonly property string uiFont: "Selawik"
    readonly property string iconFont: "FluentSystemIcons-Regular"
    readonly property string iconFontFilled: "FluentSystemIcons-Filled"

    // Acento
    readonly property color accent: dark ? "#60CDFF" : "#005FB8"
    readonly property color accentText: dark ? "#99EBFF" : "#003E92"

    // Superficies
    readonly property color windowBg: dark ? "#202020" : "#F3F3F3"
    readonly property color layerFill: dark ? "#2B2B2B" : "#F9F9F9"
    readonly property color cardStrokeSolid: dark ? "#1C1C1C" : "#EBEBEB"
    readonly property color divider: dark ? "#15FFFFFF" : "#0F000000"
    readonly property color acrylic: dark ? "#2C2C2C" : "#F9F9F9"
    readonly property color solidSecondary: dark ? "#1C1C1C" : "#EEEEEE"
    readonly property color cardBg: dark ? "#0DFFFFFF" : "#B3FFFFFF"
    readonly property color cardStroke: dark ? "#19000000" : "#0F000000"
    readonly property color dialogBg: dark ? "#262626" : "#FFFFFF"
    readonly property color smoke: "#4D000000"
    readonly property color flyoutBg: dark ? "#2C2C2C" : "#F9F9F9"
    readonly property color flyoutStroke: dark ? "#33000000" : "#0F000000"

    // Lienzo y reglas
    readonly property color canvasBorder: dark ? "#66757575" : "#0F000000"
    readonly property color rulerBg: dark ? "#202020" : "#F3F3F3"
    readonly property color rulerFg: dark ? "#CDCDCD" : "#5F5F5F"
    readonly property color checkerA: dark ? "#A9A9A9" : "#EFEFEF"
    readonly property color checkerB: dark ? "#979797" : "#D2D2D2"
    readonly property color hiddenVeil: dark ? "#B31C1C1C" : "#C9FFFFFF"

    // Texto
    readonly property color textPrimary: dark ? "#FFFFFF" : "#E4000000"
    readonly property color textSecondary: dark ? "#C5FFFFFF" : "#9E000000"
    readonly property color textTertiary: dark ? "#87FFFFFF" : "#72000000"
    readonly property color textDisabled: dark ? "#5DFFFFFF" : "#5C000000"

    // Íconos
    readonly property color iconBrush: dark ? "#FFFFFF" : "#E6000000"
    readonly property color iconHighlight: dark ? "#CC60CDFF" : "#005FB7"

    // Estados
    readonly property color hoverBg: dark ? "#15FFFFFF" : "#80F9F9F9"
    readonly property color pressedBg: dark ? "#08FFFFFF" : "#4DF9F9F9"
    readonly property color selectedBg: dark ? "#0AFFFFFF" : "#06000000"
    readonly property color selectedInner: dark ? "#454545" : "#FFFFFF"
    readonly property color subtleHover: dark ? "#0FFFFFFF" : "#09000000"
    readonly property color subtlePressed: dark ? "#0AFFFFFF" : "#06000000"
    readonly property color controlFill: dark ? "#0FFFFFFF" : "#B3FFFFFF"
    readonly property color controlStroke: dark ? "#12FFFFFF" : "#0F000000"
    readonly property color controlStrokeBottom: dark ? "#18FFFFFF" : "#29000000"
    readonly property color controlSolid: dark ? "#454545" : "#FFFFFF"
    readonly property color strongStroke: dark ? "#8BFFFFFF" : "#72000000"
    readonly property color critical: dark ? "#FF99A4" : "#C42B1C"

    // --- Diálogos de imagen --- (controles de diálogo)
    readonly property color layerAlt: dark ? "#2B2B2B" : "#FFFFFF"            // LayerFillColorAlt sobre la base
    readonly property color surfaceStroke: dark ? "#66757575" : "#66757575"   // SurfaceStrokeColorDefault
    readonly property color inputActive: dark ? "#1E1E1E" : "#FFFFFF"         // ControlFillColorInputActive
    readonly property color controlAltFill: dark ? "#19000000" : "#06000000"  // ControlAltFillColorSecondary
    readonly property color controlStrongFill: dark ? "#8BFFFFFF" : "#72000000" // ControlStrongFillColorDefault
    readonly property color accentHover: dark ? "#E660CDFF" : "#E6005FB8"     // AccentFillColorSecondary (90 %)
    readonly property color accentPressed: dark ? "#CC60CDFF" : "#CC005FB8"   // AccentFillColorTertiary (80 %)
    readonly property color accentDisabled: dark ? "#28FFFFFF" : "#37000000"
    // Texto sobre acento. Ojo: un nombre «on» + mayúscula (p. ej. `onAccent`) QML lo toma como manejador de señal y la
    // propiedad queda en negro; por eso estos nombres.
    readonly property color textOnAccent: dark ? "#000000" : "#FFFFFF"
    readonly property color textOnAccentDisabled: dark ? "#87FFFFFF" : "#FFFFFF"
    readonly property color systemChromeMedium: dark ? "#1F1F1F" : "#E6E6E6"  // SystemChromeMediumColor

    // Medidas
    readonly property int controlRadius: 4
    readonly property int overlayRadius: 8
    readonly property int bodySize: 14
    readonly property int captionSize: 12
}
