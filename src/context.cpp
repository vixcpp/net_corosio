#include <vix/net_corosio/context.hpp>

#include <boost/corosio.hpp>

#include <atomic>
#include <exception>
#include <utility>

namespace corosio = boost::corosio;

namespace vix::net_corosio
{
  namespace
  {
    static corosio::io_context *as_ioc(void *h) noexcept
    {
      return static_cast<corosio::io_context *>(h);
    }

    static const corosio::io_context *as_ioc(const void *h) noexcept
    {
      return static_cast<const corosio::io_context *>(h);
    }
  } // namespace

  struct Context::Impl final
  {
    Config cfg{};
    corosio::io_context ioc{};
    std::atomic<bool> stop_requested{false};

    explicit Impl(Config c)
        : cfg(std::move(c)), ioc()
    {
    }
  };

  Context::Context()
      : impl_(std::make_unique<Impl>(default_config()))
  {
  }

  Context::Context(Config cfg)
      : impl_(std::make_unique<Impl>(std::move(cfg)))
  {
  }

  Context::Context(Context &&other) noexcept = default;

  Context &Context::operator=(Context &&other) noexcept = default;

  Context::~Context() = default;

  const Config &Context::config() const noexcept
  {
    return impl_->cfg;
  }

  void Context::set_config(Config cfg)
  {
    impl_->cfg = std::move(cfg);
  }

  Error Context::run()
  {
    try
    {
      if (!impl_)
      {
        return Error{ErrorCode::invalid_state};
      }

      impl_->stop_requested.store(false, std::memory_order_relaxed);

      impl_->ioc.run();
      return Error{ErrorCode::none};
    }
    catch (...)
    {
      return Error{ErrorCode::unknown};
    }
  }

  void Context::stop() noexcept
  {
    if (!impl_)
    {
      return;
    }

    impl_->stop_requested.store(true, std::memory_order_relaxed);

    try
    {
      impl_->ioc.stop();
    }
    catch (...)
    {
      // Never throw from stop().
    }
  }

  bool Context::stop_requested() const noexcept
  {
    if (!impl_)
    {
      return true;
    }

    return impl_->stop_requested.load(std::memory_order_relaxed);
  }

  void *Context::native_handle() noexcept
  {
    // Safe for moved-from Context.
    return impl_ ? static_cast<void *>(&(impl_->ioc)) : nullptr;
  }

  const void *Context::native_handle() const noexcept
  {
    // Safe for moved-from Context.
    return impl_ ? static_cast<const void *>(&(impl_->ioc)) : nullptr;
  }

  Executor Context::get_executor() noexcept
  {
    if (!impl_)
      return Executor{nullptr};

    // store io_context handle, stable address
    return Executor{static_cast<void *>(&(impl_->ioc))};
  }

} // namespace vix::net_corosio
