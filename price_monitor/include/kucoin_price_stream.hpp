// Copyright (C) 2023 Joshua and Jordan Ogunyinka
#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "json_utils.hpp"
#include "price_stream/commodity.hpp"
#include "uri.hpp"
#include <optional>
#include <set>

namespace boost::asio {
namespace ssl {
class context;
}
class io_context;
} // namespace boost::asio

namespace keep_my_journal {
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace ws = beast::websocket;
namespace ip = net::ip;

class https_rest_api_t;

// kucoin price stream websocket
class kucoin_price_stream_t
    : public std::enable_shared_from_this<kucoin_price_stream_t> {
  struct instance_server_data_t {
    std::string endpoint; // wss://foo.com/path
    int pingIntervalMs = 0;
    int pingTimeoutMs = 0;
    int encryptProtocol = 0; // bool encrypt or not
  };

  using resolver = ip::tcp::resolver;
  using results_type = resolver::results_type;

  net::io_context &m_ioContext;
  ssl::context &m_sslContext;
  std::optional<resolver> m_resolver = std::nullopt;
  std::optional<ws::stream<beast::ssl_stream<beast::tcp_stream>>>
      m_sslWebStream = std::nullopt;
  std::optional<beast::flat_buffer> m_readWriteBuffer = std::nullopt;
  std::optional<net::deadline_timer> m_pingTimer;
  std::unique_ptr<https_rest_api_t> m_httpClient = nullptr;
  std::string m_apiHost;
  std::string m_apiService;
  std::string m_requestToken;
  std::string m_subscriptionString;
  std::vector<instance_server_data_t> m_instanceServers;
  uri_t m_uri;
  trade_type_e const m_tradeType;

public:
  kucoin_price_stream_t(net::io_context &ioContext, ssl::context &sslContext,
                        trade_type_e);
  virtual ~kucoin_price_stream_t() = default;
  void run();

protected:
  instrument_sink_t::list_t &m_tradedInstruments;
  bool m_tokensSubscribedFor = false;

  virtual void reset_counter() { m_tokensSubscribedFor = false; }
  virtual std::string rest_api_host() const = 0;
  virtual std::string rest_api_service() const = 0;
  virtual std::string rest_api_target() const = 0;
  virtual std::string get_subscription_json() = 0;
  virtual void on_instruments_received(std::string const &) = 0;

private:
  void rest_api_initiate_connection();
  void rest_api_obtain_token();
  void start_ping_timer();
  void reset_ping_timer();
  void on_ping_timer_tick(boost::system::error_code const &);
  void report_error_and_retry(beast::error_code);
  void negotiate_websocket_connection();
  void initiate_websocket_connection();
  void websocket_perform_ssl_handshake();
  void websocket_connect_to_resolved_names(results_type const &);
  void perform_websocket_handshake();
  void wait_for_messages();
  void send_ticker_subscription();
  void interpret_generic_messages();
  void on_token_obtained(std::string const &token);
};

class kucoin_spot_price_stream_t : public kucoin_price_stream_t {
public:
  kucoin_spot_price_stream_t(net::io_context &, ssl::context &);

  std::string rest_api_host() const override { return "api.kucoin.com"; }
  std::string rest_api_service() const override { return "https"; }
  std::string rest_api_target() const override {
    return "/api/v1/market/allTickers";
  }

  void on_instruments_received(std::string const &) override;
  std::string get_subscription_json() override;
};

class kucoin_futures_price_stream_t : public kucoin_price_stream_t {
  std::vector<instrument_type_t> m_fInstruments;
  size_t m_tokenCounter;

public:
  kucoin_futures_price_stream_t(net::io_context &, ssl::context &);
  std::string rest_api_host() const override {
    return "api-futures.kucoin.com";
  }
  std::string rest_api_service() const override { return "https"; }
  std::string rest_api_target() const override {
    return "/api/v1/contracts/active";
  }
  void on_instruments_received(std::string const &) override;
  std::string get_subscription_json() override;
  void reset_counter() override;
};
} // namespace keep_my_journal
