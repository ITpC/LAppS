# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

RFC 6455 is fully implemented. RFC 7692 (compression extensions) not yet implemented.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS from sources. 

There is package for ubuntu xenial available in this repository: [lapps-0.5.3-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.5.3-amd64.deb)

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on examples of how to build your own application.

# Architecture

![LAppS-Architecture](https://github.com/ITPC/LAppS/raw/master/docs/LAppS-Architecture.png "LAppS pipline")

There are several core components in LAppS:
  * Listeners
  * Balancer
  * IOWorkers

## Listeners

Listeners are responsible for accepting new connections, you can awlays configure how many listeners may run in parallel (see the wiki).

## Balancer

Balancer weights the IOWorkers for their load and based on connection/IO-queue weighting assigns new connections to IOWorker with smallest weight (minimal load at the time).

## IOWorkers

IOWorkers are responsible for working on IO-queue, the only thing they do is an input and output of the data, to/from clients. Amount of IOWorkers running in parallel defines the IO-performance. Each IOWorker can support tens thousands of connections. 

## Services

Services are the Lua Applications. Each service may run parallel copies of itself (instances) to achieve maximum performance. The application have a choice of two protocols for clinet-server communications: RAW and LAppS.

RAW protocol behaviour does not specified andi not affected by LAppS (excluding service frames, those are never sent to the Lua Applications). It is for application to define how to handle inbound requests and how to react on them.

LAppS protocol defines a framework similar to JSON-RPC, with following key differences:
  * Exchange between server and client is encoded with CBOR
  * There are  Out of Order Notifications (OON) available to serve as a server originated events. This makes it very easy to implement any kind of subscribe-observe mechanisms or the broadcasts.
  * LAppS defines channels as a means to distinguish type of the event streams. For example channel 0 (CCH) is reserved for request-response stream. All other channels are defined by application and may or may not be synchron/asynchron, ordered or unordered (see the examples for OON primer with broadcast)


# Conformance

[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)


# Side-notes

Though LAppS is not even reached the optimization phase, it performs surprisingly good. The server architecture is build to sustain up to one million connections (assuming sufficient hardware is provided). Take a note, that performance may be nearly doubled after development stack of the EventBus will be removed.

Here are some performance results (dev instance with Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz). Servers all were tested in TLS only. Number of clients: 80. As a client was used benchmark/echo_client_tls.cpp (websocketpp example client)

**NOTE**: Please do not consider these results seriously. Because
  * This comparison is made for raw echo servers with 277 bytes uniform text messages. 
  * This comparison is made for products with real world application vs development release of LAppS, which does not have reached production maturity
  * Testing vs nginx (a web-server), is not really a correct thing. Nginx was not build for WebSockets and running Lua-applications detached from workers (which is the case for LAppS).

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
<td>Nginx-1.12.2-r1 + lua-resty-websocket</td><td>720.6</td><td>58367.4</td><td>worker_processes 3; location section of https://github.com/ITpC/LAppS/blob/master/examples/nginx.conf</td>
</tr>
<tr>
<td>LAppS(lua echo app)</td><td>692.9</td><td>55432.7</td><td>(raw protocol - full round-trip over luajit stack: message is copied to lua string, after response it is copied to send buffer), without load balancer</td>
</tr>
<tr>
<td>websocketpp(echo_server_tls)</td><td>737.49</td><td>38849.8</td><td>(same buffer is sent back. github version: 378437aecdcb1dfe62096ffd5d944bf1f640ccc3), websocketpp server failed to support 80 clinets, only 56 clients were running</td>
</tr>
</table>



Test results for 120 clients (same clients code)

<table style="width:100%">
<tr>
<th>Server</th>
<th>Average echo round-trips per second per client </th>
<th>Average echo rps served per second by server </th>
<th>Comments</th>
</tr>
<tr>
<td>LAppS-0.5.3</td><td>775.733</td><td>92086</td><td>2 listeners, 4 workers, 4 instances of the echo service(raw protocol)</td>
</tr>
<tr>
<td>uWebSockets</td><td>693.361</td><td>83177.5</td><td>not tunable</td>
</tr>
<tr>
<td>nginx-1.12.2-r1/luajit(gentoo)+lua-resty-websocket</td><td>560.257</td><td>66962.1</td><td>4 workers</td>
</tr>
<tr>
<td>nginx-1.12.2-r1/luajit(gentoo)+lua-resty-websocket</td><td>468.393</td><td>56171.8</td><td>3 workers</td>
</tr>
</table>

