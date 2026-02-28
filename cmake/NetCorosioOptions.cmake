# ============================================================
# net_corosio - Options
# ============================================================

option(NET_COROSIO_BUILD_TESTS "Build net_corosio tests" ON)
option(NET_COROSIO_BUILD_EXAMPLES "Build net_corosio examples" ON)

# Standalone helpers
option(NET_COROSIO_INSTALL "Enable install/export rules (standalone)" ON)

# Corosio fetch policy (standalone mode only)
option(NET_COROSIO_FETCH_DEPS "Fetch Corosio via FetchContent in standalone mode" ON)
