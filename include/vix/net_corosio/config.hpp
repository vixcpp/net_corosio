#pragma once

#include <cstddef>
#include <cstdint>

namespace vix::net_corosio
{
  /**
   * @brief Build/runtime configuration for the net_corosio backend.
   *
   * This configuration is intentionally small and explicit.
   * It defines behavior boundaries, not policy surprises.
   */
  struct Config final
  {
    /**
     * @brief Default size for small I/O buffers used by examples/tests.
     *
     * This is not a hard limit. It is only a default used by helpers.
     */
    std::size_t default_buffer_bytes{4096};

    /**
     * @brief Maximum number of worker tasks for server-style accept loops.
     *
     * 0 means "backend default". Prefer explicit values for benchmarks.
     */
    std::size_t max_workers{0};

    /**
     * @brief Enable strict defensive checks in the wrapper layer.
     *
     * When true, the wrapper performs additional validations (e.g. state checks)
     * before delegating to the backend.
     *
     * This should not change networking semantics, only guardrails.
     */
    bool strict_checks{true};

    /**
     * @brief Enable lightweight tracing hooks (no logging dependency).
     *
     * This flag is designed for integration with Vix logging/telemetry
     * without creating a dependency here.
     */
    bool enable_tracing{false};
  };

  /**
   * @brief Returns a sane default configuration.
   */
  inline Config default_config()
  {
    return Config{};
  }

} // namespace vix::net_corosio
