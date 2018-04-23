# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

It is in an alpha stage right now. As of today (13 April 2018) first lua application is running under LAppS. The WebSocket implementation is not yet 100% strict, though it is complient with RFC 6455 (see autobahn testsuite results in the link bellow). No WebSockets extensions are supported yet.

## How to build and run the LAppS

### Prerequisites

1. Docker installed
2. Docker daemon is running

## Build steps

First we need a base docker image. This step is done only once and after you've got the base image lapps:luadev, you will not require to do this step anymore.
Download [Dockerfile](https://github.com/ITpC/LAppS/blob/master/dockerfiles/Dockerfile) and place it somewhere. Change the directory to that *`somewhere'*. And then run the command:


*docker build -t lapps:luadev .*


This will create base image for your experimentation/development. Check it with **docker image ls** and you will see your base image with name *lapps* and tag *luadev* listed between the images.


Download the [Dockerfile.lapps-build](https://github.com/ITpC/LAppS/blob/master/dockerfiles/Dockerfile.lapps-build) file now and place it again `somewhere'. Assuming it is the same directory as the last time run the command:

*docker build -t lapps:build -f Dockerfile.lapps-build  .*

Which will build the LAppS and create docker image with name lapps and tag build. 

One more step before we run it. Lets install LAppS somewhere:


*docker run -it --rm --name myldev -h ldev -v /opt/lapps-builds/latest:/opt/lapps lapps:build  make install-examples clone-luajit clone-libressl*


This command will install LAppS, related luajit-2.0.5 and libressl files as well an example app into /opt/lapps-build/latest/


## Running the LAppS

Lets make a separate clean container without anything we do not need. Download the [Dockerfile.lapps-runenv](https://github.com/ITpC/LAppS/blob/master/dockerfiles/Dockerfile.lapps-runenv) file and run following command:

*docker build -t lapps:runenv -f Dockerfile.lapps-runenv .*

*docker run -it --rm --name myrun -h lapps -p 5083:5083 -v /opt/lapps-builds/latest:/opt/lapps lapps:runenv env LD_LIBRARY_PATH=/opt/lapps/lib LUA_PATH="${LUA_PATH};/opt/lapps/apps/echo/?.lua" /opt/lapps/bin/lapps*

Now LAppS is up and running on port 5083, you may test it against autoban-testsuite fuzzing-client. Suppose your container has an IP address 172.17.0.1, the fuzzingclient.json config file for autoban-testsuite would look like:

```json
{
    "outdir": "./reports/servers",
    "servers": [
        {
          "agent": "LAppS TLS-enabled",
          "url": "wss://172.17.0.1:5083/echo"
        }
    ],
    "cases": ["*"],
    "exclude-cases": [],
    "exclude-agent-cases": {}
}
```

# Side-notes

Though LAppS is not even reached the optimization phase, it is running surprisingly good. 

Here are some performance results (dev instance with Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz). Servers all were tested in TLS only. Number of clients: 80. As a client was used benchmark/echo_client_tls.cpp (websocketpp example client)


<table style="width:100%">
<tr>
<th>Server</th>
<th>Average echo round-trips per second per client </th>
<th>Average echo rps served per second by server </th>
<th>Comments</th>
</tr>
<tr>
<td>LAppS(lua echo app)</td><td>953.395</td><td>78846.7</td><td>(raw protocol - full round-trip over luajit stack: message is copied to lua string, after response it is copied to send buffer)</td>
</tr>
<tr>
<td>uWebSockets(uWS_epoll)</td><td>995.026</td><td>84908.3</td><td>(same buffer is sent back. github version: bdea5fd1b1178eda1840d2d2c64f512457fc4217)</td>
</tr>
<tr>
<td>websocketpp(echo_server_tls)</td><td>737.49</td><td>38849.8</td><td>(same buffer is sent back. github version: 378437aecdcb1dfe62096ffd5d944bf1f640ccc3), websocketpp server failed to support 80 clinets, only 56 clients were running</td>
</tr>
</table>


# TODO
## ~~LAppS protocol implementation~~
## Lua applications documentation
## Dynamic deployer
## LAppS configuration guide
## Decoupled apps (aka internal apps)
## WebSocket extensions
## PostgreSQL connection pool
## ... lot of other things
## Performance optimizations


[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)

