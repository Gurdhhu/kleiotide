# Install into an ordinary Debian filesystem layout. Qt is supplied by apt.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Configure the Debian package on Debian 13.")
endif()

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_PACKAGE_NAME kleiotide)
set(CPACK_PACKAGE_CONTACT "Oleg Shchepin <oleg.shchepin@uni-greifswald.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Nucleotide alignment and chromatogram editor")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Gurdhhu/kleiotide")
set(CPACK_DEBIAN_PACKAGE_SECTION science)
set(CPACK_DEBIAN_PACKAGE_RELEASE "1~deb13")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# The Debian 13 Qt runtime is also required when the build kit was extracted
# into a prefix whose libraries dpkg-shlibdeps cannot associate with packages.
string(JOIN ", " CPACK_DEBIAN_PACKAGE_DEPENDS
  "libqt6core6t64 (>= 6.8.2)" "libqt6gui6 (>= 6.8.2)"
  "libqt6widgets6 (>= 6.8.2)" "libqt6network6 (>= 6.8.2)"
  "qt6-qpa-plugins" "qt6-wayland" "ca-certificates" "shared-mime-info" "desktop-file-utils")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "mafft, ncbi-blast+")
include(CPack)
