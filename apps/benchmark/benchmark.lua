benchmark={}
benchmark.__index=benchmark

benchmark.init=function()
  print("instance id: "..instance_id);
end

benchmark.messages_counter=0;
benchmark.start_time=time.now();
benchmark.max_connections=800;
benchmark.target="wss://127.0.0.1:5083/echo";
benchmark.message="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
benchmark.force_stop=true;

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
  local n=nap:new();
  local counter=1;
  n:sleep(1);
  local array={};
  local start=time.now();
  while(#array < benchmark.max_connections) and (not must_stop())
  do
    local sock, err_msg=cws:new(benchmark.target,
    {
      onopen=function(handler)
        local result, errstr=cws:send(handler,benchmark.message,2);
        if(not result)
        then
          print("Error on websocket send at handler "..handler..": "..errstr);
        end
      end,
      onmessage=function(handler,message,opcode)
        benchmark.meter();
        cws:send(handler,message,opcode);
      end,
      onerror=function(handler, message)
        print("Client WebSocket connection is failed for socketfd "..handler..". Error: "..message);
      end,
      onclose=function(handler)
        print("Connection is closed for socketfd "..handler);
      end
    });

    if(sock ~= nil)
    then
      table.insert(array,sock);
    else
      print(err_msg);
      err_msg=nil;
      collectgarbage("collect");
    end

    -- poll events once per 10 outgoing connections 
    -- this will improve the connection establishment speed
    if counter == 10 and (not must_stop())
    then
      cws:eventLoop();
      counter=1
    else
      counter = counter + 1;
    end
  end

  print("Sockets connected: "..#array);
  benchmark.start_time=time.now();

  if(must_stop())
  then
    print("must stop")
  end

  while not must_stop()
  do
    cws:eventLoop();
  end

  if(not force_stop)
  then
    for i=1,#array
    do
      array[i]:close();
      cws:eventLoop();
    end
  end
end

return benchmark;

