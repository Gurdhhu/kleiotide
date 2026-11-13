; GPL-3.0-only; copyright 2026 Oleg Shchepin.
; Build through make-installer.ps1. All definitions are supplied by it.
Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "nsDialogs.nsh"
!ifdef RELEASE_INSTALLER
  !define APP_NAME "Kleiotide"
  !define DISPLAY_VERSION "${APP_VERSION}"
  !define CAPABILITIES "Software\Kleiotide\Capabilities"
  !define UNINSTALL_HOME "$PROGRAMFILES64\Kleiotide Setup"
  !define OPENWITH_PROG "Kleiotide.Document"
  !define UNINSTALL_ID "Kleiotide"
  Var ExistingDir
  Var Sessions
  Var Traces
  Var Fasta
  Var SessionsBox
  Var TracesBox
  Var FastaBox
  Var UpgradeUninstall
!else
  !define APP_NAME "Kleiotide Test ${APP_VERSION}-${TEST_REVISION}"
  !define DISPLAY_VERSION "${APP_VERSION} test ${TEST_REVISION}"
  !define OPENWITH_PROG "Kleiotide.Test.${APP_VERSION}.${TEST_REVISION}"
  !define UNINSTALL_ID "Kleiotide-Test-${APP_VERSION}-${TEST_REVISION}"
!endif
!define APP_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_ID}"
Name "${APP_NAME}"
OutFile "${SETUP_FILE}"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
RequestExecutionLevel admin
ManifestDPIAware true
SetCompressor /SOLID zlib
ShowInstDetails show
ShowUninstDetails show
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_REBOOTLATER_DEFAULT
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PAYLOAD_DIR}\LICENSE"
!ifdef RELEASE_INSTALLER
  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE VerifyDestination
  !insertmacro MUI_PAGE_DIRECTORY
  Page custom AssociationPage SaveAssociations
!endif
!insertmacro MUI_PAGE_INSTFILES
!ifdef RELEASE_INSTALLER
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishOptions
!endif
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "Kleiotide requires Windows 10 or Windows 11."
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
!ifdef RELEASE_INSTALLER
  ReadRegStr $ExistingDir HKLM "${APP_KEY}" "InstallLocation"
  StrCpy $Sessions ${BST_CHECKED}
  StrCpy $Traces ${BST_UNCHECKED}
  StrCpy $Fasta ${BST_UNCHECKED}
  ${If} $ExistingDir != ""
    StrCpy $INSTDIR $ExistingDir
    ReadRegDWORD $Sessions HKLM "${APP_KEY}" "Sessions"
    ReadRegDWORD $Traces HKLM "${APP_KEY}" "Traces"
    ReadRegDWORD $Fasta HKLM "${APP_KEY}" "Fasta"
  ${EndIf}
!else
  StrCpy $INSTDIR "$PROGRAMFILES64\${APP_NAME}"
  Call VerifyDestination
!endif
!ifdef RELEASE_INSTALLER
  IfSilent 0 init_done
  Call VerifyDestination
  init_done:
!endif
FunctionEnd

Function VerifyDestination
  ; The NSIS path instruction requires the final component to exist.
  StrCmp $INSTDIR "" destination_invalid
  System::Call 'kernel32::GetFullPathNameW(w "$INSTDIR", i ${NSIS_MAX_STRLEN}, w .r0, p 0) i .r1'
  IntCmp $1 0 destination_invalid
  IntCmp $1 ${NSIS_MAX_STRLEN} destination_invalid +1 destination_invalid
  StrCpy $INSTDIR $0
!ifdef RELEASE_INSTALLER
  ${If} $ExistingDir != ""
    ${If} $INSTDIR != $ExistingDir
      MessageBox MB_OK|MB_ICONSTOP "Use the existing Kleiotide folder for this upgrade. To move it, uninstall Kleiotide first." /SD IDOK
      SetErrorLevel 1
      Abort
    ${EndIf}
    IfFileExists "${UNINSTALL_HOME}\Uninstall.exe" destination_ok
    MessageBox MB_OK|MB_ICONSTOP "The existing Kleiotide uninstaller is missing. Repair or remove the existing installation first." /SD IDOK
    SetErrorLevel 1
    Abort
  ${EndIf}
!endif
!ifdef RELEASE_INSTALLER
  StrLen $0 "${UNINSTALL_HOME}\"
  StrCpy $1 "$INSTDIR\" $0
  StrCmp $1 "${UNINSTALL_HOME}\" destination_invalid
!endif
  ; A fresh installation must not overwrite another package or a user's files.
  System::Call 'kernel32::GetFileAttributesW(w "$INSTDIR") i .r0 ?e'
  Pop $1
  ${If} $0 == -1
    ; ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND means setup can create it.
    StrCmp $1 2 destination_ok
    StrCmp $1 3 destination_ok destination_invalid
  ${EndIf}
  IntOp $0 $0 & 0x10 ; FILE_ATTRIBUTE_DIRECTORY
  IntCmp $0 0 destination_invalid
  ClearErrors
  FindFirst $0 $1 "$INSTDIR\*"
  IfErrors destination_invalid
  check_entry:
  StrCmp $1 "." next_entry
  StrCmp $1 ".." next_entry
  FindClose $0
  Goto destination_invalid
  next_entry:
  ClearErrors
  FindNext $0 $1
  IfErrors 0 check_entry
  FindClose $0
  Goto destination_ok
  destination_invalid:
  MessageBox MB_OK|MB_ICONSTOP "Choose an empty folder or a new Kleiotide folder." /SD IDOK
  SetErrorLevel 1
  Abort
  destination_ok:
  ClearErrors
  SetErrorLevel 0
FunctionEnd

!ifdef RELEASE_INSTALLER
Function AssociationPage
  !insertmacro MUI_HEADER_TEXT "File associations" "Choose the file types to register with Windows."
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}
  ${NSD_CreateLabel} 0 0 100% 28u "Choose which files Kleiotide should appear for in Open with and Default apps. You can choose the default application in Windows Settings."
  Pop $0
  ${NSD_CreateCheckbox} 0 35u 100% 13u "Kleiotide sessions (.kleio)"
  Pop $SessionsBox
  ${NSD_SetState} $SessionsBox $Sessions
  ${NSD_CreateCheckbox} 0 55u 100% 13u "Sanger chromatograms (.ab1, .abi)"
  Pop $TracesBox
  ${NSD_SetState} $TracesBox $Traces
  ${NSD_CreateCheckbox} 0 75u 100% 13u "FASTA sequence files"
  Pop $FastaBox
  ${NSD_SetState} $FastaBox $Fasta
  nsDialogs::Show
FunctionEnd

Function SaveAssociations
  ${NSD_GetState} $SessionsBox $Sessions
  ${NSD_GetState} $TracesBox $Traces
  ${NSD_GetState} $FastaBox $Fasta
FunctionEnd

Function FinishOptions
  ${If} $Sessions == ${BST_CHECKED}
  ${OrIf} $Traces == ${BST_CHECKED}
  ${OrIf} $Fasta == ${BST_CHECKED}
    ${NSD_CreateButton} 120u 154u 158u 16u "Open Windows Default Apps"
    Pop $0
    ${NSD_OnClick} $0 OpenDefaultApps
  ${EndIf}
FunctionEnd

Function OpenDefaultApps
  Pop $0
  ExecShell "open" "ms-settings:defaultapps"
FunctionEnd
!endif

; The application owns its ProgID and advertised capabilities. Windows owns UserChoice.
!macro AddExtension EXT
  WriteRegStr HKLM "Software\Classes\${EXT}\OpenWithProgids" "${OPENWITH_PROG}" ""
!ifdef RELEASE_INSTALLER
  WriteRegStr HKLM "${CAPABILITIES}\FileAssociations" "${EXT}" "${OPENWITH_PROG}"
!endif
!macroend
!macro RemoveExtension EXT
  DeleteRegValue HKLM "Software\Classes\${EXT}\OpenWithProgids" "${OPENWITH_PROG}"
!ifdef RELEASE_INSTALLER
  DeleteRegValue HKLM "${CAPABILITIES}\FileAssociations" "${EXT}"
!endif
!macroend
!macro ApplyExtension EXT FLAG
!ifdef RELEASE_INSTALLER
  ${If} ${FLAG} == ${BST_CHECKED}
    !insertmacro AddExtension "${EXT}"
  ${Else}
    !insertmacro RemoveExtension "${EXT}"
  ${EndIf}
!else
  !insertmacro AddExtension "${EXT}"
!endif
!macroend

!macro RegisterOpenWith
  WriteRegStr HKLM "Software\Classes\${OPENWITH_PROG}" "" "${APP_NAME} document"
  WriteRegStr HKLM "Software\Classes\${OPENWITH_PROG}\shell\open\command" "" '$\"$INSTDIR\kleiotide.exe$\" $\"%1$\"'
  WriteRegStr HKLM "Software\Classes\${OPENWITH_PROG}\shell\open" "MultiSelectModel" "Player"
!ifdef RELEASE_INSTALLER
  WriteRegStr HKLM "${CAPABILITIES}" "ApplicationName" "Kleiotide"
  WriteRegStr HKLM "${CAPABILITIES}" "ApplicationDescription" "Sequence alignment and chromatogram editor"
  ${If} $Sessions == ${BST_CHECKED}
  ${OrIf} $Traces == ${BST_CHECKED}
  ${OrIf} $Fasta == ${BST_CHECKED}
    WriteRegStr HKLM "Software\RegisteredApplications" "Kleiotide" "${CAPABILITIES}"
  ${Else}
    DeleteRegValue HKLM "Software\RegisteredApplications" "Kleiotide"
  ${EndIf}
!endif
  !insertmacro ApplyExtension ".kleio" "$Sessions"
  !insertmacro ApplyExtension ".ab1" "$Traces"
  !insertmacro ApplyExtension ".abi" "$Traces"
  !insertmacro ApplyExtension ".fasta" "$Fasta"
  !insertmacro ApplyExtension ".fas" "$Fasta"
  !insertmacro ApplyExtension ".fa" "$Fasta"
  !insertmacro ApplyExtension ".fna" "$Fasta"
  !insertmacro ApplyExtension ".ffn" "$Fasta"
  !insertmacro ApplyExtension ".faa" "$Fasta"
  !insertmacro ApplyExtension ".frn" "$Fasta"
  !insertmacro ApplyExtension ".fsa" "$Fasta"
  !insertmacro ApplyExtension ".mpfa" "$Fasta"
  !insertmacro ApplyExtension ".fnt" "$Fasta"
  !insertmacro ApplyExtension ".fsa_nt" "$Fasta"
  !insertmacro ApplyExtension ".fsa_aa" "$Fasta"
!macroend
!macro UnregisterOpenWith
  DeleteRegKey HKLM "Software\Classes\${OPENWITH_PROG}"
  !insertmacro RemoveExtension ".kleio"
  !insertmacro RemoveExtension ".ab1"
  !insertmacro RemoveExtension ".abi"
  !insertmacro RemoveExtension ".fasta"
  !insertmacro RemoveExtension ".fas"
  !insertmacro RemoveExtension ".fa"
  !insertmacro RemoveExtension ".fna"
  !insertmacro RemoveExtension ".ffn"
  !insertmacro RemoveExtension ".faa"
  !insertmacro RemoveExtension ".frn"
  !insertmacro RemoveExtension ".fsa"
  !insertmacro RemoveExtension ".mpfa"
  !insertmacro RemoveExtension ".fnt"
  !insertmacro RemoveExtension ".fsa_nt"
  !insertmacro RemoveExtension ".fsa_aa"
!ifdef RELEASE_INSTALLER
  DeleteRegValue HKLM "Software\RegisteredApplications" "Kleiotide"
  DeleteRegKey HKLM "${CAPABILITIES}"
!endif
!macroend

Section "Kleiotide" SEC_MAIN
  Call VerifyDestination
  ; The original Microsoft installer performs runtime version detection.
  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  File /oname=vc_redist.x64.exe "${VC_REDIST_FILE}"
  DetailPrint "Installing/checking the Microsoft Visual C++ x64 runtime..."
  StrCpy $0 -1
  ClearErrors
  ExecWait '"$PLUGINSDIR\vc_redist.x64.exe" /install /quiet /norestart' $0
  IfErrors 0 runtime_started
    MessageBox MB_OK|MB_ICONSTOP "The Microsoft runtime installer could not be started. Kleiotide was not installed." /SD IDOK
    SetErrorLevel 1
    Abort
  runtime_started:
  ${If} $0 == 3010
    SetRebootFlag true
  ${ElseIf} $0 != 0
    ${If} $0 != 1638
      MessageBox MB_OK|MB_ICONSTOP "Microsoft runtime installation failed (code $0). Kleiotide was not installed." /SD IDOK
      SetErrorLevel $0
      Abort
    ${EndIf}
  ${EndIf}
!ifdef RELEASE_INSTALLER
  ${If} $ExistingDir != ""
    ; The previous uninstaller runs from the protected setup directory.
    ClearErrors
    ExecWait '"${UNINSTALL_HOME}\Uninstall.exe" /S /KleiotideUpgrade _?=${UNINSTALL_HOME}' $0
    IfErrors upgrade_failed
    ${If} $0 != 0
      Goto upgrade_failed
    ${EndIf}
    Goto upgrade_done
    upgrade_failed:
    MessageBox MB_OK|MB_ICONSTOP "The previous version could not be removed. Close Kleiotide and try again." /SD IDOK
    SetErrorLevel 1
    Abort
    upgrade_done:
  ${EndIf}
!endif
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD_DIR}\*"
!ifdef RELEASE_INSTALLER
  CreateDirectory "${UNINSTALL_HOME}"
  WriteUninstaller "${UNINSTALL_HOME}\Uninstall.exe"
!else
  WriteUninstaller "$INSTDIR\Uninstall.exe"
!endif
  !insertmacro RegisterOpenWith
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Kleiotide.lnk" "$INSTDIR\kleiotide.exe"

  WriteRegStr HKLM "${APP_KEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "${APP_KEY}" "DisplayVersion" "${DISPLAY_VERSION}"
  WriteRegStr HKLM "${APP_KEY}" "Publisher" "Oleg Shchepin"
  WriteRegStr HKLM "${APP_KEY}" "InstallLocation" "$INSTDIR"
!ifdef RELEASE_INSTALLER
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "${UNINSTALL_HOME}\Uninstall.exe"
  WriteRegStr HKLM "${APP_KEY}" "UninstallString" '$\"${UNINSTALL_HOME}\Uninstall.exe$\"'
!else
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${APP_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
!endif
  WriteRegStr HKLM "${APP_KEY}" "DisplayIcon" "$INSTDIR\kleiotide.exe"
!ifdef RELEASE_INSTALLER
  WriteRegDWORD HKLM "${APP_KEY}" "Sessions" $Sessions
  WriteRegDWORD HKLM "${APP_KEY}" "Traces" $Traces
  WriteRegDWORD HKLM "${APP_KEY}" "Fasta" $Fasta
!endif
  WriteRegDWORD HKLM "${APP_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${APP_KEY}" "NoRepair" 1
SectionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
  ReadRegStr $0 HKLM "${APP_KEY}" "InstallLocation"
!ifdef RELEASE_INSTALLER
  StrCmp $0 "" incorrect_location
  StrCmp $INSTDIR "${UNINSTALL_HOME}" correct_location
  incorrect_location:
!else
  StrCmp $INSTDIR $0 correct_location
!endif
  MessageBox MB_OK|MB_ICONSTOP "Run this uninstaller from its registered Kleiotide installation folder." /SD IDOK
  SetErrorLevel 1
  Abort
  correct_location:
!ifdef RELEASE_INSTALLER
  StrCpy $INSTDIR $0
  ${GetParameters} $0
  ClearErrors
  ${GetOptions} "$0" "/KleiotideUpgrade" $1
  ${IfNot} ${Errors}
    StrCpy $UpgradeUninstall 1
  ${EndIf}
!endif
FunctionEnd

Section "Uninstall"
  ; Refuse before removing anything else if the running program locks its EXE.
  ClearErrors
  IfFileExists "$INSTDIR\kleiotide.exe" 0 remove_payload
  Delete "$INSTDIR\kleiotide.exe"
  IfErrors 0 remove_payload
    MessageBox MB_OK|MB_ICONSTOP "Close Kleiotide and try uninstalling again." /SD IDOK
    SetErrorLevel 1
    Abort
  remove_payload:
  ; Generated exact paths; no recursive deletion of user-created files.
  !include "${DELETE_LIST}"
!ifndef RELEASE_INSTALLER
  Delete "$INSTDIR\Uninstall.exe"
!endif
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\${APP_NAME}\Kleiotide.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
!ifdef RELEASE_INSTALLER
  ${If} $UpgradeUninstall != 1
    Delete "${UNINSTALL_HOME}\Uninstall.exe"
    RMDir "${UNINSTALL_HOME}"
!endif
    !insertmacro UnregisterOpenWith
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    DeleteRegKey HKLM "${APP_KEY}"
!ifdef RELEASE_INSTALLER
  ${EndIf}
!endif
  ; The shared Microsoft runtime, user data and application settings are retained.
SectionEnd
