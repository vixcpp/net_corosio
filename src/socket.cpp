#include <vix/net_corosio/socket.hpp>
#include <vix/net_corosio/context.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/write.hpp>

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <type_traits>
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
    concept has_bytes_method = requires(const T &t) { t.bytes_transferred(); };

    template <class T>
    concept has_size_method = requires(const T &t) { t.size(); };

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

    template <class T>
    std::size_t io_bytes(const T &r)
    {
      if constexpr (has_bytes_method<T>)
      {
        return static_cast<std::size_t>(r.bytes_transferred());
      }
      else if constexpr (has_size_method<T>)
      {
        return static_cast<std::size_t>(r.size());
      }
      else
      {
        return 0;
      }
    }
  } // namespace detail

  struct Socket::Impl final
  {
    Context *ctx{nullptr};
    corosio::io_context *ioc{nullptr};
    corosio::tcp_socket sock;
    SocketState st{SocketState::closed};

    explicit Impl(Context &c)
        : ctx(&c),
          ioc(static_cast<corosio::io_context *>(c.native_handle())),
          sock(*ioc)
    {
    }
  };

  static ErrorCode map_io_error_to_code(const std::error_code & /*ec*/, ErrorCode fallback)
  {
    // Minimal stable mapping for now.
    return fallback;
  }

  static bool parse_endpoint(const TcpEndpoint &ep, corosio::endpoint &out)
  {
    if (ep.port == 0)
      return false;

    if (ep.address == "localhost" || ep.address == "127.0.0.1")
    {
      out = corosio::endpoint(corosio::ipv4_address::loopback(), ep.port);
      return true;
    }

    corosio::ipv4_address addr4;
    if (!corosio::parse_ipv4_address(ep.address.c_str(), addr4))
    {
      out = corosio::endpoint(addr4, ep.port);
      return true;
    }

    return false;
  }

  Socket::Socket(Context &ctx)
      : impl_(new Impl(ctx))
  {
  }

  Socket::Socket(Socket &&other) noexcept
      : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }

  Socket &Socket::operator=(Socket &&other) noexcept
  {
    if (this != &other)
    {
      delete impl_;
      impl_ = other.impl_;
      other.impl_ = nullptr;
    }
    return *this;
  }

  Socket::~Socket()
  {
    if (impl_)
    {
      close();
      delete impl_;
      impl_ = nullptr;
    }
  }

  SocketState Socket::state() const noexcept
  {
    return impl_ ? impl_->st : SocketState::closed;
  }

  Error Socket::open()
  {
    if (!impl_ || !impl_->ioc)
      return Error{ErrorCode::not_initialized};

    if (impl_->st != SocketState::closed)
      return Error{ErrorCode::none};

    try
    {
      impl_->sock.open();
      impl_->st = SocketState::open;
      return Error{ErrorCode::none};
    }
    catch (...)
    {
      return Error{ErrorCode::unknown};
    }
  }

  Error Socket::connect(const TcpEndpoint &ep)
  {
    if (!impl_ || !impl_->ioc)
      return Error{ErrorCode::not_initialized};

    const bool strict = impl_->ctx ? impl_->ctx->config().strict_checks : true;

    if (strict && impl_->st == SocketState::closed)
    {
      auto e = open();
      if (e)
        return e;
    }
    else if (!strict && impl_->st == SocketState::closed)
    {
      (void)open();
    }

    corosio::endpoint target{};
    if (!parse_endpoint(ep, target))
      return Error{ErrorCode::invalid_argument};

    std::atomic<bool> done{false};
    Error out{ErrorCode::unknown};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await impl_->sock.connect(target);
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out = Error{map_io_error_to_code(ec, ErrorCode::connect_failed)};
          done.store(true, std::memory_order_release);
          co_return;
        }

        impl_->st = SocketState::connected;
        out = Error{ErrorCode::none};
      }
      catch (...)
      {
        out = Error{ErrorCode::unknown};
      }

      done.store(true, std::memory_order_release);
    };

    capy::run_async(impl_->ioc->get_executor())(task());

    while (!done.load(std::memory_order_acquire))
    {
      impl_->ioc->run_one();
    }

    return out;
  }

  IoResult Socket::read_some(void *data, std::size_t size)
  {
    IoResult out{};
    out.error = Error{ErrorCode::unknown};
    out.bytes = 0;

    if (!impl_ || !impl_->ioc)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    if (!data || size == 0)
    {
      out.error = Error{ErrorCode::invalid_argument};
      return out;
    }

    const bool strict = impl_->ctx ? impl_->ctx->config().strict_checks : true;
    if (strict && impl_->st != SocketState::connected)
    {
      out.error = Error{ErrorCode::invalid_state};
      return out;
    }

    std::atomic<bool> done{false};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await impl_->sock.read_some(capy::mutable_buffer(data, size));
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out.error = Error{map_io_error_to_code(ec, ErrorCode::read_failed)};
          out.bytes = 0;
          done.store(true, std::memory_order_release);
          co_return;
        }

        out.error = Error{ErrorCode::none};
        out.bytes = detail::io_bytes(r);
      }
      catch (...)
      {
        out.error = Error{ErrorCode::unknown};
        out.bytes = 0;
      }

      done.store(true, std::memory_order_release);
    };

    capy::run_async(impl_->ioc->get_executor())(task());

    while (!done.load(std::memory_order_acquire))
    {
      impl_->ioc->run_one();
    }

    return out;
  }

  IoResult Socket::write_some(const void *data, std::size_t size)
  {
    IoResult out{};
    out.error = Error{ErrorCode::unknown};
    out.bytes = 0;

    if (!impl_ || !impl_->ioc)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    if (!data || size == 0)
    {
      out.error = Error{ErrorCode::invalid_argument};
      return out;
    }

    const bool strict = impl_->ctx ? impl_->ctx->config().strict_checks : true;
    if (strict && impl_->st != SocketState::connected)
    {
      out.error = Error{ErrorCode::invalid_state};
      return out;
    }

    std::atomic<bool> done{false};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await capy::write(impl_->sock, capy::const_buffer(data, size));
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out.error = Error{map_io_error_to_code(ec, ErrorCode::write_failed)};
          out.bytes = 0;
          done.store(true, std::memory_order_release);
          co_return;
        }

        out.error = Error{ErrorCode::none};
        out.bytes = detail::io_bytes(r);
      }
      catch (...)
      {
        out.error = Error{ErrorCode::unknown};
        out.bytes = 0;
      }

      done.store(true, std::memory_order_release);
    };

    capy::run_async(impl_->ioc->get_executor())(task());

    while (!done.load(std::memory_order_acquire))
    {
      impl_->ioc->run_one();
    }

    return out;
  }

  void Socket::close() noexcept
  {
    if (!impl_)
      return;

    try
    {
      impl_->sock.close();
    }
    catch (...)
    {
    }

    impl_->st = SocketState::closed;
  }

  void *Socket::native_handle() noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<void *>(&(impl_->sock));
  }

  const void *Socket::native_handle() const noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<const void *>(&(impl_->sock));
  }

  void *Socket::io_context_handle() noexcept
  {
    if (!impl_ || !impl_->ioc)
      return nullptr;
    return static_cast<void *>(impl_->ioc);
  }
} // namespace vix::net_corosio
