#include <vix/net_corosio/tls_stream.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/write.hpp>

#include <atomic>
#include <exception>
#include <system_error>
#include <type_traits>
#include <utility>

namespace corosio = boost::corosio;
namespace capy = boost::capy;

//   -DVIX_NET_COROSIO_TLS_WOLFSSL=1  to use wolfSSL
//   -DVIX_NET_COROSIO_TLS_WOLFSSL=0  to use generic/OpenSSL backend
#ifndef VIX_NET_COROSIO_TLS_WOLFSSL
#define VIX_NET_COROSIO_TLS_WOLFSSL 0
#endif

#if VIX_NET_COROSIO_TLS_WOLFSSL

#include <boost/corosio/wolfssl_stream.hpp>
#define VIX_NET_COROSIO_TLS_BACKEND_WOLFSSL 1
#define VIX_NET_COROSIO_TLS_BACKEND_GENERIC 0

#else

#include <boost/corosio/tls_stream.hpp>
#define VIX_NET_COROSIO_TLS_BACKEND_WOLFSSL 0
#define VIX_NET_COROSIO_TLS_BACKEND_GENERIC 1

#endif

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

  // Minimal stable mapping for now.
  static ErrorCode map_tls_error(const std::error_code & /*ec*/, ErrorCode fallback)
  {
    return fallback;
  }

  struct TlsStream::Impl final
  {
    Socket *sock_wrap{nullptr};
    TlsContext *ctx_wrap{nullptr};

    corosio::tcp_socket *sock{nullptr};
    corosio::io_context *ioc{nullptr};

#if VIX_NET_COROSIO_TLS_BACKEND_WOLFSSL
    using NativeStream = corosio::wolfssl_stream;
    NativeStream stream;
#elif VIX_NET_COROSIO_TLS_BACKEND_GENERIC
    using NativeStream = corosio::tls_stream;
    NativeStream stream;
#endif

    explicit Impl(Socket &s, TlsContext &c)
        : sock_wrap(&s),
          ctx_wrap(&c),
          sock(static_cast<corosio::tcp_socket *>(s.native_handle())),
          ioc(static_cast<corosio::io_context *>(s.io_context_handle())),
          stream(
              sock,
              *static_cast<corosio::tls_context *>(c.native_handle()))
    {
    }
  };

  TlsStream::TlsStream(Socket &socket, TlsContext &ctx)
      : impl_(new Impl(socket, ctx))
  {
  }

  TlsStream::TlsStream(TlsStream &&other) noexcept
      : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }

  TlsStream &TlsStream::operator=(TlsStream &&other) noexcept
  {
    if (this != &other)
    {
      delete impl_;
      impl_ = other.impl_;
      other.impl_ = nullptr;
    }
    return *this;
  }

  TlsStream::~TlsStream()
  {
    if (impl_)
    {
      close();
      delete impl_;
      impl_ = nullptr;
    }
  }

  Error TlsStream::handshake()
  {
    if (!impl_ || !impl_->sock || !impl_->ioc || !impl_->ctx_wrap)
      return Error{ErrorCode::not_initialized};

    std::atomic<bool> done{false};
    Error out{ErrorCode::unknown};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
#if VIX_NET_COROSIO_TLS_BACKEND_WOLFSSL
        auto mode = (impl_->ctx_wrap->role() == TlsRole::server)
                        ? corosio::wolfssl_stream::server
                        : corosio::wolfssl_stream::client;

        auto r = co_await impl_->stream.handshake(mode);
#else
        auto mode = (impl_->ctx_wrap->role() == TlsRole::server)
                        ? corosio::tls_stream::server
                        : corosio::tls_stream::client;

        auto r = co_await impl_->stream.handshake(mode);
#endif

        const auto ec = detail::io_error(r);
        if (ec)
        {
          out = Error{map_tls_error(ec, ErrorCode::tls_handshake_failed)};
          done.store(true, std::memory_order_release);
          co_return;
        }

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

  TlsIoResult TlsStream::read_some(void *data, std::size_t size)
  {
    TlsIoResult out{};
    out.error = Error{ErrorCode::unknown};
    out.bytes = 0;

    if (!impl_ || !impl_->sock || !impl_->ioc)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    if (!data || size == 0)
    {
      out.error = Error{ErrorCode::invalid_argument};
      return out;
    }

    std::atomic<bool> done{false};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await impl_->stream.read_some(capy::mutable_buffer(data, size));
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out.error = Error{map_tls_error(ec, ErrorCode::read_failed)};
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

  TlsIoResult TlsStream::write_some(const void *data, std::size_t size)
  {
    TlsIoResult out{};
    out.error = Error{ErrorCode::unknown};
    out.bytes = 0;

    if (!impl_ || !impl_->sock || !impl_->ioc)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    if (!data || size == 0)
    {
      out.error = Error{ErrorCode::invalid_argument};
      return out;
    }

    std::atomic<bool> done{false};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await capy::write(impl_->stream, capy::const_buffer(data, size));
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out.error = Error{map_tls_error(ec, ErrorCode::write_failed)};
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

  Error TlsStream::shutdown()
  {
    if (!impl_ || !impl_->sock || !impl_->ioc)
      return Error{ErrorCode::not_initialized};

    std::atomic<bool> done{false};
    Error out{ErrorCode::unknown};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await impl_->stream.shutdown();
        const auto ec = detail::io_error(r);

        if (ec)
        {
          out = Error{map_tls_error(ec, ErrorCode::tls_shutdown_failed)};
          done.store(true, std::memory_order_release);
          co_return;
        }

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

  void TlsStream::close() noexcept
  {
    if (!impl_)
      return;

    try
    {
      (void)shutdown();
    }
    catch (...)
    {
    }

    try
    {
      if (impl_->sock_wrap)
        impl_->sock_wrap->close();
    }
    catch (...)
    {
    }
  }

  void *TlsStream::native_handle() noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<void *>(&(impl_->stream));
  }

  const void *TlsStream::native_handle() const noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<const void *>(&(impl_->stream));
  }

} // namespace vix::net_corosio
