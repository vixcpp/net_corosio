#pragma once

#include <cstddef>
#include <cstdint>

#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/socket.hpp>
#include <vix/net_corosio/tls_context.hpp>

namespace vix::net_corosio
{
  /**
   * @brief Result for TLS I/O operations.
   */
  struct TlsIoResult final
  {
    Error error{};
    std::size_t bytes{0};

    bool ok() const noexcept { return error.ok(); }
  };

  /**
   * @brief TLS stream wrapper bound to an existing TCP socket.
   *
   * The socket must be connected before creating the TLS stream.
   *
   * This wrapper keeps the public API stable while allowing different
   * TLS backends internally (wolfSSL, OpenSSL, etc.), depending on Corosio.
   */
  class TlsStream final
  {
  public:
    /**
     * @brief Create a TLS stream over a connected socket.
     *
     * The TlsContext is shared by reference at construction time.
     * The stream keeps its own internal handle to the native TLS object.
     */
    TlsStream(Socket &socket, TlsContext &ctx);

    TlsStream(TlsStream &&) noexcept;
    TlsStream &operator=(TlsStream &&) noexcept;

    TlsStream(const TlsStream &) = delete;
    TlsStream &operator=(const TlsStream &) = delete;

    ~TlsStream();

    /**
     * @brief Perform TLS handshake.
     */
    Error handshake();

    /**
     * @brief Read some decrypted bytes.
     */
    TlsIoResult read_some(void *data, std::size_t size);

    /**
     * @brief Write some plaintext bytes (encrypted on the wire).
     */
    TlsIoResult write_some(const void *data, std::size_t size);

    /**
     * @brief TLS shutdown (close_notify when supported).
     */
    Error shutdown();

    /**
     * @brief Close underlying TCP socket (best-effort).
     */
    void close() noexcept;

    void *native_handle() noexcept;
    const void *native_handle() const noexcept;

  private:
    struct Impl;
    Impl *impl_{nullptr};
  };

} // namespace vix::net_corosio
