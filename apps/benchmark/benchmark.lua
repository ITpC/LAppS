benchmark={}
benchmark.__index=benchmark

benchmark.init=function()
end

benchmark.messages_counter=0;
benchmark.start_time=time.now();

benchmark.meter=function()
  benchmark.messages_counter=benchmark.messages_counter+1;

  local slice=time.now() - benchmark.start_time;
  if( slice  >= 1000)
  then
    print(benchmark.messages_counter.." messages receved per "..slice.."ms")
    benchmark.messages_counter=0;
    benchmark.start_time=time.now();
  end
end

benchmark.run=function()
  local n=nap:new()
  n:sleep(3);
  print("Start");
  local array={};
  for i=0,100
  do
   local sock, err_msg=cws:new(
      "wss://127.0.0.1:5083/echo",
      {
        ["onopen"]=function(handler)
          local result, errstr=cws:send(handler,"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",2);
          if(not result)
          then
            print("Error on websocket send at handler "..handler..": "..errstr);
          end
        end,
        ["onmessage"]=function(handler,message,opcode)
          benchmark.meter();
          cws:send(handler,message,opcode);
        end,
        ["onerror"]=function(handler, message)
          print(message..". Socket FD:  "..handler);
        end,
        ["onclose"]=function(handler)
          print("WebSocket "..handler.." is closed by peer.");
        end
      });
      if(sock == nil)
      then
        print("Socket is not created: "..err_msg)
      else
        table.insert(array,sock);
      end
    cws:eventLoop();
  end
  print("Sockets connected: "..#array);
  benchmark.start_time=time.now();
  while not must_stop()
  do
    cws:eventLoop();
  end
end

return benchmark;

