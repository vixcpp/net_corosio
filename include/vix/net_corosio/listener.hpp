#pragma once

#include <cstddef>
#include <cstdint>

#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/socket.hpp>

namespace vix::net_corosio
{
  class Context;

  /**
   * @brief TCP listener state.
   */
  enum class ListenerState : std::uint8_t
  {
    closed = 0,
    open,
    listening
  };

  /**
   * @brief TCP listener (acceptor) wrapper.
   *
   * This is used to accept incoming TCP connections and create Socket objects.
   */
  class Listener final
  {
  public:
    explicit Listener(Context &ctx);

    Listener(Listener &&) noexcept;
    Listener &operator=(Listener &&) noexcept;

    Listener(const Listener &) = delete;
    Listener &operator=(const Listener &) = delete;

    ~Listener();

    ListenerState state() const noexcept;

    /**
     * @brief Open the listener (idempotent).
     */
    Error open();

    /**
     * @brief Bind to a local port (IPv4 any by default).
     *
     * If you need to bind to a specific address later, we can extend this API.
     */
    Error bind(std::uint16_t port);

    /**
     * @brief Start listening.
     */
    Error listen(int backlog = 128);

    /**
     * @brief Accept one incoming connection.
     *
     * On success, returns {none, Socket(ctx)} with a connected socket.
     * On error, returns {error, Socket(ctx)} where socket is closed.
     */
    struct AcceptResult final
    {
      Error error{};
      Socket socket;

      explicit AcceptResult(Context &ctx)
          : error(Error{ErrorCode::unknown}), socket(ctx)
      {
      }

      bool ok() const noexcept { return error.ok(); }
    };

    AcceptResult accept();

    /**
     * @brief Close the listener (safe to call multiple times).
     */
    void close() noexcept;

    void *native_handle() noexcept;
    const void *native_handle() const noexcept;

  private:
    struct Impl;
    Impl *impl_{nullptr};
  };

} // namespace vix::net_corosio
