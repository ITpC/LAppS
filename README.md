# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

RFC 6455 is fully implemented.  See the Conformance section.

RFC 7692 (compression extensions) not implemented due concerns about [BREACH attacks](https://en.wikipedia.org/wiki/BREACH). It is possible to have per-message compression on the [LAppS protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md) level without being affected by BREACH attacks.

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on examples of how to build your own application.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS from sources. 


There are package for ubuntu xenial available in this repository:

* [lapps-0.6.3-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.6.3-amd64.deb) (stable build with decoupled apps. New available options: auto-fragmentation of outbound messages, inbound message size limit (service specific), possibility to limit amount of connections per worker)

# Features

* Fastest WebSocket stream parser in industry (faster then uWebSockets)
* Easy API for rapid development of backend services in lua
* High vertical scalability for requests parallelization
* Up to 1 million client connections on one system
* Higly tunable through simple JSON configuration files
* Requests multiplexing on [application level protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md)
+ Copy-less high performance communications between services using MQR (an embedded shared queues) within one LAppS process


# Architecture

![LAppS-Architecture](https://github.com/ITPC/LAppS/raw/master/docs/LAppS-Architecture.png "LAppS pipline")

There are several core components of LAppS:
  * Listeners
  * Balancer
  * IOWorkers

All these components are running within one LAppS process. LAppS is the only WebSocket server that uses internal balancer for load equalization on IOWorkers.

## Listeners

Listeners are responsible for accepting new connections, you can awlays configure how many listeners may run in parallel (see the wiki).

## Balancer

Balancer weights the IOWorkers for their load and based on connection/IO-queue weighting assigns new connections to IOWorker with smallest weight (minimal load at the time).

## IOWorkers

IOWorkers are responsible for working on IO-queue, the only thing they do is an input and output of the data, to/from clients. Amount of IOWorkers running in parallel defines the IO-performance. Each IOWorker can support tens thousands of connections. 

## Services

Services are the Lua Applications. Each service may run parallel copies of itself (instances) to achieve maximum performance. The application have a choice of two protocols for clinet-server communications: RAW and LAppS.

RAW protocol behaviour is not specified and is not affected by LAppS (excluding control frames, those are never sent to the Lua Applications). It is for application to define how to handle inbound requests and how to react on them.

An optional [LAppS protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md) defines a framework similar to JSON-RPC or gRPC, with following key differences:
  * Transport is the WebSockets
  * Exchange between server and client is encoded with CBOR
  * There are  Out of Order Notifications (OON) available to serve as a server originated events. This makes it very easy to implement any kind of PUB/SUB services or the broadcasts.
  * LAppS defines channels as a means to distinguish type of the event streams for purposes of multiplexing data streams within WebSocket stream. For example channel 0 (CCH) is reserved for request-response stream. All other channels are defined by application and may or may not be synchron/asynchron, ordered or unordered (see the examples for OON primer with broadcast)

# Conformance (and regression test results)

[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)


# Further development

Please see the [Project Page](https://github.com/ITpC/LAppS/projects/1) to glimpse on what is going on with the project.

Please use branch 0.6.3, the upstream is not yet fully functioning.

Right now the experimental branch 0.7.0 with significant performance improvements is in development.

Prelimenary builds of 0.7.0 are available now. Build separation has allowd me to provide 2 different deb packages, [generic](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-ssse3-amd64.deb) one (CPU required to support SSSE3 instructions or above) and the [AVX2 build](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-avx2-amd64.deb) for CPUs with AVX2 or AVX512 support.

