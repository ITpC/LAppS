bitfinex={}
bitfinex.__index=bitfinex

bitfinex.init=function()
end


bitfinex.run=function()
  local websocket,errmsg=cws:new(
  "wss://api.bitfinex.com/ws/2",
  {
    onopen=function(handler)
      print("onopen")
      local result, errstr=cws:send(handler,[[{ "event" : "subscribe", "channel": "ticker", "symbol": "BTCUSD"}]],1);
      if(result == nil)
      then
        print("Error on websocket send at handler "..handler..": "..errstr);
      else
        print("subscription sent");
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

return bitfinex;

