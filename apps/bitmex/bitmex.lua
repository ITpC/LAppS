bitmex={}
bitmex.__index=bitmex

bitmex.init=function()
end

bitmex.run=function()
  local websocket,errmsg=cws:new(
  "wss://www.bitmex.com/realtime",
  {
    onopen=function(handler)
      print("onopen")
      local result, errstr=cws:send(handler,[[{"op": "subscribe", "args": ["orderBookL2:XBTUSD"]}]],1);
      if(not result)
      then
        print("Error on websocket send at handler "..handler..": "..errstr);
      end
    end,
    onmessage=function(handler,message,opcode)
      print(message)
    end,
    onerror=function(handler, message)
      print(message..". Socket FD:  "..handler);
    end,
    onclose=function(handler)
      print("WebSocket "..handler.." is closed by peer.");
    end
  });
  if(websocket == nil)
  then
   print(errmsg) 
  else
    while not must_stop()
    do
      cws:eventLoop();
    end
  end
end

return bitmex;

