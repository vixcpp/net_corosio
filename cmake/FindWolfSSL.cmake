# Provides:
#   WolfSSL::WolfSSL
#
# Variables:
#   WolfSSL_FOUND
#   WOLFSSL_INCLUDE_DIR
#   WOLFSSL_LIBRARY

include(FindPackageHandleStandardArgs)

set(_WOLFSSL_HINTS "")
if (DEFINED WOLFSSL_ROOT)
  list(APPEND _WOLFSSL_HINTS "${WOLFSSL_ROOT}")
endif()

# Try pkg-config first (common on Linux)
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_WOLFSSL QUIET wolfssl)
endif()

find_path(WOLFSSL_INCLUDE_DIR
  NAMES wolfssl/ssl.h
  HINTS
    ${_WOLFSSL_HINTS}
    ${PC_WOLFSSL_INCLUDEDIR}
    ${PC_WOLFSSL_INCLUDE_DIRS}
  PATH_SUFFIXES include
)

find_library(WOLFSSL_LIBRARY
  NAMES wolfssl libwolfssl
  HINTS
    ${_WOLFSSL_HINTS}
    ${PC_WOLFSSL_LIBDIR}
    ${PC_WOLFSSL_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
)

find_package_handle_standard_args(WolfSSL
  REQUIRED_VARS WOLFSSL_INCLUDE_DIR WOLFSSL_LIBRARY
)

if (WolfSSL_FOUND AND NOT TARGET WolfSSL::WolfSSL)
  add_library(WolfSSL::WolfSSL UNKNOWN IMPORTED)
  set_target_properties(WolfSSL::WolfSSL PROPERTIES
    IMPORTED_LOCATION "${WOLFSSL_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${WOLFSSL_INCLUDE_DIR}"
  )
endif()
