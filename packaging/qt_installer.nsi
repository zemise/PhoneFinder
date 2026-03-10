Unicode true
RequestExecutionLevel admin

!ifndef APP_NAME
  !define APP_NAME "PhoneFinder"
!endif
!ifndef APP_VERSION
  !define APP_VERSION "0.2.0"
!endif
!ifndef APP_EXE
  !define APP_EXE "PhoneFinder.exe"
!endif
!ifndef APP_STAGE_DIR
  !define APP_STAGE_DIR "."
!endif
!ifndef APP_OUTFILE
  !define APP_OUTFILE "PhoneFinder-installer.exe"
!endif

Name "${APP_NAME} ${APP_VERSION}"
OutFile "${APP_OUTFILE}"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
ShowInstDetails show

!include "MUI2.nsh"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "SimpChinese"

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${APP_STAGE_DIR}\*.*"

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$SMPROGRAMS\${APP_NAME}"
  RMDir /r "$INSTDIR"
SectionEnd
