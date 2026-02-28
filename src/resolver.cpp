#include <vix/net_corosio/resolver.hpp>
#include <vix/net_corosio/context.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/ex/run_async.hpp>

#include <atomic>
#include <exception>
#include <optional>
#include <utility>

namespace corosio = boost::corosio;
namespace capy = boost::capy;

namespace vix::net_corosio
{
  struct Resolver::Impl final
  {
    Context *ctx{nullptr};
    corosio::io_context *ioc{nullptr};

    explicit Impl(Context &c)
        : ctx(&c),
          ioc(static_cast<corosio::io_context *>(c.native_handle()))
    {
    }
  };

  Resolver::Resolver(Context &ctx)
      : impl_(new Impl(ctx))
  {
  }

  Resolver::Resolver(Resolver &&other) noexcept
      : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }

  Resolver &Resolver::operator=(Resolver &&other) noexcept
  {
    if (this != &other)
    {
      delete impl_;
      impl_ = other.impl_;
      other.impl_ = nullptr;
    }
    return *this;
  }

  Resolver::~Resolver()
  {
    delete impl_;
    impl_ = nullptr;
  }

  static capy::task<void>
  resolve_task(
      corosio::io_context &ioc,
      std::string host,
      std::string service,
      std::atomic<bool> &done,
      ResolveResult &out)
  {
    try
    {
      corosio::resolver r(ioc);

      auto [ec, results] = co_await r.resolve(host, service);

      if (ec)
      {
        out.error = Error{ErrorCode::resolve_failed};
        done.store(true, std::memory_order_release);
        co_return;
      }

      out.endpoints.clear();
      out.endpoints.reserve(results.size());

      for (auto const &entry : results)
      {
        auto ep = entry.get_endpoint();

        Endpoint e{};
        e.port = ep.port();

        if (ep.is_v4())
        {
          e.ip = IpVersion::v4;
          e.address = ep.v4_address().to_string();
        }
        else
        {
          e.ip = IpVersion::v6;
          e.address = ep.v6_address().to_string();
        }

        out.endpoints.push_back(std::move(e));
      }

      out.error = Error{ErrorCode::none};
    }
    catch (...)
    {
      out.error = Error{ErrorCode::unknown};
    }

    done.store(true, std::memory_order_release);
  }

  ResolveResult Resolver::resolve(std::string_view host, std::string_view service)
  {
    ResolveResult out{};
    out.error = Error{ErrorCode::unknown};

    if (!impl_ || !impl_->ioc)
    {
      out.error = Error{ErrorCode::not_initialized};
      return out;
    }

    std::atomic<bool> done{false};

    capy::run_async(impl_->ioc->get_executor())(
        resolve_task(*impl_->ioc,
                     std::string(host),
                     std::string(service),
                     done,
                     out));

    while (!done.load(std::memory_order_acquire))
    {
      impl_->ioc->run_one();
    }

    return out;
  }

} // namespace vix::net_corosio
