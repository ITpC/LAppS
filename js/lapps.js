
(function(global,undefined) { "use strict";

  function Properties()
  {
  }

  function Construct(uri,onopen,onerror,onmessage)
  {
    Properties.prototype.onopen=onopen;
    Properties.prototype.onerror=onerror;
    Properties.prototype.onmessage=onmessage;
    Properties.prototype.uri=uri;

    var properties=new Properties();

    this.websocket = new WebSocket(properties.uri);
    this.websocket.binaryType = "arraybuffer";
    this.websocket.onopen = function()
    {
      properties.onopen();
    }
    this.websocket.onmessage=function(event)
    {
      var message = CBOR.decode(event.data);
      switch(message.status)
      {
        case 0:
          properties.onerror(message.error.code,message.error.message,message.error.data);
        break;
        case 1:
          switch(message.cid)
          { 
            case 3: // OON error notification
              properties.onerror(message.result[0],message.result[1],null);
            break;
            default:
              properties.onmessage(cid, result);
            break;
          }
        break;
      }
    }
  }


  if (!global.lapps)
    global.lapps = Construct;
})(this);
