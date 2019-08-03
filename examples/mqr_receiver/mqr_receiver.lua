mqr_receiver={}
mqr_receiver.__index=mqr_receiver

mqr_receiver.init=function()
end

mqr_receiver.msg_counter=0;
mqr_receiver.start_time=time.now();

mqr_receiver.meter=function()
  mqr_receiver.msg_counter=mqr_receiver.msg_counter+1;

  local slice = time.now() - mqr_receiver.start_time;
  if(slice >= 1000)
  then
    print("Received "..mqr_receiver.msg_counter.." messages per "..slice.."ms");
    mqr_receiver.msg_counter=0;
    mqr_receiver.start_time=time.now();
  end
end

mqr_receiver.queue=mqr.create("testpipe");

mqr_receiver.run=function()
  while not must_stop()
  do
    local nljo=mqr_receiver.queue:recv();
    mqr_receiver.meter();
  end
  print("must stop["..instance_id.."]")
end

return mqr_receiver;
