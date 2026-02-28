#include <vix/net_corosio/listener.hpp>
#include <vix/net_corosio/context.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>

#include <atomic>
#include <exception>
#include <system_error>
#include <utility>

namespace corosio = boost::corosio;
namespace capy = boost::capy;

namespace vix::net_corosio
{
  namespace detail
  {
    template <class T>
    concept has_error_method = requires(const T &t) { t.error(); };

    template <class T>
    std::error_code io_error(const T &r)
    {
      if constexpr (has_error_method<T>)
      {
        return r.error();
      }
      else
      {
        return {};
      }
    }
  } // namespace detail

  struct Listener::Impl final
  {
    Context *ctx{nullptr};
    corosio::io_context *ioc{nullptr};
    corosio::tcp_acceptor acc;
    ListenerState st{ListenerState::closed};

    explicit Impl(Context &c)
        : ctx(&c),
          ioc(static_cast<corosio::io_context *>(c.native_handle())),
          acc(*ioc)
    {
    }
  };

  Listener::Listener(Context &ctx)
      : impl_(new Impl(ctx))
  {
  }

  Listener::Listener(Listener &&other) noexcept
      : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }

  Listener &Listener::operator=(Listener &&other) noexcept
  {
    if (this != &other)
    {
      delete impl_;
      impl_ = other.impl_;
      other.impl_ = nullptr;
    }
    return *this;
  }

  Listener::~Listener()
  {
    if (impl_)
    {
      close();
      delete impl_;
      impl_ = nullptr;
    }
  }

  ListenerState Listener::state() const noexcept
  {
    return impl_ ? impl_->st : ListenerState::closed;
  }

  Error Listener::open()
  {
    if (!impl_ || !impl_->ioc)
      return Error{ErrorCode::not_initialized};

    if (impl_->st != ListenerState::closed)
      return Error{ErrorCode::none};

    try
    {
      impl_->acc.open();
      impl_->st = ListenerState::open;
      return Error{ErrorCode::none};
    }
    catch (...)
    {
      return Error{ErrorCode::unknown};
    }
  }

  Error Listener::bind(std::uint16_t port)
  {
    if (!impl_ || !impl_->ioc || !impl_->ctx)
      return Error{ErrorCode::not_initialized};

    const bool strict = impl_->ctx->config().strict_checks;

    if (strict && impl_->st == ListenerState::closed)
    {
      auto e = open();
      if (e)
        return e;
    }
    else if (!strict && impl_->st == ListenerState::closed)
    {
      (void)open();
    }

    if (port == 0)
      return Error{ErrorCode::invalid_argument};

    try
    {
      auto ec = impl_->acc.bind(corosio::endpoint(port));
      if (ec)
        return Error{ErrorCode::accept_failed};

      return Error{ErrorCode::none};
    }
    catch (...)
    {
      return Error{ErrorCode::unknown};
    }
  }

  Error Listener::listen(int backlog)
  {
    if (!impl_ || !impl_->ioc || !impl_->ctx)
      return Error{ErrorCode::not_initialized};

    const bool strict = impl_->ctx->config().strict_checks;

    if (strict && impl_->st == ListenerState::closed)
      return Error{ErrorCode::invalid_state};

    if (backlog <= 0)
      backlog = 128;

    try
    {
      auto ec = impl_->acc.listen(backlog);
      if (ec)
        return Error{ErrorCode::accept_failed};

      impl_->st = ListenerState::listening;
      return Error{ErrorCode::none};
    }
    catch (...)
    {
      return Error{ErrorCode::unknown};
    }
  }

  Listener::AcceptResult Listener::accept()
  {
    if (!impl_ || !impl_->ioc || !impl_->ctx)
    {
      std::terminate();
    }

    AcceptResult out(*impl_->ctx);
    out.error = Error{ErrorCode::unknown};
    out.socket.close();

    const bool strict = impl_->ctx->config().strict_checks;
    if (strict && impl_->st != ListenerState::listening)
    {
      out.error = Error{ErrorCode::invalid_state};
      return out;
    }

    auto *native_sock = static_cast<corosio::tcp_socket *>(out.socket.native_handle());
    if (!native_sock)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    std::atomic<bool> done{false};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await impl_->acc.accept(*native_sock);
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out.error = Error{ErrorCode::accept_failed};
          done.store(true, std::memory_order_release);
          co_return;
        }

        out.error = Error{ErrorCode::none};
      }
      catch (...)
      {
        out.error = Error{ErrorCode::unknown};
      }

      done.store(true, std::memory_order_release);
    };

    capy::run_async(impl_->ioc->get_executor())(task());

    while (!done.load(std::memory_order_acquire))
    {
      impl_->ioc->run_one();
    }

    if (out.error)
      out.socket.close();

    return out;
  }

  void Listener::close() noexcept
  {
    if (!impl_)
      return;

    try
    {
      impl_->acc.close();
    }
    catch (...)
    {
    }

    impl_->st = ListenerState::closed;
  }

  void *Listener::native_handle() noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<void *>(&(impl_->acc));
  }

  const void *Listener::native_handle() const noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<const void *>(&(impl_->acc));
  }

} // namespace vix::net_corosio
