#include <vix/net_corosio/tls_context.hpp>

#include <boost/corosio/tls_context.hpp>

#include <string>
#include <type_traits>
#include <utility>

namespace corosio = boost::corosio;

namespace vix::net_corosio
{
  namespace detail
  {
    template <class T>
    concept has_error_method = requires(const T &t) { t.error(); };

    template <class T>
    concept has_value_method = requires(T t) { t.value(); };

    template <class T>
    concept is_bool_testable = requires(const T &t) { static_cast<bool>(t); };

    template <class R>
    Error handle_ret_as_error_code(R &&r, ErrorCode fallback)
    {
      if (static_cast<bool>(r))
        return Error{fallback};
      return Error{ErrorCode::none};
    }

    template <class R>
    Error handle_ret_as_result(R &&r, ErrorCode fallback)
    {
      if constexpr (has_error_method<std::remove_cvref_t<R>>)
      {
        auto ec = r.error();
        if (static_cast<bool>(ec))
          return Error{fallback};
        return Error{ErrorCode::none};
      }
      else if constexpr (is_bool_testable<std::remove_cvref_t<R>>)
      {
        if (!static_cast<bool>(r))
          return Error{fallback};
        return Error{ErrorCode::none};
      }
      else
      {
        return Error{ErrorCode::none};
      }
    }

    template <class Fn>
    Error call_and_map(Fn &&fn, ErrorCode fallback)
    {
      try
      {
        using Ret = decltype(fn());

        if constexpr (std::is_void_v<Ret>)
        {
          fn();
          return Error{ErrorCode::none};
        }
        else
        {
          auto r = fn();

          if constexpr (is_bool_testable<std::remove_cvref_t<decltype(r)>> &&
                        !has_error_method<std::remove_cvref_t<decltype(r)>> &&
                        !has_value_method<std::remove_cvref_t<decltype(r)>>)
          {
            return handle_ret_as_error_code(std::move(r), fallback);
          }

          return handle_ret_as_result(std::move(r), fallback);
        }
      }
      catch (...)
      {
        return Error{ErrorCode::unknown};
      }
    }

    inline corosio::tls_verify_mode to_native(TlsVerifyMode m)
    {
      switch (m)
      {
      case TlsVerifyMode::none:
        return corosio::tls_verify_mode::none;
      case TlsVerifyMode::peer:
        return corosio::tls_verify_mode::peer;
      case TlsVerifyMode::require_peer:
        return corosio::tls_verify_mode::require_peer;
      }
      return corosio::tls_verify_mode::none;
    }

    inline corosio::tls_file_format to_native(TlsFileFormat f)
    {
      switch (f)
      {
      case TlsFileFormat::pem:
        return corosio::tls_file_format::pem;
      case TlsFileFormat::der:
        return corosio::tls_file_format::der;
      }
      return corosio::tls_file_format::pem;
    }

    inline corosio::tls_version to_native(TlsVersion v)
    {
      switch (v)
      {
      case TlsVersion::tls_1_2:
        return corosio::tls_version::tls_1_2;
      case TlsVersion::tls_1_3:
        return corosio::tls_version::tls_1_3;
      }
      return corosio::tls_version::tls_1_2;
    }

  } // namespace detail

  struct TlsContext::Impl final
  {
    TlsRole r{TlsRole::client};
    corosio::tls_context ctx{};

    explicit Impl(TlsRole role)
        : r(role), ctx()
    {
    }
  };

  TlsContext::TlsContext(TlsRole role)
      : impl_(new Impl(role))
  {
  }

  TlsContext::TlsContext(TlsContext &&other) noexcept
      : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }

  TlsContext &TlsContext::operator=(TlsContext &&other) noexcept
  {
    if (this != &other)
    {
      delete impl_;
      impl_ = other.impl_;
      other.impl_ = nullptr;
    }
    return *this;
  }

  TlsContext::~TlsContext()
  {
    delete impl_;
    impl_ = nullptr;
  }

  TlsRole TlsContext::role() const noexcept
  {
    return impl_ ? impl_->r : TlsRole::client;
  }

  Error TlsContext::set_verify_mode(TlsVerifyMode mode)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.set_verify_mode(detail::to_native(mode)); },
        ErrorCode::tls_verify_failed);
  }

  Error TlsContext::set_hostname(std::string_view hostname)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (hostname.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { impl_->ctx.set_hostname(std::string(hostname)); },
        ErrorCode::invalid_argument);
  }

  Error TlsContext::set_default_verify_paths()
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.set_default_verify_paths(); },
        ErrorCode::tls_verify_failed);
  }

  Error TlsContext::load_verify_file(std::string_view path)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (path.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.load_verify_file(std::string(path)); },
        ErrorCode::tls_verify_failed);
  }

  Error TlsContext::add_certificate_authority(std::string_view ca_pem)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (ca_pem.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.add_certificate_authority(std::string(ca_pem)); },
        ErrorCode::tls_verify_failed);
  }

  Error TlsContext::use_certificate_chain_file(std::string_view path)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (path.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.use_certificate_chain_file(std::string(path)); },
        ErrorCode::invalid_argument);
  }

  Error TlsContext::use_certificate_file(std::string_view path, TlsFileFormat fmt)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (path.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.use_certificate_file(std::string(path), detail::to_native(fmt)); },
        ErrorCode::invalid_argument);
  }

  Error TlsContext::use_private_key_file(std::string_view path, TlsFileFormat fmt)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (path.empty())
      return Error{ErrorCode::invalid_argument};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.use_private_key_file(std::string(path), detail::to_native(fmt)); },
        ErrorCode::invalid_argument);
  }

  Error TlsContext::set_alpn(const std::vector<std::string> &protocols)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    if (protocols.empty())
      return Error{ErrorCode::invalid_argument};

    const auto is = [&](std::size_t i, std::string_view v) -> bool
    {
      return i < protocols.size() && std::string_view(protocols[i]) == v;
    };

    if (protocols.size() == 1 && is(0, "h2"))
    {
      return detail::call_and_map([&]
                                  { return impl_->ctx.set_alpn({"h2"}); }, ErrorCode::invalid_argument);
    }

    if (protocols.size() == 1 && is(0, "http/1.1"))
    {
      return detail::call_and_map([&]
                                  { return impl_->ctx.set_alpn({"http/1.1"}); }, ErrorCode::invalid_argument);
    }

    if (protocols.size() == 2 && is(0, "h2") && is(1, "http/1.1"))
    {
      return detail::call_and_map([&]
                                  { return impl_->ctx.set_alpn({"h2", "http/1.1"}); }, ErrorCode::invalid_argument);
    }

    if (protocols.size() == 2 && is(0, "http/1.1") && is(1, "h2"))
    {
      return detail::call_and_map([&]
                                  { return impl_->ctx.set_alpn({"http/1.1", "h2"}); }, ErrorCode::invalid_argument);
    }

    return Error{ErrorCode::invalid_argument};
  }

  Error TlsContext::set_min_protocol_version(TlsVersion v)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.set_min_protocol_version(detail::to_native(v)); },
        ErrorCode::invalid_argument);
  }

  Error TlsContext::set_max_protocol_version(TlsVersion v)
  {
    if (!impl_)
      return Error{ErrorCode::not_initialized};

    return detail::call_and_map(
        [&]
        { return impl_->ctx.set_max_protocol_version(detail::to_native(v)); },
        ErrorCode::invalid_argument);
  }

  void *TlsContext::native_handle() noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<void *>(&(impl_->ctx));
  }

  const void *TlsContext::native_handle() const noexcept
  {
    if (!impl_)
      return nullptr;
    return static_cast<const void *>(&(impl_->ctx));
  }

} // namespace vix::net_corosio
