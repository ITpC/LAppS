echo = {}

echo.__index = echo;

echo["onStart"]=function()
  print("echo::onStart");
end


echo["onShutdown"]=function()
  print("echo::onShutdown()");
end

echo["onMessage"]=function(message)
  -- print("echo::onMessage");
  return message;
end


return echo;
