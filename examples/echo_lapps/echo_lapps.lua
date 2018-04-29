echo = {}
echo.__index = echo;

maps=require("lapps_echo_maps");
local methods=require("lapps_echo_methods");

echo["onStart"]=function()
  print("echo::onStart");
end

echo["onShutdown"]=function()
  print("echo::onShutdown()");
end

echo["onDisconnect"]=function(handler)
  nljson.erase(maps.keys, tostring(handler))
  print("Disconnected: "..handler);
end

echo["method_not_found"] = function(handler)
  local err_msg=nljson.decode([[{
    "status" : 0,
    "error" : {
      "code" : -32601,
      "message": "No such method"
    },
    "cid" : 0 -- we deliver the error message to CCH
  }]]);
  ws:send(handler,err_msg); 
  ws:close(handler,1003); -- WebSocket close code here. 1003 - "no comprende", 
                          -- lets close clients which asking for wrong methods
end



--[[
--  @param handler - io handler to use for ws:send and ws:close
--  @param msg_type - helper enum defining type of message:
--    1 - Client Notification Message as defined in LAppS Specification 1.0
--    2 - Client Notification Message which has params attribute
--    3 - Request as defined in LAppS Specification 1.0
--    4 - Request wich has params attribute
--  @param message - an nljson userdata object
--]]
echo["onMessage"]=function(handler,msg_type, message)
  local switch={
    [1] = function() -- a CN message does not require any response. Let's restrict CN's without params
            local err_msg=nljson.decode([[{
              "status" : 0,
              "error" : {
                "code" : -32600,
                "message": "This server does not accept Client Notifications"
              },
              "cid" : 0 -- we deliverthe error message to CCH
            }]]);
            ws:send(handler,err_msg); 
            ws:close(handler,1003); -- WebSocket close code here. 1003 - "no comprende"
          end,
    [2] = function() -- a CN with params,  does not require any response.
            local method=methods._cn_w_params_method[message.method] or echo.method_not_found;
            method(handler,message.params);
          end,
    [3] = function() -- requests without params are unsupported by this app
            local method=echo.method_not_found;
            method(handler);
          end,
    [4] = function()
            local method=methods._request_w_params_method[message.method] or echo.method_not_found;
            method(handler,message.params);
          end
  }
  switch[msg_type]();

  return true;
end

return echo;
