echo = {}

echo.__index = echo;

echo["onStart"]=function()
  print("echo::onStart");
end

echo["onDisconnect"]=function()
end

echo["onShutdown"]=function()
  print("echo::onShutdown()");
end

echo["onMessage"]=function(handler,opcode, message)
  -- print("echo::onMessage");
  
  local result, errmsg=ws:send(handler,opcode,message);
  if(not result)
  then
    print("echo::OnMessage(): "..errmsg);
  end
  return result;
end

return echo;
