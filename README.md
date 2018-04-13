# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455).

It is in an alpha stage right now. As of today (13 April 2018) first lua application is running under LAppS. The WebSocket implementation is not yet 100% strict, though it is complient with RFC 6455 (see autobahn testsuite results in the link bellow). No WebSockets extensions are supported yet.


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
## Docker dev build
## Docker run build
## LAppS protocol implementation
## Lua applications documentation
## WebSocket extensions
## Decoupled apps (aka internal apps)
## PostgreSQL connection pool
## ... lot of other things
## Performance optimizations


[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)

