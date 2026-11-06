#pragma once

#include "core/Document.h"
#include "core/FileFormats.h"
#include "core/Tool.h"
#include "core/tools/Text.h"

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <cmath>
#include <map>
#include <memory>

struct PageSettings; // --- Archivos e impresión --- (ui/Printing.h)

// Estado de la aplicación expuesto a QML: documento, herramienta, colores, zoom y comandos.
class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Se crea en main")
    Q_PROPERTY(QString tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY colorsChanged)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY colorsChanged)
    Q_PROPERTY(int activeColor READ activeColor WRITE setActiveColor NOTIFY colorsChanged)
    Q_PROPERTY(int toolSize READ toolSize WRITE setToolSize NOTIFY toolSizeChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QSize canvasSize READ canvasSize NOTIFY canvasSizeChanged)
    Q_PROPERTY(QPoint cursorPos READ cursorPos NOTIFY cursorChanged)
    Q_PROPERTY(bool cursorInside READ cursorInside NOTIFY cursorChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY historyChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileChanged)
    Q_PROPERTY(bool showGridlines READ showGridlines WRITE setShowGridlines NOTIFY viewChanged)
    Q_PROPERTY(bool showRulers READ showRulers WRITE setShowRulers NOTIFY viewChanged)
    Q_PROPERTY(bool showStatusBar READ showStatusBar WRITE setShowStatusBar NOTIFY viewChanged)
    // Variantes de herramienta
    Q_PROPERTY(QString selectMode MEMBER m_selectMode NOTIFY toolOptionsChanged)       // rectangle | freeform
    Q_PROPERTY(bool transparentSelection MEMBER m_transparentSelection NOTIFY toolOptionsChanged)
    Q_PROPERTY(QString brush READ brush WRITE setBrush NOTIFY toolOptionsChanged)      // brush, calligraphyBrush, ...
    Q_PROPERTY(int toolSizeMin READ toolSizeMin NOTIFY toolSizeChanged)
    Q_PROPERTY(int toolSizeMax READ toolSizeMax NOTIFY toolSizeChanged)
    Q_PROPERTY(bool toolHasSize READ toolHasSize NOTIFY toolChanged)
    Q_PROPERTY(bool toolHasOpacity READ toolHasOpacity NOTIFY toolChanged)
    Q_PROPERTY(QString shape MEMBER m_shape NOTIFY toolOptionsChanged)                 // line, curve, oval, ...
    Q_PROPERTY(QString shapeOutline MEMBER m_shapeOutline NOTIFY toolOptionsChanged)   // none, solid, crayon, ...
    Q_PROPERTY(QString shapeFill MEMBER m_shapeFill NOTIFY toolOptionsChanged)
    Q_PROPERTY(int opacity READ opacityPercent WRITE setOpacityPercent NOTIFY toolOptionsChanged)
    Q_PROPERTY(int tolerance READ tolerance WRITE setTolerance NOTIFY toolOptionsChanged)
    Q_PROPERTY(QVariantList customColors READ customColors NOTIFY colorsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QSize selectionSize READ selectionSize NOTIFY selectionChanged)
    Q_PROPERTY(int layerCount READ layerCount NOTIFY layersChanged)
    Q_PROPERTY(int activeLayer READ activeLayer WRITE setActiveLayer NOTIFY layersChanged)
    Q_PROPERTY(int layersRevision READ layersRevision NOTIFY layersChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY layersChanged)
    Q_PROPERTY(bool backgroundVisible READ backgroundVisible NOTIFY layersChanged)
    Q_PROPERTY(QString fileSizeText READ fileSizeText NOTIFY fileChanged)
    // --- Herramientas ---
    Q_PROPERTY(int cursorShape READ cursorShape NOTIFY cursorShapeChanged)
    Q_PROPERTY(QPointF viewOrigin MEMBER m_viewOrigin)
    Q_PROPERTY(bool textEditing READ textEditing NOTIFY textChanged)
    Q_PROPERTY(QString textFamily READ textFamily WRITE setTextFamily NOTIFY textChanged)
    Q_PROPERTY(qreal textSize READ textSize WRITE setTextSize NOTIFY textChanged)
    Q_PROPERTY(bool textBold READ textBold WRITE setTextBold NOTIFY textChanged)
    Q_PROPERTY(bool textItalic READ textItalic WRITE setTextItalic NOTIFY textChanged)
    Q_PROPERTY(bool textUnderline READ textUnderline WRITE setTextUnderline NOTIFY textChanged)
    Q_PROPERTY(bool textStrike READ textStrike WRITE setTextStrike NOTIFY textChanged)
    Q_PROPERTY(int textAlign READ textAlign WRITE setTextAlign NOTIFY textChanged)   // 0 izq, 1 centro, 2 der
    Q_PROPERTY(bool textFill READ textFill WRITE setTextFill NOTIFY textChanged)
    // --- Diálogos de imagen ---
    Q_PROPERTY(int units READ units WRITE setUnits NOTIFY unitsChanged) // 0 Pixels, 1 Inches, 2 Centimeters
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    // --- Archivos e impresión ---
    Q_PROPERTY(QString windowTitle READ windowTitle NOTIFY fileChanged)
    Q_PROPERTY(QString fileFormat READ fileFormat NOTIFY fileChanged)          // id de mp::files (png, jpg, bmp8, paint…)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QVariantList saveFormats READ saveFormats CONSTANT)             // [{id, filter}] en el orden del diálogo
    Q_PROPERTY(QStringList openNameFilters READ openNameFilters CONSTANT)
    Q_PROPERTY(bool hasProjectSupport READ hasProjectSupport CONSTANT)
    Q_PROPERTY(bool canSetDesktopBackground READ canSetDesktopBackground CONSTANT)
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    mp::Document *document() const { return m_doc; }

    QString tool() const { return m_tool; }
    void setTool(const QString &id);
    QColor color1() const { return m_ctx.color1; }
    QColor color2() const { return m_ctx.color2; }
    void setColor1(const QColor &c);
    void setColor2(const QColor &c);
    int activeColor() const { return m_activeColor; }
    void setActiveColor(int which);
    int toolSize() const { return m_ctx.size; }
    void setToolSize(int s);
    qreal zoom() const { return m_zoom; }
    void setZoom(qreal z);
    QSize canvasSize() const { return m_doc->size(); }
    QPoint cursorPos() const { return m_cursor; }
    bool cursorInside() const { return m_cursorInside; }
    bool canUndo() const;
    bool canRedo() const;
    bool modified() const;
    QString fileName() const;
    QUrl fileUrl() const { return m_fileUrl; }
    bool showGridlines() const { return m_gridlines; }
    void setShowGridlines(bool v);
    bool showRulers() const { return m_rulers; }
    void setShowRulers(bool v);
    bool showStatusBar() const { return m_statusBar; }
    void setShowStatusBar(bool v);

    QString brush() const { return m_brush; }
    void setBrush(const QString &id);
    int toolSizeMin() const;
    int toolSizeMax() const;
    bool toolHasSize() const;
    bool toolHasOpacity() const;
    Q_INVOKABLE void stepToolSize(int delta);
    Q_INVOKABLE QVariantList toolSizeSteps() const;
    int opacityPercent() const { return int(std::lround(m_ctx.opacity * 100)); }
    void setOpacityPercent(int v);
    int tolerance() const { return m_ctx.tolerance; }
    void setTolerance(int v);
    QVariantList customColors() const { return m_customColors; }
    bool hasSelection() const;
    QSize selectionSize() const;
    int layerCount() const { return m_doc->layerCount(); }
    int activeLayer() const { return m_doc->activeLayer(); }
    void setActiveLayer(int i) { m_doc->setActiveLayer(i); }
    int layersRevision() const { return m_layersRevision; }
    QColor backgroundColor() const { return m_doc->background(); }
    bool backgroundVisible() const { return m_doc->backgroundVisible(); }
    Q_INVOKABLE void toggleBackgroundVisible();
    Q_INVOKABLE void setBackgroundColor(const QColor &c);
    QString fileSizeText() const;
    Q_INVOKABLE bool layerVisible(int i) const { return i >= 0 && i < m_doc->layerCount() && m_doc->layer(i).visible; }

    // Paleta: elige color para el "color activo" (1 o 2).
    Q_INVOKABLE void pickPaletteColor(const QColor &c);
    Q_INVOKABLE void addCustomColor(const QColor &c);
    Q_INVOKABLE void swapColors();

    // Imagen
    Q_INVOKABLE void rotateRight();
    Q_INVOKABLE void rotateLeft();
    Q_INVOKABLE void rotate180();
    Q_INVOKABLE void flipHorizontal();
    Q_INVOKABLE void flipVertical();
    Q_INVOKABLE void resizeImage(int width, int height);
    Q_INVOKABLE void resizeCanvas(int width, int height);
    Q_INVOKABLE void invertColors();
    // Selección y portapapeles
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void invertSelection();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void crop();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void copyVisibleLayers();
    Q_INVOKABLE void cancelPending();
    // Capas
    Q_INVOKABLE void addLayer();
    Q_INVOKABLE void duplicateLayer();
    Q_INVOKABLE void deleteLayer();
    Q_INVOKABLE void mergeDown();
    Q_INVOKABLE void moveLayerUp();
    Q_INVOKABLE void moveLayerDown();
    Q_INVOKABLE void toggleLayerVisible(int index);
    // --- Herramientas ---
    int cursorShape() const;
    bool textEditing() const;
    QString textFamily() const;
    void setTextFamily(const QString &v);
    qreal textSize() const;
    void setTextSize(qreal v);
    bool textBold() const;
    void setTextBold(bool v);
    bool textItalic() const;
    void setTextItalic(bool v);
    bool textUnderline() const;
    void setTextUnderline(bool v);
    bool textStrike() const;
    void setTextStrike(bool v);
    int textAlign() const;
    void setTextAlign(int v);
    bool textFill() const;
    void setTextFill(bool v);
    Q_INVOKABLE QStringList fontFamilies() const;
    // Teclado del lienzo (texto en edición, Enter/flechas). Devuelve true si se consumió.
    Q_INVOKABLE bool keyPress(int key, int modifiers, const QString &text);
    void pointerDoubleClick(QPointF p);
    void inputText(const QString &text);

    // Formas: ids en orden de galería y silueta SVG ajustada a un cuadrado de `size`.
    Q_INVOKABLE QStringList shapeIds() const;
    Q_INVOKABLE QString shapeIconPath(const QString &id, qreal size) const;

    // Vista
    Q_INVOKABLE void zoom100() { setZoom(1.0); }
    Q_INVOKABLE void fitToWindow(qreal viewportWidth, qreal viewportHeight);
    Q_INVOKABLE void newImage();
    Q_INVOKABLE bool openFile(const QUrl &url);
    Q_INVOKABLE bool saveFile(const QUrl &url = QUrl());
    Q_INVOKABLE bool importFile(const QUrl &url);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

    // --- Diálogos de imagen ---
    int units() const { return m_units; }
    void setUnits(int u);
    QString appVersion() const;
    Q_INVOKABLE qreal dpi() const { return m_doc->dpi(); }
    // Resize and Skew: escala (si cambia) y luego inclina en horizontal y vertical (grados, 0 = nada).
    Q_INVOKABLE void resizeAndSkew(int width, int height, int skewH, int skewV);
    Q_INVOKABLE void rotateCustom(qreal degrees);
    // Image properties: «Last Saved», «Size on disk» (texto vacío = «Not Available») y tamaño por defecto.
    Q_INVOKABLE QVariantMap imageFileInfo() const;
    Q_INVOKABLE QSize defaultImageSize() const;
    Q_INVOKABLE void applyImageProperties(int width, int height, int units);
    // Desarrollo: MSEPAINT_OPEN_DIALOG=resize|rotate|properties|colors|background|settings abre ese diálogo al arrancar
    // (sirve para capturas sin pantalla junto con MSEPAINT_SCREENSHOT).
    Q_INVOKABLE QString devOpenDialog() const { return qEnvironmentVariable("MSEPAINT_OPEN_DIALOG"); }

    // --- Archivos e impresión ---
    QString windowTitle() const;
    QString fileFormat() const;
    QStringList recentFiles() const { return m_recentFiles; }
    QVariantList saveFormats() const;
    QStringList openNameFilters() const;
    bool hasProjectSupport() const;
    bool canSetDesktopBackground() const;
    // Formato con el que se guardará: el del filtro elegido, salvo que la extensión escrita indique otro.
    Q_INVOKABLE QString formatForSave(const QUrl &url, const QString &filterFormat) const;
    Q_INVOKABLE int saveFilterIndex(const QString &format) const;
    Q_INVOKABLE QUrl suggestedSaveUrl(const QString &format) const;
    // Avisos previos a guardar: aplanar capas y pérdida de color.
    Q_INVOKABLE bool needsFlattenPrompt(const QString &format) const;
    Q_INVOKABLE bool needsColorWarning(const QString &format) const;
    // Guarda sin preguntar. `copy` = «Save copy as» (no cambia el archivo del documento).
    Q_INVOKABLE bool saveDocumentAs(const QUrl &url, const QString &format, bool copy = false);
    Q_INVOKABLE bool isOpenableFile(const QUrl &url) const;
    Q_INVOKABLE QString recentDisplayName(const QString &path) const;
    Q_INVOKABLE QUrl localFileUrl(const QString &path) const { return QUrl::fromLocalFile(path); }
    // Captura de desarrollo (MSEPAINT_SCREENSHOT): no se restaura la ventana ni se cargan ajustes.
    Q_INVOKABLE bool devMode() const { return qEnvironmentVariableIsSet("MSEPAINT_SCREENSHOT"); }
    // Ajustes persistentes (QSettings). Las claves sueltas sirven también a QML (ventana, cinta, tema).
    Q_INVOKABLE QVariant settingValue(const QString &key, const QVariant &def = QVariant()) const;
    Q_INVOKABLE void setSettingValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();
    // Impresión.
    Q_INVOKABLE bool print(bool showDialog = true);
    Q_INVOKABLE void pageSetup();
    Q_INVOKABLE void printPreview();
    // Fondo de escritorio: mode = fill | tile | center. El archivo debe estar guardado.
    Q_INVOKABLE bool setDesktopBackground(const QString &mode);

    // Entrada del lienzo (coordenadas de imagen).
    void pointerPress(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m);
    void pointerMove(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m);
    void pointerRelease(QPointF p, Qt::MouseButton b, Qt::KeyboardModifiers m);
    void pointerHover(QPointF p, Qt::KeyboardModifiers m);
    void pointerLeave();
    void paintOverlay(QPainter &p, qreal scale) const;

signals:
    // --- Herramientas ---
    void cursorShapeChanged();
    void textChanged();
    // La lupa pide centrar la vista en un punto de la imagen tras cambiar el zoom.
    void centerViewOn(QPointF imagePoint);
    // Clic derecho con la herramienta Selección: menú contextual del lienzo.
    void contextMenuRequested();
    void toolChanged();
    void colorsChanged();
    void toolSizeChanged();
    void zoomChanged();
    void canvasSizeChanged();
    void cursorChanged();
    void historyChanged();
    void fileChanged();
    void viewChanged();
    void overlayChanged();
    void toolOptionsChanged();
    void selectionChanged();
    void layersChanged();
    // --- Diálogos de imagen ---
    void unitsChanged();
    // --- Archivos e impresión ---
    void recentFilesChanged();
    void errorMessage(const QString &text); // QML lo muestra en un diálogo simple

private:
    mp::Tool *currentTool() const;
    void setCursor(QPointF p, bool inside);
    QString sizeKey() const;  // herramienta o variante de pincel a la que pertenece el grosor
    QString opacityKey() const;
    QTimer *m_airTimer = nullptr;
    std::map<QString, qreal> m_opacities;

    mp::Document *m_doc;
    mp::ToolContext m_ctx;
    std::map<QString, std::unique_ptr<mp::Tool>> m_tools;
    std::map<QString, int> m_sizes; // grosor recordado por herramienta
    QString m_tool;
    int m_activeColor = 1;
    qreal m_zoom = 1.0;
    QPoint m_cursor;
    bool m_cursorInside = false;
    QUrl m_fileUrl;
    bool m_gridlines = false;
    bool m_rulers = false;
    bool m_statusBar = true;
    QString m_selectMode = QStringLiteral("rectangle");
    bool m_transparentSelection = false;
    QString m_brush = QStringLiteral("brush");
    QString m_shape = QStringLiteral("line");
    QString m_shapeOutline = QStringLiteral("solid");
    QString m_shapeFill = QStringLiteral("none");
    QVariantList m_customColors;
    int m_layersRevision = 0;
    // --- Herramientas ---
    void syncToolOptions();
    void afterToolEvent();
    mp::TextFormat textFormat() const;
    void setTextFormat(const mp::TextFormat &f);
    QString m_previousTool;   // el selector de color vuelve a la herramienta anterior
    QPointF m_viewOrigin;
    int m_cursorShape = Qt::CrossCursor;
    mp::TextFormat m_textFormat;
    // --- Diálogos de imagen ---
    int m_units = 0;
    // --- Archivos e impresión ---
    void addRecentFile(const QString &path);
    void loadIntoDocument(const mp::files::ImageData &data);
    mp::files::Format m_fileFormat = mp::files::Format::Unknown;
    int m_docBpp = 32;                  // profundidad del documento (aviso de calidad de color)
    QStringList m_recentFiles;
    std::unique_ptr<PageSettings> m_page;
};
