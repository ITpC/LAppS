local xavante = require "xavante"
local hfile = require "xavante.filehandler"
local hredir = require "xavante.redirecthandler"

http = {}
http.__index = http


http["init"]=function()
  webDir = "/tmp/test/";
end

http["mustStop"]=function()
  return must_stop()
end

http["run"]=function()
  local simplerules = {
    { -- URI remapping example
      match = "^[^%./]*/$",
      with = hredir,
      params = {"index.html"}
    },
    { 
      match = ".",
      with = hfile,
      params = {baseDir = webDir}
    }
  }

  xavante.HTTP{
    server = {host = "*", port = 80},

    defaultHost = {
    	rules = simplerules
    }
  }

  xavante.start(http.mustStop,1);
end


return http
