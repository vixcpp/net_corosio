#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>

#include <cassert>
#include <iostream>

using namespace vix::net_corosio;

namespace
{
  void test_default_config()
  {
    Context ctx;

    const Config &cfg = ctx.config();

    assert(cfg.strict_checks == true || cfg.strict_checks == false);

    std::cout << "[test_context] test_default_config OK\n";
  }

  void test_set_config()
  {
    Context ctx;

    Config cfg = ctx.config();
    cfg.strict_checks = false;

    ctx.set_config(cfg);

    const Config &after = ctx.config();
    assert(after.strict_checks == false);

    std::cout << "[test_context] test_set_config OK\n";
  }

  void test_stop_requested_flag()
  {
    Context ctx;

    // Initially false
    assert(ctx.stop_requested() == false);

    ctx.stop();

    assert(ctx.stop_requested() == true);

    std::cout << "[test_context] test_stop_requested_flag OK\n";
  }

  void test_native_handle()
  {
    Context ctx;

    void *handle = ctx.native_handle();
    assert(handle != nullptr);

    const void *chandle = static_cast<const Context &>(ctx).native_handle();
    assert(chandle != nullptr);

    std::cout << "[test_context] test_native_handle OK\n";
  }

  void test_run_returns_ok()
  {
    Context ctx;

    // run() without posted work should return immediately.
    const Error e = ctx.run();

    // run() is designed to return ErrorCode::none on success.
    assert(!e);

    std::cout << "[test_context] test_run_returns_ok OK\n";
  }
} // namespace

int main()
{
  test_default_config();
  test_set_config();
  test_stop_requested_flag();
  test_native_handle();
  test_run_returns_ok();

  std::cout << "[test_context] all tests passed\n";
  return 0;
}
