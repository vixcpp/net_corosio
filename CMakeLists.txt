# ====================================================================
# Vix.cpp - net_corosio module
# - Standalone: find_package(net_corosio) works (net_corosioTargets + net_corosioConfig.cmake)
# - Umbrella  : contributes to VixTargets only (umbrella owns packaging)
# ====================================================================

# Standalone vs Umbrella
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  project(vix_net_corosio VERSION 0.1.0 LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include(GNUInstallDirs)

# Options + helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(NetCorosioOptions)
include(NetCorosioWarnings)
include(NetCorosioSanitizers)

# ----------------------------------------------------
# Corosio policy
# ----------------------------------------------------
function(vix_net_corosio_link_corosio onto link_scope)
  # Umbrella mode
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    if (TARGET vix::thirdparty_corosio)
      target_link_libraries(${onto} ${link_scope} vix::thirdparty_corosio)
      target_compile_definitions(${onto} ${link_scope} VIX_NET_COROSIO_WITH_COROSIO=1)
      return()
    endif()
    message(FATAL_ERROR "[net_corosio] Umbrella build: vix::thirdparty_corosio must be provided by umbrella.")
  endif()

  # Standalone mode: FetchContent
  include(FetchContent)

  option(NET_COROSIO_FETCH_DEPS "Fetch Corosio dependencies (recommended)" ON)

  FetchContent_Declare(
    corosio
    GIT_REPOSITORY https://github.com/cppalliance/corosio.git
    GIT_TAG develop
    GIT_SHALLOW TRUE
  )

  if (NET_COROSIO_FETCH_DEPS)
    FetchContent_MakeAvailable(corosio)
  else()
    FetchContent_Populate(corosio)
    add_subdirectory(${corosio_SOURCE_DIR} ${corosio_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()

  if (NOT TARGET Boost::corosio)
    message(FATAL_ERROR
      "[net_corosio] Corosio target not found after FetchContent.\n"
      "Expected target: Boost::corosio\n"
    )
  endif()

  target_link_libraries(${onto} ${link_scope} Boost::corosio)
  target_compile_definitions(${onto} ${link_scope} VIX_NET_COROSIO_WITH_COROSIO=1)

  # Link the correct Corosio TLS backend (this fixes undefined wolfssl_stream symbols)
  if (NET_COROSIO_TLS_WOLFSSL)
    if (TARGET Boost::corosio_wolfssl)
      target_link_libraries(${onto} ${link_scope} Boost::corosio_wolfssl)
      message(STATUS "[net_corosio] Corosio TLS backend: Boost::corosio_wolfssl")
    else()
      message(FATAL_ERROR
        "[net_corosio] Corosio wolfSSL backend not built.\n"
        "Expected target: Boost::corosio_wolfssl\n"
        "Fix: ensure find_package(WolfSSL) succeeds for Corosio (provide FindWolfSSL.cmake in this module).\n"
      )
    endif()
  else()
    if (TARGET Boost::corosio_openssl)
      target_link_libraries(${onto} ${link_scope} Boost::corosio_openssl)
      message(STATUS "[net_corosio] Corosio TLS backend: Boost::corosio_openssl")
    else()
      message(STATUS "[net_corosio] Corosio OpenSSL backend target not found (Boost::corosio_openssl). Continuing with Boost::corosio only.")
    endif()
  endif()

  message(STATUS "[net_corosio] Corosio: using FetchContent (Boost::corosio)")
endfunction()

# ----------------------------------------------------
# Library target
# ----------------------------------------------------
add_library(vix_net_corosio)
add_library(vix::net_corosio ALIAS vix_net_corosio)

target_compile_features(vix_net_corosio PUBLIC cxx_std_20)

# Public headers live under include/vix/net_corosio/...
target_include_directories(vix_net_corosio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Sources + headers
file(GLOB_RECURSE VIX_NET_COROSIO_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE VIX_NET_COROSIO_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/vix/net_corosio/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/vix/net_corosio/*.h"
)

target_sources(vix_net_corosio
  PRIVATE
    ${VIX_NET_COROSIO_SOURCES}
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
      FILES ${VIX_NET_COROSIO_HEADERS}
)

# ----------------------------------------------------
# TLS backend selection (exclusive)
# ----------------------------------------------------
if (NET_COROSIO_TLS_WOLFSSL)
  find_package(WolfSSL REQUIRED)
  target_link_libraries(vix_net_corosio PUBLIC WolfSSL::WolfSSL)
  target_compile_definitions(vix_net_corosio PUBLIC VIX_NET_COROSIO_TLS_WOLFSSL=1)
else()
  find_package(OpenSSL REQUIRED)
  target_link_libraries(vix_net_corosio PUBLIC OpenSSL::SSL OpenSSL::Crypto)
  target_compile_definitions(vix_net_corosio PUBLIC VIX_NET_COROSIO_TLS_WOLFSSL=0)
endif()

# Corosio link (policy)
vix_net_corosio_link_corosio(vix_net_corosio PUBLIC)

# Warnings / sanitizers (module local)
net_corosio_apply_warnings(vix_net_corosio)
net_corosio_apply_sanitizers(vix_net_corosio)

set_target_properties(vix_net_corosio PROPERTIES
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME vix_net_corosio
  EXPORT_NAME net_corosio
)

# ----------------------------------------------------
# Tests / Examples
# ----------------------------------------------------
include(CTest)

if (NET_COROSIO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if (NET_COROSIO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Bench
if (NET_COROSIO_BUILD_BENCH)
  add_subdirectory(bench)
endif()

# ----------------------------------------------------
# Install + export
# ----------------------------------------------------
set(_NET_COROSIO_EXPORT_SET "net_corosioTargets")
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(_NET_COROSIO_EXPORT_SET "VixTargets")
endif()

if (NET_COROSIO_INSTALL OR (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  install(
    TARGETS vix_net_corosio
    EXPORT ${_NET_COROSIO_EXPORT_SET}
    FILE_SET HEADERS
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Standalone package exports (only when not built via umbrella)
if (NET_COROSIO_INSTALL AND NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  install(
    EXPORT net_corosioTargets
    NAMESPACE vix::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/net_corosio
  )

  include(CMakePackageConfigHelpers)

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/net_corosioConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/net_corosioConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/net_corosioConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/net_corosio"
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/net_corosioConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/net_corosioConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/net_corosio
  )
endif()
