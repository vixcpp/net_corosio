#pragma once

#include <vix/net_corosio/error.hpp>

namespace vix::net_corosio
{
  /**
   * @brief Backend-agnostic executor handle.
   *
   * Stores an opaque pointer to the backend event-loop context
   * (currently corosio::io_context).
   *
   * Internal .cpp files can recover the real executor via
   * io_context::get_executor().
   */
  class Executor final
  {
  public:
    Executor() = default;

    Executor(Executor &&) noexcept = default;
    Executor &operator=(Executor &&) noexcept = default;

    Executor(const Executor &) = delete;
    Executor &operator=(const Executor &) = delete;

    ~Executor() = default;

    /**
     * @brief Returns true if executor is valid.
     */
    bool valid() const noexcept
    {
      return native_ != nullptr;
    }

    /**
     * @brief Exposes opaque backend handle.
     *
     * Only for internal .cpp usage.
     */
    void *native_handle() noexcept
    {
      return native_;
    }

    const void *native_handle() const noexcept
    {
      return native_;
    }

  private:
    friend class Context;

    explicit Executor(void *native) noexcept
        : native_(native)
    {
    }

    void *native_{nullptr};
  };

} // namespace vix::net_corosio
