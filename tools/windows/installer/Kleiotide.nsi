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
  !define LEGACY_HOME "$PROGRAMFILES64\Kleiotide Setup"
  !define OPENWITH_PROG "Kleiotide.Document"
  !define UNINSTALL_ID "Kleiotide"
  Var ExistingVersion
  Var KeepSettings
  Var KeepSettingsBox
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
Var ConfigBox
Var RemoveMafftBox
Var RemoveBlastBox
Var RemoveConfig
Var RemoveMafft
Var RemoveBlast
Var InstallMafft
Var InstallBlast
Var DesktopLink
Var MafftBox
Var BlastBox
Var DesktopBox
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
  Page custom UpgradePage SaveUpgradeChoice
  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE VerifyDestination
  !insertmacro MUI_PAGE_DIRECTORY
  Page custom AssociationPage SaveAssociations
!endif
Page custom ToolsPage SaveTools
!insertmacro MUI_PAGE_INSTFILES
!ifdef RELEASE_INSTALLER
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishOptions
!endif
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
UninstPage custom un.RemovalPage un.SaveRemoval
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
  StrCpy $InstallMafft ${BST_CHECKED}
  StrCpy $InstallBlast ${BST_CHECKED}
  StrCpy $DesktopLink ${BST_CHECKED}
  ReadRegDWORD $0 HKLM "${APP_KEY}" "DesktopShortcut"
  IfErrors +2
    StrCpy $DesktopLink $0
  ReadRegDWORD $0 HKLM "${APP_KEY}" "InstallMAFFT"
  IfErrors +2
    StrCpy $InstallMafft $0
  ReadRegDWORD $0 HKLM "${APP_KEY}" "InstallBLAST"
  IfErrors +2
    StrCpy $InstallBlast $0
  IfSilent 0 +3
    StrCpy $InstallMafft ${BST_UNCHECKED}
    StrCpy $InstallBlast ${BST_UNCHECKED}
  ClearErrors
!ifdef RELEASE_INSTALLER
  StrCpy $KeepSettings ${BST_CHECKED}
  ReadRegStr $ExistingDir HKLM "${APP_KEY}" "InstallLocation"
  ReadRegStr $ExistingVersion HKLM "${APP_KEY}" "DisplayVersion"
  StrCpy $Sessions ${BST_CHECKED}
  StrCpy $Traces ${BST_CHECKED}
  StrCpy $Fasta ${BST_CHECKED}
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

!ifdef RELEASE_INSTALLER
Function UpgradePage
  ${If} $ExistingDir == ""
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Replace Kleiotide?" "Choose what setup should do with your existing installation."
  nsDialogs::Create 1018
  Pop $0
  StrCmp $0 error 0 +2
    Abort
  ${If} $ExistingVersion == ""
    StrCpy $ExistingVersion "(version unavailable)"
  ${EndIf}
  ${NSD_CreateLabel} 0 0 100% 36u "Kleiotide $ExistingVersion is installed. Click Replace to continue installing Kleiotide ${DISPLAY_VERSION}, or Cancel to keep the installed version."
  Pop $0
  ReadEnvStr $0 USERNAME
  ${NSD_CreateCheckbox} 0 43u 100% 24u "Keep Kleiotide settings for Windows account $0"
  Pop $KeepSettingsBox
  ${NSD_SetState} $KeepSettingsBox $KeepSettings
  GetDlgItem $0 $HWNDPARENT 1
  SendMessage $0 ${WM_SETTEXT} 0 "STR:Replace"
  nsDialogs::Show
FunctionEnd

Function SaveUpgradeChoice
  ${NSD_GetState} $KeepSettingsBox $KeepSettings
FunctionEnd
!endif

Function ToolsPage
  !insertmacro MUI_HEADER_TEXT "Optional tools and shortcut" "Choose the features to set up on this computer."
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}
  ${NSD_CreateLabel} 0 0 100% 32u "Setup can download the tools from their publishers and configure Kleiotide for all users. If an existing installation is detected, it will be reused. You can also choose your own executables in Settings."
  Pop $0
  ${NSD_CreateCheckbox} 0 39u 100% 16u "Set up MAFFT for sequence alignment (about 27 MB)"
  Pop $MafftBox
  ${NSD_SetState} $MafftBox $InstallMafft
  ${NSD_CreateCheckbox} 0 61u 100% 16u "Set up NCBI BLAST+ for local database searches (about 133 MB)"
  Pop $BlastBox
  ${NSD_SetState} $BlastBox $InstallBlast
  ${NSD_CreateCheckbox} 0 90u 100% 16u "Create a desktop shortcut for Kleiotide"
  Pop $DesktopBox
  ${NSD_SetState} $DesktopBox $DesktopLink
  nsDialogs::Show
FunctionEnd

Function SaveTools
  ${NSD_GetState} $MafftBox $InstallMafft
  ${NSD_GetState} $BlastBox $InstallBlast
  ${NSD_GetState} $DesktopBox $DesktopLink
FunctionEnd

!macro SetupTool NAME FLAG
  ${If} ${FLAG} == ${BST_CHECKED}
    retry_${NAME}:
    DetailPrint "Setting up ${NAME}..."
    nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$PLUGINSDIR\Install-Dependency.ps1" -Tool ${NAME} -ProgramFiles64 "$PROGRAMFILES64"'
    Pop $0
    ${If} $0 != 0
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Could not set up ${NAME}. Check your internet connection and see the installation details. Retry, or choose Cancel to finish installing Kleiotide without this tool." /SD IDCANCEL IDRETRY retry_${NAME}
    ${EndIf}
  ${EndIf}
!macroend

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
    ReadRegStr $0 HKLM "${APP_KEY}" "UninstallerSHA256"
    ${If} $0 != ""
      IfFileExists "$INSTDIR\Uninstall.exe" destination_ok
    ${Else}
      IfFileExists "${LEGACY_HOME}\Uninstall.exe" destination_ok
    ${EndIf}
    MessageBox MB_OK|MB_ICONSTOP "The existing Kleiotide uninstaller is missing. Repair or remove the existing installation first." /SD IDOK
    SetErrorLevel 1
    Abort
  ${EndIf}
!endif
!ifdef RELEASE_INSTALLER
  StrLen $0 "${LEGACY_HOME}\"
  StrCpy $1 "$INSTDIR\" $0
  StrCmp $1 "${LEGACY_HOME}\" destination_invalid
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
!ifdef RELEASE_INSTALLER
  File "Uninstaller-Identity.ps1"
!endif
  File "Mafft-Files.ps1"
  File /oname=Install-Dependency.ps1 "Install-Dependency.ps1"
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
    ReadRegStr $1 HKLM "${APP_KEY}" "UninstallerSHA256"
    ${If} $1 != ""
      nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "$PLUGINSDIR\Uninstaller-Identity.ps1" Stage "${APP_KEY}" "$PLUGINSDIR\previous.exe" "$INSTDIR\Uninstall.exe"'
      Pop $0
      StrCmp $0 0 0 upgrade_failed
      ClearErrors
      ExecWait '"$PLUGINSDIR\previous.exe" /S /KleiotideUpgrade _?=$INSTDIR' $0
    ${Else}
      ClearErrors
      ExecWait '"${LEGACY_HOME}\Uninstall.exe" /S /KleiotideUpgrade _?=${LEGACY_HOME}' $0
    ${EndIf}
    IfErrors upgrade_failed
    ${If} $0 != 0
      Goto upgrade_failed
    ${EndIf}
    ${If} $1 == ""
      Delete "${LEGACY_HOME}\Uninstall.exe"
      RMDir "${LEGACY_HOME}"
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
  WriteUninstaller "$PLUGINSDIR\Uninstall.exe"
  nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "$PLUGINSDIR\Uninstaller-Identity.ps1" Record "${APP_KEY}" "$PLUGINSDIR\Uninstall.exe"'
  Pop $0
  StrCmp $0 0 +3
    MessageBox MB_ICONSTOP "Could not register the new uninstaller." /SD IDOK
    Abort
  ClearErrors
  CopyFiles /SILENT "$PLUGINSDIR\Uninstall.exe" "$INSTDIR"
  IfErrors 0 +3
    MessageBox MB_ICONSTOP "Could not install Uninstall.exe in the application folder." /SD IDOK
    Abort
!else
  WriteUninstaller "$INSTDIR\Uninstall.exe"
!endif
  !insertmacro RegisterOpenWith
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Kleiotide.lnk" "$INSTDIR\kleiotide.exe"

  ${If} $DesktopLink == ${BST_CHECKED}
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\kleiotide.exe"
  ${EndIf}
  WriteRegDWORD HKLM "${APP_KEY}" "DesktopShortcut" $DesktopLink
  WriteRegDWORD HKLM "${APP_KEY}" "InstallMAFFT" $InstallMafft
  WriteRegDWORD HKLM "${APP_KEY}" "InstallBLAST" $InstallBlast
  !insertmacro SetupTool MAFFT $InstallMafft
  !insertmacro SetupTool BLAST $InstallBlast
  WriteRegStr HKLM "${APP_KEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "${APP_KEY}" "DisplayVersion" "${DISPLAY_VERSION}"
  WriteRegStr HKLM "${APP_KEY}" "Publisher" "Oleg Shchepin"
  WriteRegStr HKLM "${APP_KEY}" "InstallLocation" "$INSTDIR"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${APP_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr HKLM "${APP_KEY}" "DisplayIcon" "$INSTDIR\kleiotide.exe"
!ifdef RELEASE_INSTALLER
  WriteRegDWORD HKLM "${APP_KEY}" "Sessions" $Sessions
  WriteRegDWORD HKLM "${APP_KEY}" "Traces" $Traces
  WriteRegDWORD HKLM "${APP_KEY}" "Fasta" $Fasta
!endif
  WriteRegDWORD HKLM "${APP_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${APP_KEY}" "NoRepair" 1
!ifdef RELEASE_INSTALLER
  ${If} $ExistingDir != ""
  ${AndIf} $KeepSettings != ${BST_CHECKED}
    DeleteRegKey HKCU "Software\Kleiotide\Kleiotide"
    DeleteRegKey /ifempty HKCU "Software\Kleiotide"
  ${EndIf}
!endif
SectionEnd

Function un.onInit
  StrCpy $RemoveConfig 0
  StrCpy $RemoveMafft 0
  StrCpy $RemoveBlast 0
  SetRegView 64
  SetShellVarContext all
  ReadRegStr $0 HKLM "${APP_KEY}" "InstallLocation"
  StrCmp $INSTDIR $0 correct_location
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

Function un.RemovalPage
  nsDialogs::Create 1018
  Pop $0
  StrCmp $0 error 0 +2
    Abort
  ${NSD_CreateLabel} 0 0 100% 30u "Optional cleanup. MAFFT and BLAST+ are shared with other installations and applications. Saved sessions, recovery copies and BLAST jobs are kept."
  Pop $0
  ReadEnvStr $0 USERNAME
  ${NSD_CreateCheckbox} 0 37u 100% 20u "Delete Kleiotide settings for the Windows account $0"
  Pop $ConfigBox
  ${NSD_SetState} $ConfigBox $RemoveConfig
  ${NSD_CreateCheckbox} 0 63u 100% 16u "Uninstall MAFFT installed by Kleiotide Setup"
  Pop $RemoveMafftBox
  ${NSD_SetState} $RemoveMafftBox $RemoveMafft
  ReadRegStr $0 HKLM "Software\Kleiotide\Tools" "OwnedMAFFT"
  StrCmp $0 "" 0 +2
    EnableWindow $RemoveMafftBox 0
  ${NSD_CreateCheckbox} 0 84u 100% 16u "Uninstall BLAST+ installed by Kleiotide Setup"
  Pop $RemoveBlastBox
  ${NSD_SetState} $RemoveBlastBox $RemoveBlast
  ReadRegStr $0 HKLM "Software\Kleiotide\Tools" "OwnedBLAST"
  StrCmp $0 "" 0 +2
    EnableWindow $RemoveBlastBox 0
  nsDialogs::Show
FunctionEnd

Function un.SaveRemoval
  ${NSD_GetState} $ConfigBox $RemoveConfig
  ${NSD_GetState} $RemoveMafftBox $RemoveMafft
  ${NSD_GetState} $RemoveBlastBox $RemoveBlast
FunctionEnd

!macro RemoveOptional NAME FLAG
  ${If} ${FLAG} == ${BST_CHECKED}
    retry_remove_${NAME}:
    nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "$PLUGINSDIR\Remove-Dependency.ps1" -Tool ${NAME} -ProgramFiles64 "$PROGRAMFILES64"'
    Pop $0
    ${If} $0 != 0
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Could not finish removing ${NAME}. Close any programs using it. Retry or choose Cancel to continue." /SD IDCANCEL IDRETRY retry_remove_${NAME}
    ${EndIf}
  ${EndIf}
!macroend

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
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  ReadRegDWORD $0 HKLM "${APP_KEY}" "DesktopShortcut"
  ${If} $0 == ${BST_CHECKED}
    Delete "$DESKTOP\${APP_NAME}.lnk"
  ${EndIf}
  Delete "$SMPROGRAMS\${APP_NAME}\Kleiotide.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
!ifdef RELEASE_INSTALLER
  ${If} $UpgradeUninstall != 1
!endif
    ${If} $RemoveConfig == ${BST_CHECKED}
      DeleteRegKey HKCU "Software\Kleiotide\Kleiotide"
      DeleteRegKey /ifempty HKCU "Software\Kleiotide"
    ${EndIf}
    InitPluginsDir
    SetOutPath "$PLUGINSDIR"
    File "Mafft-Files.ps1"
    File "Remove-Dependency.ps1"
    File "mafft-7.526-files.txt"
    !insertmacro RemoveOptional MAFFT $RemoveMafft
    !insertmacro RemoveOptional BLAST $RemoveBlast
    !insertmacro UnregisterOpenWith
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    DeleteRegKey HKLM "${APP_KEY}"
!ifdef RELEASE_INSTALLER
  ${EndIf}
!endif
  ; The shared Microsoft runtime, user data and application settings are retained.
SectionEnd
