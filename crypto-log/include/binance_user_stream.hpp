#pragma once

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <memory>

#include "json_utils.hpp"
#include "request_handler.hpp"

namespace jordan {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace websock = beast::websocket;
namespace ip = net::ip;

// https://binance-docs.github.io/apidocs/spot/en/#user-data-streams
class binance_stream_t : public std::enable_shared_from_this<binance_stream_t> {
  using resolver = ip::tcp::resolver;
  using string_t = json::string_t;
  using inumber_t = json::number_integer_t;
  using fnumber_t = json::number_float_t;
  using ssl_websocket_stream_t =
      websock::stream<beast::ssl_stream<beast::tcp_stream>>;

  static char const *const ws_host;
  static char const *const ws_port_number;
  static char const *const rest_api_host;

  net::io_context &m_ioContext;
  net::ssl::context &m_sslContext;
  utils::waitable_container_t<user_stream_result_t> &m_results;
  user_exchange_info_t const m_userInfo;

  std::unique_ptr<ip::tcp::resolver> m_resolver = nullptr;
  std::unique_ptr<ssl_websocket_stream_t> m_sslWebStream = nullptr;
  std::unique_ptr<beast::flat_buffer> m_buffer = nullptr;
  std::unique_ptr<http::request<http::empty_body>> m_httpRequest = nullptr;
  std::unique_ptr<http::response<http::string_body>> m_httpResponse = nullptr;
  std::unique_ptr<net::high_resolution_timer> m_onErrorTimer = nullptr;
  std::unique_ptr<net::high_resolution_timer> m_listenKeyTimer = nullptr;
  std::unique_ptr<std::string> m_listenKey = nullptr;
  bool m_isStopped = false;

private:
  void rest_api_initiate_connection();
  void rest_api_connect_to(resolver::results_type const &);
  void rest_api_perform_ssl_connection(
      resolver::results_type::endpoint_type const &);
  void rest_api_get_listen_key();
  void rest_api_prepare_request();
  void rest_api_send_request();
  void rest_api_receive_response();
  void rest_api_on_data_received(beast::error_code const ec);
  void ws_initiate_connection();
  void ws_connect_to_names(resolver::results_type const &);
  void ws_perform_ssl_handshake(resolver::results_type::endpoint_type const &);
  void ws_upgrade_to_websocket();
  void ws_wait_for_messages();
  void ws_interpret_generic_messages();
  void ws_process_orders_execution_report(json::object_t const &);
  void ws_process_balance_update(json::object_t const &);
  void ws_process_account_position(json::object_t const &);
  void on_ws_connection_severed();
  void activate_listen_key_keepalive();
  void on_periodic_time_timeout();

public:
  binance_stream_t(net::io_context &, net::ssl::context &,
                   user_exchange_info_t const &userApiKey);
  ~binance_stream_t();
  void run();
  void stop();
};
} // namespace jordan
