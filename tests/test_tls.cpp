#include <vix/net_corosio/context.hpp>
#include <vix/net_corosio/error.hpp>
#include <vix/net_corosio/socket.hpp>
#include <vix/net_corosio/tls_context.hpp>
#include <vix/net_corosio/tls_stream.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace vix::net_corosio;

namespace
{
  static void skip(const char *what, Error e)
  {
    std::cout << "[test_tls] SKIP: " << what << ": " << to_string(e.value()) << "\n";
  }

  void test_tls_handshake_example_com()
  {
    Context ctx;

    Socket sock(ctx);

    TcpEndpoint ep{};
    ep.address = "example.com";
    ep.port = 443;

    const Error e_conn = sock.connect(ep);
    if (e_conn)
    {
      skip("connect", e_conn);
      return;
    }

    TlsContext tls_ctx(TlsRole::client);

    const Error e_paths = tls_ctx.set_default_verify_paths();
    if (e_paths)
    {
      skip("set_default_verify_paths", e_paths);
      return;
    }

    const Error e_verify = tls_ctx.set_verify_mode(TlsVerifyMode::peer);
    if (e_verify)
    {
      skip("set_verify_mode", e_verify);
      return;
    }

    const Error e_sni = tls_ctx.set_hostname("example.com");
    if (e_sni)
    {
      skip("set_hostname", e_sni);
      return;
    }

    {
      std::vector<std::string> protos;
      protos.emplace_back("h2");
      protos.emplace_back("http/1.1");
      (void)tls_ctx.set_alpn(protos);
    }

    TlsStream tls(sock, tls_ctx);

    const Error e_hs = tls.handshake();
    if (e_hs)
    {
      skip("handshake", e_hs);
      tls.close();
      return;
    }

    const std::string req =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: vix-net_corosio-test/0.1\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n";

    const auto w = tls.write_some(req.data(), req.size());
    assert(w.ok());
    assert(w.bytes > 0);

    std::vector<char> buf(8192);
    std::size_t total = 0;

    for (;;)
    {
      const auto r = tls.read_some(buf.data(), buf.size());
      if (!r.ok())
        break;
      if (r.bytes == 0)
        break;
      total += r.bytes;
      if (total >= 128)
        break;
    }

    assert(total > 0);

    (void)tls.shutdown();
    tls.close();

    std::cout << "[test_tls] handshake + basic IO OK\n";
  }
} // namespace

int main()
{
  test_tls_handshake_example_com();
  return 0;
}
