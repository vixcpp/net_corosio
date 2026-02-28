#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/listener.hpp>
#include <vix/net_corosio/socket.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace vix::net_corosio;

namespace vix::net_corosio::bench
{
  static void server_pingpong(std::uint16_t port, std::atomic<bool> &ready, std::atomic<bool> &stop_flag)
  {
    Context ctx;

    Listener listener(ctx);
    if (listener.open())
      return;
    if (listener.bind(port))
      return;
    if (listener.listen(1))
      return;

    ready.store(true, std::memory_order_release);

    auto accepted = listener.accept();
    if (!accepted.ok())
      return;

    Socket &client = accepted.socket;

    std::uint8_t b = 0;

    while (!stop_flag.load(std::memory_order_acquire))
    {
      auto r = client.read_some(&b, 1);
      if (!r.ok() || r.bytes == 0)
        break;

      auto w = client.write_some(&b, 1);
      if (!w.ok() || w.bytes == 0)
        break;
    }

    client.close();
    listener.close();
  }

  static int run_tcp_latency()
  {
    constexpr std::uint16_t port = 19082;
    constexpr int iters = 2000;

    std::atomic<bool> ready{false};
    std::atomic<bool> stop_flag{false};

    std::thread server([&]
                       { server_pingpong(port, ready, stop_flag); });

    while (!ready.load(std::memory_order_acquire))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    Context ctx;
    Socket sock(ctx);

    TcpEndpoint ep{};
    ep.address = "127.0.0.1";
    ep.port = port;

    if (sock.connect(ep))
    {
      stop_flag.store(true, std::memory_order_release);
      server.join();
      std::cerr << "[tcp_latency] connect failed\n";
      return 1;
    }

    std::vector<double> rtts_us;
    rtts_us.reserve(static_cast<std::size_t>(iters));

    std::uint8_t b = 0x7F;

    for (int i = 0; i < iters; ++i)
    {
      const auto t0 = std::chrono::steady_clock::now();

      auto w = sock.write_some(&b, 1);
      if (!w.ok() || w.bytes != 1)
        break;

      auto r = sock.read_some(&b, 1);
      if (!r.ok() || r.bytes != 1)
        break;

      const auto t1 = std::chrono::steady_clock::now();
      const auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
      rtts_us.push_back(static_cast<double>(us));
    }

    sock.close();

    stop_flag.store(true, std::memory_order_release);
    server.join();

    if (rtts_us.empty())
    {
      std::cerr << "[tcp_latency] no samples\n";
      return 1;
    }

    std::sort(rtts_us.begin(), rtts_us.end());

    auto pct = [&](double p) -> double
    {
      if (rtts_us.empty())
        return 0.0;
      const double idx = (p / 100.0) * (static_cast<double>(rtts_us.size() - 1));
      const std::size_t i = static_cast<std::size_t>(idx);
      return rtts_us[i];
    };

    const double p50 = pct(50.0);
    const double p90 = pct(90.0);
    const double p99 = pct(99.0);
    const double minv = rtts_us.front();
    const double maxv = rtts_us.back();

    std::cout << "[tcp_latency]\n";
    std::cout << "  samples: " << rtts_us.size() << "\n";
    std::cout << "  min(us): " << minv << "\n";
    std::cout << "  p50(us): " << p50 << "\n";
    std::cout << "  p90(us): " << p90 << "\n";
    std::cout << "  p99(us): " << p99 << "\n";
    std::cout << "  max(us): " << maxv << "\n";

    return 0;
  }

} // namespace vix::net_corosio::bench

int main()
{
  return vix::net_corosio::bench::run_tcp_latency();
}
