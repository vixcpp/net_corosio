#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <vix/net_corosio/config.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/executor.hpp>

namespace vix::net_corosio
{
  /**
   * @brief Execution context for the net_corosio backend.
   *
   * This is the only object that owns the underlying event loop.
   * It is intentionally small and stable at the API level.
   *
   * Design goals:
   * - No backend types in public headers
   * - Explicit lifecycle (start/run/stop)
   * - Deterministic ownership
   */
  class Context final
  {
  public:
    Context();
    explicit Context(Config cfg);

    Context(Context &&) noexcept;
    Context &operator=(Context &&) noexcept;

    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;

    ~Context();
    const Config &config() const noexcept;
    void set_config(Config cfg);

    /**
     * @brief Run the event loop (blocking).
     *
     * Returns:
     * - Error{none} on clean exit
     * - Error{...} on failure
     */
    Error run();

    /**
     * @brief Request stop.
     *
     * Safe to call from any thread.
     */
    void stop() noexcept;

    /**
     * @brief Returns true if stop() was requested.
     */
    bool stop_requested() const noexcept;

    /**
     * @brief Returns an opaque handle for integration.
     *
     * This is intentionally "void*" to avoid leaking backend types.
     * Internal modules may downcast in .cpp files only.
     */
    void *native_handle() noexcept;
    const void *native_handle() const noexcept;
    Executor get_executor() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

} // namespace vix::net_corosio
