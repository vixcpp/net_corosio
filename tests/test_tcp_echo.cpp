#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/listener.hpp>
#include <vix/net_corosio/socket.hpp>

#include <cassert>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace vix::net_corosio;

namespace
{
  constexpr std::uint16_t kTestPort = 19080;

  static void sleep_short()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  static void fail(const char *what, Error e)
  {
    std::cerr << "[tcp_echo] " << what << " failed: " << to_string(e.value()) << "\n";
    std::abort();
  }

  static void require_ok(const char *what, Error e)
  {
    if (e)
      fail(what, e);
  }

  static void require_ok(const char *what, const IoResult &r)
  {
    if (!r.ok())
      fail(what, r.error);
  }

  static bool is_retryable(Error e)
  {
    const auto c = e.value();
    return c == ErrorCode::invalid_state || c == ErrorCode::timeout;
  }

  static IoResult read_some_retry(Socket &s, void *buf, std::size_t n)
  {
    for (int i = 0; i < 800; ++i)
    {
      auto r = s.read_some(buf, n);
      if (r.ok() && r.bytes > 0)
        return r;

      if (!r.ok() && !is_retryable(r.error))
        return r;

      sleep_short();
    }
    return s.read_some(buf, n);
  }

  class ContextPump final
  {
  public:
    explicit ContextPump(Context &ctx)
        : ctx_(ctx), t_([this]
                        { loop(); })
    {
    }

    void stop()
    {
      ctx_.stop();
      if (t_.joinable())
        t_.join();
    }

    ~ContextPump()
    {
      stop();
    }

  private:
    void loop()
    {
      while (!ctx_.stop_requested())
      {
        (void)ctx_.run();
        sleep_short();
      }
    }

    Context &ctx_;
    std::thread t_;
  };

  void run_server_once(std::atomic<bool> &ready)
  {
    Context ctx;
    ContextPump pump(ctx);

    Listener listener(ctx);
    require_ok("listener.open", listener.open());
    require_ok("listener.bind", listener.bind(kTestPort));
    require_ok("listener.listen", listener.listen(1));

    ready.store(true, std::memory_order_release);

    auto accepted = listener.accept();
    if (!accepted.ok())
      fail("listener.accept", accepted.error);

    Socket &client = accepted.socket;

    std::vector<char> buffer(4096);

    auto r = read_some_retry(client, buffer.data(), buffer.size());
    require_ok("server.read_some", r);
    assert(r.bytes > 0);

    auto w = client.write_some(buffer.data(), r.bytes);
    require_ok("server.write_some", w);

    client.close();
    listener.close();

    pump.stop();
  }

  void run_client_once()
  {
    Context ctx;
    ContextPump pump(ctx);

    Socket sock(ctx);

    TcpEndpoint ep{};
    ep.address = "127.0.0.1";
    ep.port = kTestPort;

    require_ok("client.connect", sock.connect(ep));

    const std::string msg = "hello from client\n";

    auto w = sock.write_some(msg.data(), msg.size());
    require_ok("client.write_some", w);

    std::vector<char> buffer(4096);

    auto r = read_some_retry(sock, buffer.data(), buffer.size());
    require_ok("client.read_some", r);
    assert(r.bytes == msg.size());

    std::string echoed(buffer.data(), r.bytes);
    assert(echoed == msg);

    sock.close();

    pump.stop();
  }
} // namespace

int main()
{
  std::atomic<bool> ready{false};

  std::thread server_thread([&]
                            { run_server_once(ready); });

  for (int i = 0; i < 400; ++i)
  {
    if (ready.load(std::memory_order_acquire))
      break;
    sleep_short();
  }

  run_client_once();

  server_thread.join();

  std::cout << "[test_tcp_echo] OK\n";
  return 0;
}
