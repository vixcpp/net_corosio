#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/socket.hpp>
#include <vix/net_corosio/tls_context.hpp>
#include <vix/net_corosio/tls_stream.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace vix::net_corosio;

namespace vix::net_corosio::bench
{
  static int run_tls_handshake(std::string host, std::uint16_t port, int iters)
  {
    if (iters <= 0)
      iters = 20;

    std::vector<double> samples_ms;
    samples_ms.reserve(static_cast<std::size_t>(iters));

    for (int i = 0; i < iters; ++i)
    {
      Context ctx;

      Socket sock(ctx);

      TcpEndpoint ep{};
      ep.address = host;
      ep.port = port;

      const Error e_conn = sock.connect(ep);
      if (e_conn)
      {
        std::cerr << "[tls_handshake] connect failed: " << static_cast<int>(e_conn.code) << "\n";
        return 1;
      }

      TlsContext tls_ctx(TlsRole::client);

      // Best effort, keep strict by default.
      const Error e_paths = tls_ctx.set_default_verify_paths();
      if (e_paths)
      {
        std::cerr << "[tls_handshake] set_default_verify_paths failed: " << static_cast<int>(e_paths.code) << "\n";
        return 1;
      }

      const Error e_verify = tls_ctx.set_verify_mode(TlsVerifyMode::peer);
      if (e_verify)
      {
        std::cerr << "[tls_handshake] set_verify_mode failed: " << static_cast<int>(e_verify.code) << "\n";
        return 1;
      }

      const Error e_sni = tls_ctx.set_hostname(host);
      if (e_sni)
      {
        std::cerr << "[tls_handshake] set_hostname failed: " << static_cast<int>(e_sni.code) << "\n";
        return 1;
      }

      // ALPN optional (don't fail benchmark).
      {
        std::vector<std::string> protos;
        protos.emplace_back("h2");
        protos.emplace_back("http/1.1");
        (void)tls_ctx.set_alpn(protos);
      }

      TlsStream tls(sock, tls_ctx);

      const auto t0 = std::chrono::steady_clock::now();
      const Error e_hs = tls.handshake();
      const auto t1 = std::chrono::steady_clock::now();

      if (e_hs)
      {
        std::cerr << "[tls_handshake] handshake failed: " << static_cast<int>(e_hs.code) << "\n";
        return 1;
      }

      const auto ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
      samples_ms.push_back(ms);

      (void)tls.shutdown();
      tls.close();
    }

    std::sort(samples_ms.begin(), samples_ms.end());

    auto pct = [&](double p) -> double
    {
      if (samples_ms.empty())
        return 0.0;
      const double idx = (p / 100.0) * (static_cast<double>(samples_ms.size() - 1));
      return samples_ms[static_cast<std::size_t>(idx)];
    };

    const double minv = samples_ms.front();
    const double p50 = pct(50.0);
    const double p90 = pct(90.0);
    const double p99 = pct(99.0);
    const double maxv = samples_ms.back();

    std::cout << "[tls_handshake]\n";
    std::cout << "  target: " << host << ":" << port << "\n";
    std::cout << "  samples: " << samples_ms.size() << "\n";
    std::cout << "  min(ms): " << minv << "\n";
    std::cout << "  p50(ms): " << p50 << "\n";
    std::cout << "  p90(ms): " << p90 << "\n";
    std::cout << "  p99(ms): " << p99 << "\n";
    std::cout << "  max(ms): " << maxv << "\n";

    return 0;
  }

} // namespace vix::net_corosio::bench

int main(int argc, char **argv)
{
  std::string host = "example.com";
  std::uint16_t port = 443;
  int iters = 20;

  if (argc >= 2)
    host = argv[1];

  if (argc >= 3)
  {
    try
    {
      const int p = std::stoi(argv[2]);
      if (p > 0 && p <= 65535)
        port = static_cast<std::uint16_t>(p);
    }
    catch (...)
    {
    }
  }

  if (argc >= 4)
  {
    try
    {
      iters = std::stoi(argv[3]);
    }
    catch (...)
    {
    }
  }

  return vix::net_corosio::bench::run_tls_handshake(host, port, iters);
}
