# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

RFC 6455 is fully implemented. RFC 7692 (compression extensions) not yet implemented.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS. 

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on examples of how to build your own application.

# Side-notes

Though LAppS is not even reached the optimization phase, it performs surprisingly good. The server architecture is build to sustain up to one million connections (assuming sufficient hardware is provided). Take a note, that performance may be nearly doubled after development stack of the EventBus will be removed.

Here are some performance results (dev instance with Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz). Servers all were tested in TLS only. Number of clients: 80. As a client was used benchmark/echo_client_tls.cpp (websocketpp example client)


<table style="width:100%">
<tr>
<th>Server</th>
<th>Average echo round-trips per second per client </th>
<th>Average echo rps served per second by server </th>
<th>Comments</th>
</tr>
<tr>
<td>uWebSockets(uWS_epoll)</td><td>995.026</td><td>84908.3</td><td>(same buffer is sent back. github version: bdea5fd1b1178eda1840d2d2c64f512457fc4217)</td>
</tr>
<tr>
<td>LAppS(lua echo app)</td><td>1077.93</td><td>86627.7</td><td>(raw protocol - full round-trip over luajit stack: message is copied to lua string, after response it is copied to send buffer), with internal load balancer</td>
</tr>
<tr>
<td>LAppS(lua echo app)</td><td>953.395</td><td>55432.7</td><td>(raw protocol - full round-trip over luajit stack: message is copied to lua string, after response it is copied to send buffer), without load balancer</td>
</tr>
<tr>
<td>websocketpp(echo_server_tls)</td><td>737.49</td><td>38849.8</td><td>(same buffer is sent back. github version: 378437aecdcb1dfe62096ffd5d944bf1f640ccc3), websocketpp server failed to support 80 clinets, only 56 clients were running</td>
</tr>
</table>


# TODO
## ~~LAppS configuration guide~~
## Lua applications documentation
## Dynamic deployer
## Decoupled apps (aka internal apps)
## WebSocket extensions
## PostgreSQL connection pool (please use existing lua modules like luasql, pgmoon, etc; until this module is created)
## ... lot of other things
## Performance optimizations


[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)

