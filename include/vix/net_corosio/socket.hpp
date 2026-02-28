#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <vix/net_corosio/error.hpp>

namespace vix::net_corosio
{
  class Context;

  /**
   * @brief TCP socket state.
   */
  enum class SocketState : std::uint8_t
  {
    closed = 0,
    open,
    connected
  };

  /**
   * @brief TCP connection endpoint.
   *
   * This is intentionally minimal: address string + port.
   * Parsing/validation is performed in the implementation layer.
   */
  struct TcpEndpoint final
  {
    std::string address{};
    std::uint16_t port{0};
  };

  /**
   * @brief Read result.
   */
  struct IoResult final
  {
    Error error{};
    std::size_t bytes{0};

    bool ok() const noexcept { return error.ok(); }
  };

  /**
   * @brief Backend-agnostic TCP socket wrapper (Corosio implementation).
   *
   * Design goals:
   * - stable API surface
   * - explicit error model
   * - no backend types in public headers
   * - no hidden allocations policy (caller owns buffers)
   */
  class Socket final
  {
  public:
    explicit Socket(Context &ctx);

    Socket(Socket &&) noexcept;
    Socket &operator=(Socket &&) noexcept;

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    ~Socket();

    /**
     * @brief Returns the current state.
     */
    SocketState state() const noexcept;

    /**
     * @brief Open the socket (idempotent).
     */
    Error open();

    /**
     * @brief Connect to a TCP endpoint.
     *
     * Requires: open() (or will open implicitly if strict_checks=false).
     */
    Error connect(const TcpEndpoint &ep);

    /**
     * @brief Read some bytes into caller-provided buffer.
     */
    IoResult read_some(void *data, std::size_t size);

    /**
     * @brief Write some bytes from caller-provided buffer.
     */
    IoResult write_some(const void *data, std::size_t size);

    /**
     * @brief Close the socket (safe to call multiple times).
     */
    void close() noexcept;

    /**
     * @brief Exposes an opaque native handle for internal integration.
     *
     * Never downcast outside .cpp files.
     */
    void *native_handle() noexcept;
    const void *native_handle() const noexcept;
    void *io_context_handle() noexcept;

  private:
    struct Impl;
    Impl *impl_{nullptr};
  };

} // namespace vix::net_corosio
