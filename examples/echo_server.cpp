#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/listener.hpp>
#include <vix/net_corosio/socket.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace vix::net_corosio::example
{
  static int run_echo_server(std::uint16_t port)
  {
    using namespace vix::net_corosio;

    Context ctx;

    Listener listener(ctx);

    {
      const Error e = listener.open();
      if (e)
      {
        std::cerr << "[echo_server] open failed: " << static_cast<int>(e.code) << "\n";
        return 1;
      }
    }

    {
      const Error e = listener.bind(port);
      if (e)
      {
        std::cerr << "[echo_server] bind failed: " << static_cast<int>(e.code) << "\n";
        return 1;
      }
    }

    {
      const Error e = listener.listen(128);
      if (e)
      {
        std::cerr << "[echo_server] listen failed: " << static_cast<int>(e.code) << "\n";
        return 1;
      }
    }

    std::cout << "[echo_server] listening on 0.0.0.0:" << port << "\n";

    std::vector<std::uint8_t> buffer(16 * 1024);

    for (;;)
    {
      auto accepted = listener.accept();
      if (!accepted.ok())
      {
        std::cerr << "[echo_server] accept failed: " << static_cast<int>(accepted.error.code) << "\n";
        continue;
      }

      Socket &client = accepted.socket;

      for (;;)
      {
        auto r = client.read_some(buffer.data(), buffer.size());
        if (!r.ok())
        {
          // Client closed or IO error.
          client.close();
          break;
        }

        if (r.bytes == 0)
        {
          client.close();
          break;
        }

        auto w = client.write_some(buffer.data(), r.bytes);
        if (!w.ok())
        {
          client.close();
          break;
        }
      }
    }

    return 0;
  }

} // namespace vix::net_corosio::example

int main(int argc, char **argv)
{
  std::uint16_t port = 8080;

  if (argc >= 2)
  {
    try
    {
      const int p = std::stoi(argv[1]);
      if (p > 0 && p <= 65535)
        port = static_cast<std::uint16_t>(p);
    }
    catch (...)
    {
      // keep default
    }
  }

  return vix::net_corosio::example::run_echo_server(port);
}
