#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <vix/net_corosio/error.hpp>

namespace vix::net_corosio
{
  /**
   * @brief TLS verify mode (high-level).
   */
  enum class TlsVerifyMode : std::uint8_t
  {
    none = 0,        // do not verify peer
    peer = 1,        // verify peer certificate
    require_peer = 2 // require peer certificate (mTLS-style)
  };

  /**
   * @brief TLS file format.
   */
  enum class TlsFileFormat : std::uint8_t
  {
    pem = 0,
    der = 1
  };

  /**
   * @brief TLS protocol version (high-level).
   *
   * This is intentionally minimal.
   */
  enum class TlsVersion : std::uint8_t
  {
    tls_1_2 = 12,
    tls_1_3 = 13
  };

  /**
   * @brief TLS role.
   */
  enum class TlsRole : std::uint8_t
  {
    client = 0,
    server = 1
  };

  /**
   * @brief A stable, backend-agnostic TLS context wrapper.
   *
   * This does not create sockets. It only stores TLS configuration
   * and produces TLS streams later.
   *
   * Design goals:
   * - keep API explicit
   * - keep configuration auditable
   * - avoid exposing backend TLS types
   */
  class TlsContext final
  {
  public:
    explicit TlsContext(TlsRole role);

    TlsContext(TlsContext &&) noexcept;
    TlsContext &operator=(TlsContext &&) noexcept;

    TlsContext(const TlsContext &) = delete;
    TlsContext &operator=(const TlsContext &) = delete;

    ~TlsContext();

    /**
     * @brief Returns configured role.
     */
    TlsRole role() const noexcept;

    /**
     * @brief Set verify mode.
     */
    Error set_verify_mode(TlsVerifyMode mode);

    /**
     * @brief Set SNI hostname (client-side).
     *
     * Also used for hostname verification when verify mode is peer.
     */
    Error set_hostname(std::string_view hostname);

    /**
     * @brief Use system default CA store.
     */
    Error set_default_verify_paths();

    /**
     * @brief Load CA bundle file (PEM).
     */
    Error load_verify_file(std::string_view path);

    /**
     * @brief Add CA certificate from PEM buffer.
     */
    Error add_certificate_authority(std::string_view ca_pem);

    /**
     * @brief Load server certificate chain from file.
     */
    Error use_certificate_chain_file(std::string_view path);

    /**
     * @brief Load certificate from file (client cert or single cert).
     */
    Error use_certificate_file(std::string_view path, TlsFileFormat fmt);

    /**
     * @brief Load private key from file.
     */
    Error use_private_key_file(std::string_view path, TlsFileFormat fmt);

    /**
     * @brief Configure ALPN protocols.
     *
     * Example: {"h2", "http/1.1"}
     */
    Error set_alpn(const std::vector<std::string> &protocols);

    /**
     * @brief Set minimum TLS version.
     */
    Error set_min_protocol_version(TlsVersion v);

    /**
     * @brief Set maximum TLS version.
     *
     * If not set, "no max" is assumed (backend default).
     */
    Error set_max_protocol_version(TlsVersion v);

    /**
     * @brief Opaque handle for internal integration.
     *
     * Never downcast outside .cpp files.
     */
    void *native_handle() noexcept;
    const void *native_handle() const noexcept;

  private:
    struct Impl;
    Impl *impl_{nullptr};
  };

} // namespace vix::net_corosio
