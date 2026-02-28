#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace corosio = boost::corosio;
namespace capy = boost::capy;

namespace vix::net_corosio::example
{
  static int run_dns_lookup(std::string host, std::string service)
  {
    using namespace vix::net_corosio;

    Context ctx;

    auto *ioc = static_cast<corosio::io_context *>(ctx.native_handle());
    if (!ioc)
    {
      std::cerr << "[dns_lookup] ctx.native_handle() returned null\n";
      return 1;
    }

    corosio::resolver res(*ioc);

    std::atomic<bool> done{false};
    Error out{ErrorCode::unknown};

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await res.resolve(host, service);
        (void)r;

        out = Error{ErrorCode::none};
      }
      catch (...)
      {
        out = Error{ErrorCode::unknown};
      }

      done.store(true, std::memory_order_release);
    };

    capy::run_async(ioc->get_executor())(task());

    while (!done.load(std::memory_order_acquire))
    {
      ioc->run_one();
    }

    if (out)
    {
      std::cerr << "[dns_lookup] resolve failed: " << static_cast<int>(out.code) << "\n";
      return 1;
    }

    std::cout << "[dns_lookup] resolved\n";
    return 0;
  }

} // namespace vix::net_corosio::example

int main(int argc, char **argv)
{
  std::string host = "example.com";
  std::string service = "https";

  if (argc >= 2)
    host = argv[1];
  if (argc >= 3)
    service = argv[2];

  return vix::net_corosio::example::run_dns_lookup(std::move(host), std::move(service));
}
