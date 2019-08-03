mqr_sender={}
mqr_sender.__index=mqr_sender

mqr_sender.init=function()
end

mqr_sender.queue=mqr.create("testpipe");

mqr_sender.run=function()
  local hw=nljson.decode([[
    {
        "message" : [ "Hello World", "PAYLOADXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ]
    }
  ]]);
  while not must_stop()
  do
    mqr_sender.queue:post(hw);
  end
end

return mqr_sender;
