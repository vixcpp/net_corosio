# ============================================================
# net_corosio - Options
# ============================================================

# Detect umbrella build
set(_NET_COROSIO_IS_UMBRELLA OFF)
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(_NET_COROSIO_IS_UMBRELLA ON)
endif()

# ------------------------------------------------------------
# Development features (OFF by default for clean packaging)
# ------------------------------------------------------------

option(NET_COROSIO_BUILD_TESTS "Build net_corosio tests" OFF)
option(NET_COROSIO_BUILD_EXAMPLES "Build net_corosio examples" OFF)
option(NET_COROSIO_BUILD_BENCH "Build net_corosio benchmarks" OFF)
option(NET_COROSIO_TLS_WOLFSSL "Use wolfSSL TLS backend (instead of generic/OpenSSL)" OFF)

# ------------------------------------------------------------
# Corosio fetch policy (standalone mode only)
# ------------------------------------------------------------

option(NET_COROSIO_FETCH_DEPS
  "Fetch Corosio via FetchContent in standalone mode"
  ON
)

# ------------------------------------------------------------
# Install / Export (standalone only)
# ------------------------------------------------------------

option(NET_COROSIO_INSTALL
  "Enable install/export rules (standalone)"
  ON
)

# ------------------------------------------------------------
# Safety rules
# ------------------------------------------------------------

# If we are in umbrella mode, disable install + dev extras
if (_NET_COROSIO_IS_UMBRELLA)
  set(NET_COROSIO_INSTALL OFF CACHE BOOL "" FORCE)
  set(NET_COROSIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(NET_COROSIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(NET_COROSIO_BUILD_BENCH OFF CACHE BOOL "" FORCE)
endif()

# If we FetchContent deps, packaging is unsafe
if (NET_COROSIO_FETCH_DEPS AND NET_COROSIO_INSTALL)
  message(STATUS
    "[net_corosio] NET_COROSIO_FETCH_DEPS=ON → packaging disabled (NET_COROSIO_INSTALL=OFF)."
  )
  set(NET_COROSIO_INSTALL OFF CACHE BOOL "" FORCE)
endif()
