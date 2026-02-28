add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE vix::net_corosio)

add_executable(echo_client echo_client.cpp)
target_link_libraries(echo_client PRIVATE vix::net_corosio)

add_executable(dns_lookup dns_lookup.cpp)
target_link_libraries(dns_lookup PRIVATE vix::net_corosio)

add_executable(https_client https_client.cpp)
target_link_libraries(https_client PRIVATE vix::net_corosio)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(echo_server PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(echo_client PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(dns_lookup PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(https_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
