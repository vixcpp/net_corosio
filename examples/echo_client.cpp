#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/socket.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace vix::net_corosio::example
{
  static int run_echo_client(std::string host, std::uint16_t port)
  {
    using namespace vix::net_corosio;

    Context ctx;

    Socket sock(ctx);

    {
      TcpEndpoint ep{};
      ep.address = std::move(host);
      ep.port = port;

      const Error e = sock.connect(ep);
      if (e)
      {
        std::cerr << "[echo_client] connect failed: " << static_cast<int>(e.code) << "\n";
        return 1;
      }
    }

    std::cout << "[echo_client] connected. type lines, press Enter. Ctrl+D to quit.\n";

    std::string line;
    std::vector<char> buf(16 * 1024);

    while (std::getline(std::cin, line))
    {
      line.push_back('\n');

      {
        const auto w = sock.write_some(line.data(), line.size());
        if (!w.ok())
        {
          std::cerr << "[echo_client] write failed: " << static_cast<int>(w.error.code) << "\n";
          sock.close();
          return 1;
        }
      }

      // Read back one chunk (echo server echoes what it receives).
      const auto r = sock.read_some(buf.data(), buf.size());
      if (!r.ok())
      {
        std::cerr << "[echo_client] read failed: " << static_cast<int>(r.error.code) << "\n";
        sock.close();
        return 1;
      }

      if (r.bytes == 0)
      {
        std::cerr << "[echo_client] server closed connection\n";
        sock.close();
        return 0;
      }

      std::cout.write(buf.data(), static_cast<std::streamsize>(r.bytes));
      std::cout.flush();
    }

    sock.close();
    return 0;
  }

} // namespace vix::net_corosio::example

int main(int argc, char **argv)
{
  std::string host = "127.0.0.1";
  std::uint16_t port = 8080;

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
      // keep default
    }
  }

  return vix::net_corosio::example::run_echo_client(host, port);
}
