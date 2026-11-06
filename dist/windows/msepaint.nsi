; Instalador de msepaint para Windows (NSIS 3, Modern UI 2).
; Instala por usuario, sin pedir permisos de administrador, en %LOCALAPPDATA%\Programs\msepaint.
; Uso: makensis -DVERSION=0.1.0 -DSRCDIR=<carpeta desplegada> -DOUTFILE=<setup.exe> msepaint.nsi

Unicode true
SetCompressor /SOLID lzma

!ifndef VERSION
  !define VERSION "0.1.0"
!endif
!define APPNAME "msepaint"
!define PUBLISHER "Msedek"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\msepaint"

Name "${APPNAME} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$LOCALAPPDATA\Programs\msepaint"
InstallDirRegKey HKCU "Software\msepaint" "InstallDir"
RequestExecutionLevel user
BrandingText "${APPNAME} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "${APPNAME} installer"
VIAddVersionKey "CompanyName" "${PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2026 ${PUBLISHER}. MIT License."

!include "MUI2.nsh"
!define MUI_ICON "${SRCDIR}\..\msepaint.ico"
!define MUI_UNICON "${SRCDIR}\..\msepaint.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\msepaint.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run msepaint"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${SRCDIR}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"

Section "msepaint" SecMain
  SectionIn RO
  ; Una instalación previa se reemplaza completa.
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\qml"
  SetOutPath "$INSTDIR"
  File /r "${SRCDIR}\*.*"
  File "${SRCDIR}\..\msepaint.ico"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  CreateShortcut "$SMPROGRAMS\msepaint.lnk" "$INSTDIR\msepaint.exe" "" "$INSTDIR\msepaint.exe" 0

  ; "Abrir con": aparece en el menú de imágenes sin cambiar la app predeterminada.
  WriteRegStr HKCU "Software\Classes\Applications\msepaint.exe" "FriendlyAppName" "msepaint"
  WriteRegStr HKCU "Software\Classes\Applications\msepaint.exe\shell\open\command" "" '"$INSTDIR\msepaint.exe" "%1"'
  !macro OpenWith EXT
    WriteRegStr HKCU "Software\Classes\Applications\msepaint.exe\SupportedTypes" "${EXT}" ""
    WriteRegStr HKCU "Software\Classes\${EXT}\OpenWithList\msepaint.exe" "" ""
  !macroend
  !insertmacro OpenWith ".png"
  !insertmacro OpenWith ".jpg"
  !insertmacro OpenWith ".jpeg"
  !insertmacro OpenWith ".bmp"
  !insertmacro OpenWith ".gif"
  !insertmacro OpenWith ".tif"
  !insertmacro OpenWith ".tiff"
  !insertmacro OpenWith ".ico"
  !insertmacro OpenWith ".webp"

  ; Programas y características
  WriteRegStr HKCU "Software\msepaint" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayName" "msepaint"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINSTKEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKCU "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\msepaint.exe"
  WriteRegStr HKCU "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "${UNINSTKEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKCU "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTKEY}" "NoRepair" 1
SectionEnd

Section "Desktop shortcut" SecDesktop
  CreateShortcut "$DESKTOP\msepaint.lnk" "$INSTDIR\msepaint.exe" "" "$INSTDIR\msepaint.exe" 0
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\msepaint.lnk"
  Delete "$DESKTOP\msepaint.lnk"
  DeleteRegKey HKCU "Software\Classes\Applications\msepaint.exe"
  !macro UnOpenWith EXT
    DeleteRegKey HKCU "Software\Classes\${EXT}\OpenWithList\msepaint.exe"
  !macroend
  !insertmacro UnOpenWith ".png"
  !insertmacro UnOpenWith ".jpg"
  !insertmacro UnOpenWith ".jpeg"
  !insertmacro UnOpenWith ".bmp"
  !insertmacro UnOpenWith ".gif"
  !insertmacro UnOpenWith ".tif"
  !insertmacro UnOpenWith ".tiff"
  !insertmacro UnOpenWith ".ico"
  !insertmacro UnOpenWith ".webp"
  DeleteRegKey HKCU "${UNINSTKEY}"
  DeleteRegKey HKCU "Software\msepaint"
  RMDir /r "$INSTDIR"
SectionEnd
