; GPL-3.0-only; copyright 2026 Oleg Shchepin.
; Build through make-installer.ps1. All definitions are supplied by it.
Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"
Name "Kleiotide ${APP_VERSION} test ${TEST_REVISION}"
OutFile "${SETUP_FILE}"
InstallDir "$PROGRAMFILES64\Kleiotide Test ${APP_VERSION}-${TEST_REVISION}"
RequestExecutionLevel admin
ManifestDPIAware true
SetCompressor /SOLID zlib
ShowInstDetails show
ShowUninstDetails show
!define APP_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Kleiotide-Test-${APP_VERSION}-${TEST_REVISION}"
!define MENU_DIR "Kleiotide Test ${APP_VERSION}-${TEST_REVISION}"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_REBOOTLATER_DEFAULT
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PAYLOAD_DIR}\LICENSE"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "This test build requires Windows 10 or Windows 11."
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastBuild} 17763
    MessageBox MB_OK|MB_ICONSTOP "Windows 10 version 1809 or later is required."
    Abort
  ${EndIf}
  ${If} ${IsNativeAMD64}
    ; Native x64 Windows follows the existing path, including Windows 10.
  ${ElseIf} ${IsNativeARM64}
    ${IfNot} ${AtLeastBuild} 22000
      MessageBox MB_OK|MB_ICONSTOP "This x64 build can be tested on ARM64 using Windows 11 x64 emulation. Windows 10 on ARM is not supported."
      Abort
    ${EndIf}
    ; Windows 11 reports whether x64 user-mode applications can actually run.
    ; MACHINE_ATTRIBUTES_USER_ENABLED is bit 0; HRESULT S_OK is zero.
    StrCpy $0 0
    StrCpy $1 -1
    System::Call 'kernel32::GetMachineTypeAttributes(i 0x8664, *i .r0) i.r1'
    IntOp $0 $0 & 1
    ${If} $1 != 0
    ${OrIf} $0 == 0
      MessageBox MB_OK|MB_ICONSTOP "Windows did not report support for x64 applications on this ARM64 computer. Windows 11 with x64 emulation is required."
      Abort
    ${EndIf}
  ${Else}
    MessageBox MB_OK|MB_ICONSTOP "This build requires x64 Windows 10/11, or ARM64 Windows 11 with x64 emulation. 32-bit Windows is not supported."
    Abort
  ${EndIf}
  SetRegView 64
  SetShellVarContext all
  ; The versioned destination is fixed. Never merge files with another install.
  StrCpy $INSTDIR "$PROGRAMFILES64\Kleiotide Test ${APP_VERSION}-${TEST_REVISION}"
  IfFileExists "$INSTDIR\*.*" 0 fresh_install
    MessageBox MB_OK|MB_ICONSTOP "This test version already has an installation folder. Uninstall it through Windows Settings first. If files remain, move them out of that folder before installing again."
    Abort
  fresh_install:
FunctionEnd

; Advertise Open With only; never write extension defaults or UserChoice.
!define OPENWITH_APP "Software\Classes\Applications\kleiotide.exe"
!define OPENWITH_PROG "Software\Classes\Kleiotide.Test.${APP_VERSION}.${TEST_REVISION}"
!macro RegisterOpenWith ROOT
  ReadRegStr $0 ${ROOT} "Software\Classes\Kleiotide.File\shell\open\command" ""
  ${If} $0 != ""
    WriteRegStr ${ROOT} "Software\Classes\Kleiotide.File\shell\open\command" "" '$\"$INSTDIR\kleiotide.exe$\" $\"%1$\"'
  ${EndIf}
  WriteRegStr ${ROOT} "${OPENWITH_APP}" "FriendlyAppName" "Kleiotide"
  WriteRegStr ${ROOT} "${OPENWITH_APP}\shell\open\command" "" '$\"$INSTDIR\kleiotide.exe$\" $\"%1$\"'
  WriteRegStr ${ROOT} "${OPENWITH_APP}\shell\open" "MultiSelectModel" "Player"
  WriteRegStr ${ROOT} "${OPENWITH_PROG}" "" "Kleiotide alignment, trace or session"
  WriteRegStr ${ROOT} "${OPENWITH_PROG}\shell\open\command" "" '$\"$INSTDIR\kleiotide.exe$\" $\"%1$\"'
  !insertmacro OpenWithExtension ${ROOT} ".ab1"
  !insertmacro OpenWithExtension ${ROOT} ".abi"
  !insertmacro OpenWithExtension ${ROOT} ".kleio"
  !insertmacro OpenWithExtension ${ROOT} ".fasta"
  !insertmacro OpenWithExtension ${ROOT} ".fas"
  !insertmacro OpenWithExtension ${ROOT} ".fa"
  !insertmacro OpenWithExtension ${ROOT} ".fna"
  !insertmacro OpenWithExtension ${ROOT} ".ffn"
  !insertmacro OpenWithExtension ${ROOT} ".faa"
  !insertmacro OpenWithExtension ${ROOT} ".frn"
  !insertmacro OpenWithExtension ${ROOT} ".fsa"
!macroend
!macro OpenWithExtension ROOT EXT
  WriteRegStr ${ROOT} "${OPENWITH_APP}\SupportedTypes" "${EXT}" ""
  WriteRegStr ${ROOT} "Software\Classes\${EXT}\OpenWithProgids" "Kleiotide.Test.${APP_VERSION}.${TEST_REVISION}" ""
!macroend
!macro UnregisterOpenWith ROOT
  ; Do not remove another side-by-side version's shared application entry.
  ReadRegStr $0 ${ROOT} "${OPENWITH_APP}\shell\open\command" ""
  ${If} $0 == '$\"$INSTDIR\kleiotide.exe$\" $\"%1$\"'
    DeleteRegKey ${ROOT} "${OPENWITH_APP}"
  ${EndIf}
  DeleteRegKey ${ROOT} "${OPENWITH_PROG}"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".ab1"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".abi"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".kleio"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".fasta"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".fas"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".fa"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".fna"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".ffn"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".faa"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".frn"
  !insertmacro RemoveOpenWithExtension ${ROOT} ".fsa"
!macroend
!macro RemoveOpenWithExtension ROOT EXT
  DeleteRegValue ${ROOT} "Software\Classes\${EXT}\OpenWithProgids" "Kleiotide.Test.${APP_VERSION}.${TEST_REVISION}"
!macroend

Section "Kleiotide" SEC_MAIN
  ; The original Microsoft installer performs runtime version detection.
  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  File /oname=vc_redist.x64.exe "${VC_REDIST_FILE}"
  DetailPrint "Installing/checking the Microsoft Visual C++ x64 runtime..."
  StrCpy $0 -1
  ClearErrors
  ExecWait '"$PLUGINSDIR\vc_redist.x64.exe" /install /quiet /norestart' $0
  IfErrors 0 runtime_started
    MessageBox MB_OK|MB_ICONSTOP "The Microsoft runtime installer could not be started. Kleiotide was not installed."
    SetErrorLevel 1
    Abort
  runtime_started:
  ${If} $0 == 3010
    SetRebootFlag true
  ${ElseIf} $0 != 0
    ${If} $0 != 1638
      MessageBox MB_OK|MB_ICONSTOP "Microsoft runtime installation failed (code $0). Kleiotide was not installed."
      SetErrorLevel $0
      Abort
    ${EndIf}
  ${EndIf}
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD_DIR}\*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  !insertmacro RegisterOpenWith HKLM
  ; Refresh a stale per-user Applications entry left by a previous test build.
  !insertmacro RegisterOpenWith HKCU
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  CreateDirectory "$SMPROGRAMS\${MENU_DIR}"
  CreateShortcut "$SMPROGRAMS\${MENU_DIR}\Kleiotide.lnk" "$INSTDIR\kleiotide.exe"
  CreateShortcut "$SMPROGRAMS\${MENU_DIR}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${APP_KEY}" "DisplayName" "Kleiotide ${APP_VERSION} test ${TEST_REVISION}"
  WriteRegStr HKLM "${APP_KEY}" "DisplayVersion" "${APP_VERSION} test ${TEST_REVISION}"
  WriteRegStr HKLM "${APP_KEY}" "Publisher" "Oleg Shchepin"
  WriteRegStr HKLM "${APP_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${APP_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr HKLM "${APP_KEY}" "DisplayIcon" "$INSTDIR\kleiotide.exe"
  WriteRegDWORD HKLM "${APP_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${APP_KEY}" "NoRepair" 1
SectionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
  StrCmp $INSTDIR "$PROGRAMFILES64\Kleiotide Test ${APP_VERSION}-${TEST_REVISION}" correct_location
    MessageBox MB_OK|MB_ICONSTOP "Run this uninstaller from its original Kleiotide installation folder."
    Abort
  correct_location:
FunctionEnd

Section "Uninstall"
  ; Refuse before removing anything else if the running program locks its EXE.
  ClearErrors
  IfFileExists "$INSTDIR\kleiotide.exe" 0 remove_payload
  Delete "$INSTDIR\kleiotide.exe"
  IfErrors 0 remove_payload
    MessageBox MB_OK|MB_ICONSTOP "Close Kleiotide and try uninstalling again."
    Abort
  remove_payload:
  ; Generated exact paths; no recursive deletion of user-created files.
  !include "${DELETE_LIST}"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\${MENU_DIR}\Kleiotide.lnk"
  Delete "$SMPROGRAMS\${MENU_DIR}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${MENU_DIR}"
  !insertmacro UnregisterOpenWith HKLM
  !insertmacro UnregisterOpenWith HKCU
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  DeleteRegKey HKLM "${APP_KEY}"
  ; The shared Microsoft runtime, user data and application settings are retained.
SectionEnd
