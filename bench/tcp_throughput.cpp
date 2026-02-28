#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/listener.hpp>
#include <vix/net_corosio/socket.hpp>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace vix::net_corosio;

namespace vix::net_corosio::bench
{
  struct ThroughputResult final
  {
    std::uint64_t bytes_total{0};
    std::uint64_t seconds{0};
  };

  static void server_worker(std::uint16_t port, std::atomic<bool> &ready, std::atomic<bool> &stop_flag, ThroughputResult &out)
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

    std::vector<std::uint8_t> buffer(64 * 1024);

    const auto start = std::chrono::steady_clock::now();

    std::uint64_t total = 0;

    while (!stop_flag.load(std::memory_order_acquire))
    {
      auto r = client.read_some(buffer.data(), buffer.size());
      if (!r.ok() || r.bytes == 0)
        break;

      total += static_cast<std::uint64_t>(r.bytes);
    }

    const auto end = std::chrono::steady_clock::now();
    const auto sec = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    out.bytes_total = total;
    out.seconds = static_cast<std::uint64_t>(sec);

    client.close();
    listener.close();
  }

  static void client_worker(std::string host, std::uint16_t port, std::chrono::seconds duration)
  {
    Context ctx;
    Socket sock(ctx);

    TcpEndpoint ep{};
    ep.address = std::move(host);
    ep.port = port;

    if (sock.connect(ep))
      return;

    std::vector<std::uint8_t> buffer(64 * 1024, 0xAB);

    const auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < duration)
    {
      auto w = sock.write_some(buffer.data(), buffer.size());
      if (!w.ok())
        break;
      if (w.bytes == 0)
        break;
    }

    sock.close();
  }

  static int run_tcp_throughput()
  {
    constexpr std::uint16_t port = 19081;
    constexpr auto duration = std::chrono::seconds(3);

    std::atomic<bool> ready{false};
    std::atomic<bool> stop_flag{false};

    ThroughputResult result{};

    std::thread server([&]
                       { server_worker(port, ready, stop_flag, result); });

    while (!ready.load(std::memory_order_acquire))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::thread client([&]
                       { client_worker("127.0.0.1", port, duration); });

    std::this_thread::sleep_for(duration);
    stop_flag.store(true, std::memory_order_release);

    client.join();
    server.join();

    const double sec = (result.seconds == 0) ? duration.count() : static_cast<double>(result.seconds);
    const double mb = static_cast<double>(result.bytes_total) / (1024.0 * 1024.0);
    const double mbps = mb / sec;

    std::cout << "[tcp_throughput]\n";
    std::cout << "  bytes: " << result.bytes_total << "\n";
    std::cout << "  seconds: " << sec << "\n";
    std::cout << "  throughput: " << mbps << " MiB/s\n";

    return 0;
  }

} // namespace vix::net_corosio::bench

int main()
{
  return vix::net_corosio::bench::run_tcp_throughput();
}
