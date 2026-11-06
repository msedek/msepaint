import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import MsePaint

// --- Archivos e impresión ---
// Flujo de abrir/guardar con sus avisos, arrastrar y soltar,
// y estado de la ventana guardado en QSettings.
Item {
    id: ff
    required property AppController app
    required property ApplicationWindow window
    property bool closeConfirmed: false

    // ---- Comandos del menú File ----
    function newImage() { confirmDiscard(() => ff.app.newImage()) }
    function open() { confirmDiscard(() => openDialog.open()) }
    function openUrl(url) { confirmDiscard(() => ff.app.openFile(url)) }
    function openRecent(path) { openUrl(ff.app.localFileUrl(path)) }

    // Save: con archivo y formato escribible guarda en el mismo; si no, «Save as». `then` se llama tras guardar.
    function save(then) {
        const url = ff.app.fileUrl
        if (url.toString() === "") { saveAs("png", then); return }
        const fmt = ff.app.formatForSave(url, ff.app.fileFormat)
        if (fmt !== ff.app.fileFormat) { saveAs(fmt, then); return }
        startSave(url, fmt, then)
    }
    // format: png | jpg | bmp | gif | paint | "" (Other format: diálogo con todos los tipos)
    function saveAs(format, then) {
        const fmt = format === "" ? (ff.app.fileFormat !== "" ? ff.app.fileFormat : "png") : format
        saveDialog.then = then === undefined ? null : then
        saveDialog.selectedNameFilter.index = ff.app.saveFilterIndex(fmt)
        saveDialog.selectedFile = ff.app.suggestedSaveUrl(ff.app.saveFormats[ff.app.saveFilterIndex(fmt)].id)
        saveDialog.currentFolder = ff.folderOf(saveDialog.selectedFile)
        saveDialog.open()
    }

    function startSave(url, fmt, then) {
        if (ff.app.needsFlattenPrompt(fmt)) {
            flattenDialog.url = url; flattenDialog.fmt = fmt; flattenDialog.then = then === undefined ? null : then
            flattenDialog.open()
            return
        }
        checkColors(url, fmt, then)
    }
    function checkColors(url, fmt, then) {
        if (ff.app.needsColorWarning(fmt)) {
            colorDialog.url = url; colorDialog.fmt = fmt; colorDialog.then = then === undefined ? null : then
            colorDialog.open()
            return
        }
        finishSave(url, fmt, then)
    }
    function finishSave(url, fmt, then) {
        if (ff.app.saveDocumentAs(url, fmt, false) && then) then()
    }

    // Guardar cambios antes de New, Open o cerrar.
    function confirmDiscard(action) {
        if (!ff.app.modified) { action(); return }
        saveChangesDialog.action = action
        saveChangesDialog.open()
    }

    function printDocument() { ff.app.print(true) }
    function pageSetup() { ff.app.pageSetup() }
    function printPreview() { ff.app.printPreview() }

    // Fondo de escritorio: hay que guardar antes (MustSaveWallpaper).
    function desktopBackground(mode) {
        if (ff.app.fileUrl.toString() === "" || ff.app.modified) {
            wallpaperDialog.mode = mode
            wallpaperDialog.open()
            return
        }
        ff.app.setDesktopBackground(mode)
    }

    // Cierre de la ventana (onClosing de Main.qml).
    function handleClosing(close) {
        if (!ff.closeConfirmed && ff.app.modified) {
            close.accepted = false
            confirmDiscard(() => { ff.closeConfirmed = true; ff.window.close() })
            return
        }
        persist()
    }

    function folderOf(url) {
        const s = url.toString()
        return s.substring(0, s.lastIndexOf("/"))
    }

    // ---- Persistencia de la ventana, la cinta y el tema ----
    function persist() {
        const w = ff.window
        const maximized = w.visibility === Window.Maximized
        ff.app.setSettingValue("Window/maximized", maximized)
        if (!maximized && w.visibility !== Window.FullScreen) {
            ff.app.setSettingValue("Window/x", w.x)
            ff.app.setSettingValue("Window/y", w.y)
            ff.app.setSettingValue("Window/width", w.width)
            ff.app.setSettingValue("Window/height", w.height)
        }
        ff.app.setSettingValue("View/toolbarMode", w.ribbonAutoHide ? 1 : 0)
        ff.app.setSettingValue("View/theme", ["system", "light", "dark"].indexOf(Theme.themeMode))
        ff.app.saveSettings()
    }
    function restore() {
        const w = ff.window
        const width = Number(ff.app.settingValue("Window/width", 0))
        const height = Number(ff.app.settingValue("Window/height", 0))
        if (width >= 320 && height >= 240) {
            w.width = width; w.height = height
            w.x = Number(ff.app.settingValue("Window/x", w.x))
            w.y = Number(ff.app.settingValue("Window/y", w.y))
        } else {
            // Sin posición guardada: 10 % de margen y 80 % de cada lado de la pantalla.
            w.x = Screen.width * 0.1; w.y = Screen.height * 0.1
            w.width = Screen.width * 0.8; w.height = Screen.height * 0.8
        }
        if (String(ff.app.settingValue("Window/maximized", false)) === "true") w.visibility = Window.Maximized
        const autoHide = Number(ff.app.settingValue("View/toolbarMode", 0)) === 1
        w.ribbonAutoHide = autoHide
        w.ribbonShown = !autoHide
        const theme = Number(ff.app.settingValue("View/theme", 0))
        Theme.themeMode = ["system", "light", "dark"][theme >= 0 && theme <= 2 ? theme : 0]
    }
    Component.onCompleted: if (!ff.app.devMode()) restore()

    // ---- Arrastrar y soltar un archivo sobre la ventana: abrir ----
    DropArea {
        anchors.fill: parent
        onEntered: (drag) => { drag.accepted = drag.hasUrls && drag.urls.length > 0 && ff.app.isOpenableFile(drag.urls[0]) }
        onDropped: (drop) => {
            if (!drop.hasUrls || drop.urls.length === 0 || !ff.app.isOpenableFile(drop.urls[0])) return
            drop.accept(Qt.CopyAction)
            const url = drop.urls[0]
            Qt.callLater(() => ff.openUrl(url))
        }
    }

    Connections {
        target: ff.app
        function onErrorMessage(text) { messageDialog.text = text; messageDialog.open() }
    }

    // ---- Diálogos del sistema ----
    FileDialog {
        id: openDialog
        title: qsTr("Open")
        nameFilters: ff.app.openNameFilters
        selectedNameFilter.index: Math.max(0, ff.app.openNameFilters.length - 2) // «All Picture Files»
        onAccepted: ff.app.openFile(selectedFile)
    }
    FileDialog {
        id: saveDialog
        property var then: null
        title: qsTr("Save as")
        fileMode: FileDialog.SaveFile
        nameFilters: ff.app.saveFormats.map(f => f.filter)
        onAccepted: {
            const fmt = ff.app.formatForSave(selectedFile, ff.app.saveFormats[selectedNameFilter.index].id)
            ff.startSave(selectedFile, fmt, then)
        }
    }

    // ---- Diálogos propios ----
    component Message: Text {
        Layout.fillWidth: true
        Layout.preferredWidth: 452
        font.family: Theme.uiFont
        font.pixelSize: Theme.bodySize
        color: Theme.textPrimary
        wrapMode: Text.Wrap
    }

    WDialog {
        id: saveChangesDialog
        property var action: null
        title: qsTr("Do you want to save your work?")
        primaryText: qsTr("Save")
        secondaryText: qsTr("Don’t save")
        closeText: qsTr("Cancel")
        Message { text: qsTr("There are unsaved changes in “%1”.").arg(ff.app.fileName) }
        onAccepted: { const a = action; Qt.callLater(() => ff.save(a)) }
        onSecondaryClicked: { const a = action; Qt.callLater(a) }
        onRejected: ff.closeConfirmed = false
    }

    WDialog {
        id: flattenDialog
        property url url
        property string fmt
        property var then: null
        title: qsTr("Save as flattened image?")
        primaryText: qsTr("Continue")
        secondaryText: qsTr("Save as project file")
        closeText: qsTr("Cancel")
        Message { text: qsTr("This format keeps only the flattened picture, so the layers can't be edited afterwards. Save as a project to keep them editable.") }
        onAccepted: { const u = url, f = fmt, t = then; Qt.callLater(() => ff.checkColors(u, f, t)) }
        onSecondaryClicked: { const t = then; Qt.callLater(() => ff.saveAs("paint", t)) }
    }

    WDialog {
        id: colorDialog
        property url url
        property string fmt
        property var then: null
        title: ""
        primaryText: qsTr("OK")
        closeText: qsTr("Cancel")
        Message { text: qsTr("This format supports fewer colors, so some colors may change.") }
        Message { Layout.topMargin: 12; text: qsTr("Do you want to continue?") }
        onAccepted: { const u = url, f = fmt, t = then; Qt.callLater(() => ff.finishSave(u, f, t)) }
    }

    WDialog {
        id: wallpaperDialog
        property string mode
        title: qsTr("Set as desktop background")
        primaryText: qsTr("Save")
        closeText: qsTr("Cancel")
        Message { text: qsTr("Save the picture first to use it as the desktop background.") }
        onAccepted: {
            const m = mode
            Qt.callLater(() => ff.save(() => ff.app.setDesktopBackground(m)))
        }
    }

    WDialog {
        id: messageDialog
        property string text
        title: ""
        primaryText: qsTr("OK")
        closeText: ""
        Message { text: messageDialog.text }
    }
}
