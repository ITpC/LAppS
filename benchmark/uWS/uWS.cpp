#include "../src/uWS.h"
#include <thread>
#include <stdio.h>
#include <string.h>

void server_thread()
{
    uWS::Hub h;
    uS::TLS::Context tls = uS::TLS::createContext ("./cert.pem", "./key.pem", "");

    h.onMessage([](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        ws->send(message, length, opCode);
    });

    h.listen(5083,tls);
    h.run();
}
int main()
{
  std::thread thr1(server_thread);
  int c=getchar();
  
}
