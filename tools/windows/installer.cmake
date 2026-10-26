# GPL-3.0-only; copyright 2026 Oleg Shchepin.
# Packaging only; no changes to application compilation or normal build targets.
if(WIN32 AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KLEIOTIDE_SOURCE_ARCHIVE "" CACHE FILEPATH "The complete Kleiotide source ZIP extracted for this build")
  set(KLEIOTIDE_QT_SOURCE_ARCHIVE "" CACHE FILEPATH "Official qt-everywhere-src matching this Qt kit, ZIP format")
  set(KLEIOTIDE_VC_REDIST "" CACHE FILEPATH "Original Microsoft-signed vc_redist.x64.exe")
  set(KLEIOTIDE_NSIS_COMPILER "" CACHE FILEPATH "Optional makensis.exe override; standard NSIS installation is detected")
  set(KLEIOTIDE_INSTALLER_REVISION "1" CACHE STRING "Test packaging revision; increase to create a fresh output")
  find_program(KLEIOTIDE_POWERSHELL powershell.exe REQUIRED)
  set(nsis_arguments)
  if(KLEIOTIDE_NSIS_COMPILER)
    list(APPEND nsis_arguments -NsisCompiler "${KLEIOTIDE_NSIS_COMPILER}")
  endif()
  add_custom_target(test-installer
    COMMAND "${KLEIOTIDE_POWERSHELL}" -NoProfile -ExecutionPolicy RemoteSigned
        -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/make-installer.ps1"
        -QtRoot "$<TARGET_FILE_DIR:Qt6::Core>/.."
        -BuildDir "${CMAKE_CURRENT_BINARY_DIR}"
        -SourceArchive "${KLEIOTIDE_SOURCE_ARCHIVE}"
        -QtSourceArchive "${KLEIOTIDE_QT_SOURCE_ARCHIVE}"
        -VcRedist "${KLEIOTIDE_VC_REDIST}"
        -CmakeExecutable "${CMAKE_COMMAND}"
        ${nsis_arguments}
        -TestRevision "${KLEIOTIDE_INSTALLER_REVISION}"
    DEPENDS kleiotide
    USES_TERMINAL VERBATIM)
  if(TARGET release-check)
    add_dependencies(test-installer release-check)
  endif()
endif()
