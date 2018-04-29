lapps_echo_methods={}
lapps_echo_methods.__index=lapps_echo_methods

maps=require("lapps_echo_maps") -- must be global

lapps_echo_methods["_cn_w_params_method"]={
  ["logout"]=function(handler,params)
    if(nljson.find(params[1],"authkey") ~= nil) and (type(params[1].authkey) == "number")
    then
    -- ticking memory leack bomb here: WebSocket connections terminated by client and then reconnected will get new authkey with persisting old key.
    -- on reconnect client can use old key, it will work for everything but this logout method. NOTE: never do this in production.
    -- TODO: add notifications for client about connections termination for keys invalidation, or if you want authkeys to be persistent and act like
    -- a cookie then you need to invalidate old map value if you get message with already existing authkey and different handler
    local hkey=handler;
    local connection_authkey=nljson.find(maps.keys, tostring(hkey))
    if(connection_authkey ~= nil) and (connection_authkey == params[1].authkey) -- logged in and is an owner of the key
    then
      nljson.erase(maps.keys,hkey)
      ws:close(handler,1000);
    end
    else
      local can_not_logout=nljson.decode([[{
        "status" : 0,
        "error" : {
          "code" : 20,
          "message": "Can not logout. Not logged in."
        }
      }]]);
      ws:send(handler,can_not_logout); -- lets try to send a response to CCH here, with no regard if client closed its websocket or not
      ws:close(handler,1000);
    end
  end
}

lapps_echo_methods["_request_w_params_method"]={
  ["login"]=function(handler,params)
      local login_wrong_params=nljson.decode([[{
        "status" : 0,
        "error" : {
          "code" : -32602,
          "message": "method 'login' requires 2 parameters: 'user', 'password', both of them must have a string value"
        },
        "cid" : 0 
      }]]);


      local login_authentication_error=nljson.decode([[{
        "status" : 0,
        "error" : {
          "code" : 10,
          "message": "Authentication error"
        },
        "cid" : 0 
      }]]);


      local login_success=nljson.decode([[{
        "status" : 1,
        "result" : [
          { "authkey" : 0 }
        ],
        "cid" : 0 
      }]]);

      local wrong_params=nljson.decode([[{
        "status" : 0,
        "error" : {
          "code" : -32602,
          "message" : "there must be only one object inside the params array for method login(). This object must have two key/value pairs: { \"user\" : \"someusername\", \"password\" : \"somepassword\" }"
        }
      }]]);

    local username=""
    local password=""
      
    print(nljson.typename(params))
    if(nljson.typename(params[1]) == "object")
    then
      username=nljson.find(params[1],"user");
      password=nljson.find(params[1],"password");
    else
      ws:send(handler,login_wrong_params);
      ws:close(handler,1003); -- WebSocket close code here. 1002 - "protocol violation"
      return
    end
    if((type(username) ~= "string") or (type(password) ~= "string"))
    then
      ws:send(handler,login_wrong_params);
      ws:close(handler,1003); -- WebSocket close code here. 1002 - "protocol violation"
      return
    end

    local user_exists = nljson.find(maps.logins,username) ~= nil;
    if(user_exists)
    then
      if(maps.logins[username] == password)
      then -- generate authkey. it is a bad example of keys generation. do not use it in production. Do not use it ever. Im just lasy here.
        local authkey=math.random(4294967296);
        local idxkey=handler;
         maps.keys[tostring(idxkey)]=authkey; 
         login_success.result[1].authkey=authkey;
         ws:send(handler,login_success)
      else
        ws:send(handler,login_authentication_error);
        ws:close(handler,1000); -- WebSocket close code here. 1000 - "normal close"
      end
    else
        ws:send(handler,login_authentication_error);
        ws:close(handler,1000); -- WebSocket close code here. 1000 - "normal close"
    end
  end,
  ["echo"] = function(handler,params)
    local response=nljson.decode([[{
      "cid" : 0,
      "status" : 1,
      "result" : []
    }]]);

    response.result=params;
    ws:send(handler,response);
  end
}

return lapps_echo_methods