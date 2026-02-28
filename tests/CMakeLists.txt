function(net_corosio_add_test test_name source_file)
  add_executable(${test_name} ${source_file})
  target_link_libraries(${test_name} PRIVATE vix::net_corosio)

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

net_corosio_add_test(net_corosio.context   test_context.cpp)
net_corosio_add_test(net_corosio.resolver  test_resolver.cpp)
net_corosio_add_test(net_corosio.tcp_echo  test_tcp_echo.cpp)
net_corosio_add_test(net_corosio.tls       test_tls.cpp)
