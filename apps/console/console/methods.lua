methods = {}
methods.__index = methods;

local app_dir_prefix=os.getenv("LAPPS_APPS_DIR").."/console";

local pam_config=nljson.decode_file(app_dir_prefix.."/pam.json");

methods.maps={}
methods.messages={}
methods.private={}
methods._cn_w_params_method={}
methods._cn_wo_params_method={}
methods._request_wo_params_method={}
methods._request_w_params_method={}

methods.maps.active_users=nljson.decode([[{
}]]);

methods.messages.login_auth_error=nljson.decode([[{
  "status" : 0,
  "error" : {
    "code" : 10,
    "message": "Authentication error"
  },
  "cid" : 0
}]]);

methods.messages.login_success=nljson.decode([[{
  "status" : 1,
  "result" : [
    { "authkey" : 0 }
  ],
  "cid" : 0
}]]);

methods.messages.invalid_params=nljson.decode([[{
  "status" : 0,
  "error" : {
  "code" : -32602,
  "message" : "There must be only one object inside the params array for method login(). This object must have two key/value pairs: { \"user\" : \"someusername\", \"password\" : \"somepassword\" }"
    },
    "cid" : 0
}]]);

methods.private.is_active=
function(authkey)
  if(methods.maps.active_users.find(authkey) ~= nil)
  then
    return true;
  else
    return false;
  end
end

methods.private.push_view=
function(handler)
  print("pushview");
end

methods._cn_w_params_method.login =
function(handler, params)

  local username="";
  local password="";

  if(nljson.typename(params[1]) == "object")
  then
    username=nljson.find(params[1],"user");
    password=nljson.find(params[1],"password");
  else
    ws:send(handler,methods.messages.invalid_params);
    ws:close(handler,1003); 
    return
  end

  local username_allowed = false;

  nljson.foreach(
    pam_config.uses,
    function(value)
      if(type(value) == "string")
      then
        if(value == username)
        then
          username_allowed=true;
        end
      end
    end
  );

  if(pam_auth:login(pam_config.service,username,password))
  then
    local message=methods.messages.login_success;
    message.result[1].authkey=murmur.hashstr(handler);
    methods.maps.active_users[tostring(handler)]=message.result[1].authkey;
    ws:send(handler,message);
    methods.private.push_view(handler);
  else
    ws:send(handler,methods.messages.login_auth_error);
    ws:close(1000);
  end
end

return methods;
