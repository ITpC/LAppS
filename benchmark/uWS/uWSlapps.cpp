#include "../src/uWS.h"
#include <thread>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <json.hpp>

using json = nlohmann::json;


const uint64_t time_now()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()
  ).count();
}

void server_thread()
{
    uWS::Hub h;
    uS::TLS::Context tls = uS::TLS::createContext ("./cert.pem", "./key.pem", "");

    size_t start=time_now();
    size_t end=time_now();
    size_t counter=0;

    h.onMessage([&start,&end,&counter](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        std::vector<uint8_t> bindata(length);
        memcpy(bindata.data(),message,length);
        json msg=json::from_cbor(bindata);

        if(msg["method"] == std::string("login"))
        {
          json login_ok=R"({
            "status" : 1,
            "result" : [
              { "authkey" : 3244 }
          ], 
          "cid" : 0 
          })"_json;
          std::vector<uint8_t> out=json::to_cbor(login_ok);
          ws->send((const char*)(out.data()), out.size(), uWS::OpCode::BINARY);
        }
        else if(msg["method"] == std::string("echo"))
        {
          json echo=R"({ 
            "cid" : 0,
            "status" : 1,
            "result" : []
          })"_json;
          echo["result"]=msg["params"];
          std::vector<uint8_t> out=json::to_cbor(echo);
          ws->send((const char*)(out.data()), out.size(), uWS::OpCode::BINARY);
        }

        end=time_now();
        if((end-start) >= 1000)
        {
          std::cout << "Messages " << counter << " per " << (end - start) << "ms" << std::endl;
          start=time_now();
          counter=0;
        }
        else counter++;
    });

    h.listen(5083,tls);
    h.run();
}
int main()
{
  std::thread thr1(server_thread);
  int c=getchar();
  
}
