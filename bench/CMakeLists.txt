if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  project(vix_net_corosio_bench LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  find_package(net_corosio CONFIG REQUIRED)
endif()

add_executable(tcp_throughput tcp_throughput.cpp)
target_link_libraries(tcp_throughput PRIVATE vix::net_corosio)

add_executable(tcp_latency tcp_latency.cpp)
target_link_libraries(tcp_latency PRIVATE vix::net_corosio)

add_executable(tls_handshake tls_handshake.cpp)
target_link_libraries(tls_handshake PRIVATE vix::net_corosio)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(tcp_throughput PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tcp_latency PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tls_handshake PRIVATE -Wall -Wextra -Wpedantic)
endif()
