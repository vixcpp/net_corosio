#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <vix/net_corosio/error.hpp>

namespace vix::net_corosio
{
  class Context;

  /**
   * @brief IP version of a resolved endpoint.
   */
  enum class IpVersion : std::uint8_t
  {
    v4 = 4,
    v6 = 6
  };

  /**
   * @brief Resolved endpoint (address + port).
   *
   * This is a stable, backend-agnostic representation.
   */
  struct Endpoint final
  {
    IpVersion ip{IpVersion::v4};

    // For v4: dotted string "127.0.0.1"
    // For v6: canonical string, e.g. "2001:db8::1"
    std::string address{};

    std::uint16_t port{0};
  };

  /**
   * @brief DNS resolution result.
   */
  struct ResolveResult final
  {
    Error error{};
    std::vector<Endpoint> endpoints{};

    bool ok() const noexcept { return error.ok(); }
  };

  /**
   * @brief DNS resolver bound to a Context.
   *
   * This is a thin wrapper whose purpose is:
   * - stable API surface
   * - consistent error model
   * - easy benchmarking/instrumentation
   */
  class Resolver final
  {
  public:
    explicit Resolver(Context &ctx);

    Resolver(Resolver &&) noexcept;
    Resolver &operator=(Resolver &&) noexcept;

    Resolver(const Resolver &) = delete;
    Resolver &operator=(const Resolver &) = delete;

    ~Resolver();

    /**
     * @brief Resolve host and service to a list of endpoints.
     *
     * service examples:
     * - "http"
     * - "https"
     * - "8080"
     *
     * If service is empty, resolved endpoints may have port=0.
     */
    ResolveResult resolve(std::string_view host, std::string_view service);

  private:
    struct Impl;
    Impl *impl_{nullptr};
  };

} // namespace vix::net_corosio
