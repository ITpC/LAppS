time_broadcast={}
time_broadcast.__index=time_broadcast

time_broadcast["init"]=function()
  bcast:create(1000);
  time_broadcast["nap"]=nap:new();
end

time_broadcast["run"]=function()
  print("running")
  local oon=nljson.decode([[{
      "cid" : 5,
      "message" : [ "" ]
  }]]);
  while not must_stop()
  do
    time_broadcast.nap:usleep(1000000);
    local timenow=os.time();
    local timestr=os.date("%c",timenow);
    oon.message[1]="Server time: "..timestr;
    bcast:send(1000,oon);
  end
end

return time_broadcast;
