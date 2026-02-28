# ============================================================
# net_corosio - Sanitizers (ASan/UBSan)
# ============================================================

option(NET_COROSIO_SANITIZERS "Enable sanitizers for net_corosio targets" OFF)
option(NET_COROSIO_ASAN "Enable AddressSanitizer" OFF)
option(NET_COROSIO_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

function(net_corosio_apply_sanitizers target_name)
  if (NOT TARGET ${target_name})
    message(FATAL_ERROR "[net_corosio] net_corosio_apply_sanitizers: target not found: ${target_name}")
  endif()

  if (MSVC)
    # MSVC sanitizers exist, but behavior differs. Keep off by default.
    if (NET_COROSIO_SANITIZERS OR NET_COROSIO_ASAN OR NET_COROSIO_UBSAN)
      message(WARNING "[net_corosio] Sanitizers are not configured for MSVC in this module.")
    endif()
    return()
  endif()

  set(_enable_asan OFF)
  set(_enable_ubsan OFF)

  if (NET_COROSIO_SANITIZERS)
    set(_enable_asan ON)
    set(_enable_ubsan ON)
  endif()

  if (NET_COROSIO_ASAN)
    set(_enable_asan ON)
  endif()

  if (NET_COROSIO_UBSAN)
    set(_enable_ubsan ON)
  endif()

  if (_enable_asan)
    target_compile_options(${target_name} PRIVATE -fsanitize=address)
    target_link_options(${target_name} PRIVATE -fsanitize=address)
    target_compile_definitions(${target_name} PRIVATE NET_COROSIO_WITH_ASAN=1)
  endif()

  if (_enable_ubsan)
    target_compile_options(${target_name} PRIVATE -fsanitize=undefined)
    target_link_options(${target_name} PRIVATE -fsanitize=undefined)
    target_compile_definitions(${target_name} PRIVATE NET_COROSIO_WITH_UBSAN=1)
  endif()
endfunction()
