# ============================================================
# net_corosio - Warnings
# ============================================================

function(net_corosio_apply_warnings target_name)
  if (NOT TARGET ${target_name})
    message(FATAL_ERROR "[net_corosio] net_corosio_apply_warnings: target not found: ${target_name}")
  endif()

  if (MSVC)
    target_compile_options(${target_name} PRIVATE
      /W4
      /permissive-
      /EHsc
    )
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
    )

    # Optional strictness (kept mild by default to avoid third-party noise)
    option(NET_COROSIO_WERROR "Treat warnings as errors (net_corosio only)" OFF)
    if (NET_COROSIO_WERROR)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()
