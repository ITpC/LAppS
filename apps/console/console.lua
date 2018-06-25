console = {}
console.__index = console;

maps=require("console_maps");
local methods=require("console_methods");


console["onStart"]=function()
  local result, err=bcast:create(100);
end

console["onShutdown"]=function()
end

console["onDisconnect"]=function(handler)
  bcast:unsubscribe(100,handler);
  nljson.erase(maps.keys, tostring(handler))
end

console["method_not_found"] = function(handler)
  local err_msg=nljson.decode([[{
    "status" : 0,
    "error" : {
      "code" : -32601,
      "message": "No such method"
    },
    "cid" : 0
  }]]);
  ws:send(handler,err_msg); 
  ws:close(handler,1003); -- WebSocket close code here. 1003 - "no comprende", 
                          -- lets close clients which asking for wrong methods
end



--[[
--  @param handler - io handler to use for ws:send and ws:close
--  @param msg_type - helper enum defining type of message:
--    1 - Client Notification Message as defined in LAppS Specification 1.0 (without params)
--    2 - Client Notification Message with params attribute
--    3 - Request as defined in LAppS Specification 1.0 (without params)
--    4 - Request with params attribute
--  @param message - an nljson userdata object
--]]
console["onMessage"]=function(handler,msg_type, message)
  local switch={
    [1] = function() -- a CN without params 
            local method=methods._cn_wo_params_method[message.method] or echo.method_not_found;
          end,
    [2] = function() -- a CN with params 
            local method=methods._cn_w_params_method[message.method] or echo.method_not_found;
            method(handler,message.params,message.cid);
          end,
    [3] = function() -- request without params
            local method=methods._request_wo_params_method[message.method] or echo.method_not_found;
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

return console;
