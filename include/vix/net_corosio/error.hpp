#pragma once

#include <cstdint>
#include <string_view>

namespace vix::net_corosio
{
  /**
   * @brief High-level error category for net_corosio.
   *
   * This enum intentionally abstracts away the underlying
   * networking backend (Boost/Corosio).
   */
  enum class ErrorCode : std::uint16_t
  {
    none = 0,

    // Generic
    unknown,
    invalid_argument,
    invalid_state,
    not_initialized,

    // Networking
    resolve_failed,
    connect_failed,
    accept_failed,
    read_failed,
    write_failed,
    timeout,
    connection_closed,

    // TLS
    tls_handshake_failed,
    tls_shutdown_failed,
    tls_verify_failed
  };

  /**
   * @brief Lightweight error wrapper.
   *
   * This type is intentionally simple.
   * No dynamic allocation.
   * No std::error_code dependency.
   */
  struct Error final
  {
    ErrorCode code{ErrorCode::none};

    constexpr bool ok() const noexcept
    {
      return code == ErrorCode::none;
    }

    constexpr explicit operator bool() const noexcept
    {
      return !ok();
    }

    constexpr ErrorCode value() const noexcept
    {
      return code;
    }
  };

  /**
   * @brief Convert error code to string (for logging / debugging).
   *
   * This is a stable mapping layer.
   */
  constexpr std::string_view to_string(ErrorCode ec) noexcept
  {
    switch (ec)
    {
    case ErrorCode::none:
      return "none";
    case ErrorCode::unknown:
      return "unknown";
    case ErrorCode::invalid_argument:
      return "invalid_argument";
    case ErrorCode::invalid_state:
      return "invalid_state";
    case ErrorCode::not_initialized:
      return "not_initialized";

    case ErrorCode::resolve_failed:
      return "resolve_failed";
    case ErrorCode::connect_failed:
      return "connect_failed";
    case ErrorCode::accept_failed:
      return "accept_failed";
    case ErrorCode::read_failed:
      return "read_failed";
    case ErrorCode::write_failed:
      return "write_failed";
    case ErrorCode::timeout:
      return "timeout";
    case ErrorCode::connection_closed:
      return "connection_closed";

    case ErrorCode::tls_handshake_failed:
      return "tls_handshake_failed";
    case ErrorCode::tls_shutdown_failed:
      return "tls_shutdown_failed";
    case ErrorCode::tls_verify_failed:
      return "tls_verify_failed";
    }

    return "unknown";
  }

} // namespace vix::net_corosio
