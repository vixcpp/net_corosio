#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>

#include <boost/corosio.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace corosio = boost::corosio;
namespace capy = boost::capy;

using namespace vix::net_corosio;

namespace
{
  template <class T>
  concept has_error_method = requires(const T &t) { t.error(); };

  template <class T>
  std::error_code io_error(const T &r)
  {
    if constexpr (has_error_method<T>)
      return r.error();
    return {};
  }

  void test_resolve_example_com_https()
  {
    Context ctx;

    auto *ioc = static_cast<corosio::io_context *>(ctx.native_handle());
    assert(ioc != nullptr);

    corosio::resolver res(*ioc);

    std::atomic<bool> done{false};
    Error out{ErrorCode::unknown};

    std::vector<corosio::endpoint> endpoints;

    auto task = [&]() -> capy::task<void>
    {
      try
      {
        auto r = co_await res.resolve("example.com", "https");

        const auto ec = io_error(r);
        if (ec)
        {
          out = Error{ErrorCode::resolve_failed};
          done.store(true, std::memory_order_release);
          co_return;
        }

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

    assert(!out);

    (void)endpoints;

    std::cout << "[test_resolver] resolve example.com:https OK\n";
  }
} // namespace

int main()
{
  test_resolve_example_com_https();
  std::cout << "[test_resolver] all tests passed\n";
  return 0;
}
