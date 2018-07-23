/*
 * Copyright (c) 2016, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <utility>
#include <thread>
#include <future>
#include <functional>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

context_ptr on_tls_init(websocketpp::connection_hdl hdl)
{
  std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
  context_ptr ctx =
     websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::tlsv12_client);

  try {
      ctx->set_options(boost::asio::ssl::context::default_workarounds |
                       boost::asio::ssl::context::no_sslv2 |
                       boost::asio::ssl::context::no_sslv3 |
                       boost::asio::ssl::context::no_tlsv1 |
                       boost::asio::ssl::context::no_tlsv1_1 |
                       boost::asio::ssl::context::tlsv12 |
                       boost::asio::ssl::context::single_dh_use);
      ctx->set_verify_mode( 0 );
  } catch (std::exception& e) {
      std::cout << e.what() << std::endl;
  }
  return ctx;
}


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

size_t om_counter=0;
size_t nm_counter=0;

uint64_t start=0;
uint64_t slice=0;

const uint64_t time_now()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()
  ).count();
}

json echo_msg="{\
        \"lapps\" : 1,\
        \"method\": \"echo\",\
        \"params\": [\
          { \"authkey\" : 0 },\
          [] \
        ]\
      }\
"_json;


json subscribe="{ \"lapps\" : 1, \"method\": \"subscribe\", \"params\": [ { \"authkey\": 0 } ], \"cid\": 5 }"_json;



// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

std::string blob(1024,'x');
std::string blob1(128,'x');

std::vector<uint8_t>  out_echo_msg;

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
{

    websocketpp::lib::error_code ec;
    om_counter++;

    json response=json::from_cbor(msg->get_payload());
    
    if(response["cid"] == 0)
    {
      if(response["status"] == 1) // logged in or response
      {
        if((om_counter == 1)&&(echo_msg["params"][0]["authkey"] == 0)) // login
        {
          echo_msg["params"][0]["authkey"]=static_cast<size_t>(response["result"][0]["authkey"]);
          echo_msg["params"][0]["echo"]=blob1;
          out_echo_msg=json::to_cbor(echo_msg);
          subscribe["params"][0]["authkey"]=echo_msg["params"][0]["authkey"];
          subscribe["params"][1]=blob;
          auto  subscribe_msg=json::to_cbor(subscribe);
          c->send(hdl, subscribe_msg.data(), subscribe_msg.size(), websocketpp::frame::opcode::binary, ec);
        }
      }
      else // error
      {
        std::cout << "Error: " << response << std::endl;
      }
    }

    slice=time_now();
    if((slice-start)>=1000)
    {
      std::cout << "Messages: " << om_counter << " per " << slice - start << "ms" << std::endl;
      start=time_now();
      om_counter=0;
    }


    if(response["cid"] == 5) //oon time pulse
    {
      std::cout << "Counter on pulse: " << om_counter << " per " << slice - start << "ms" << std::endl;
    }

    if(response["cid"] == 3 ) // error
    {
      std::cout << "Error: " << response << std::endl;
    }

    if(echo_msg["params"][0]["authkey"] != 0)
    {
      c->send(hdl, out_echo_msg.data(), out_echo_msg.size(), websocketpp::frame::opcode::binary, ec);
      if (ec) {
          std::cout << "Echo failed because: " << ec.message() << std::endl;
      }
    }
}

void on_open(client* c, websocketpp::connection_hdl hdl)
{
  websocketpp::lib::error_code ec;
  json login({
    {"lapps",1},
    {"method", "login"},
    { "params", {{{"user", "admin"},{"password", "admin"}}}}
  });

  auto msg=json::to_cbor(login);

  c->send(hdl,msg.data(),msg.size(), websocketpp::frame::opcode::binary,ec);

  if(ec)
  {
    std::cout << "Can't send a message: " << ec.message() << std::endl;
  }

}

int main(int argc, char* argv[]) {
    // Create a client endpoint
    client c;

    websocketpp::transport::asio::tls_socket::tls_init_handler f=on_tls_init;
    c.set_tls_init_handler(f);


    std::string uri = "wss://localhost:5083/echo_lapps";

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        // Set logging to be pretty verbose (everything except message payloads)
        //c.set_access_channels(websocketpp::log::alevel::all);
        //c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none);

        // Initialize ASIO
        c.init_asio();

        // Register our message handler
        c.set_message_handler(bind(&on_message,&c,::_1,::_2));
        c.set_open_handler(bind(&on_open,&c,::_1));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.

        start=time_now();
        slice=time_now();
        c.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}
