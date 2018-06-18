# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

RFC 6455 is fully implemented. RFC 7692 (compression extensions) not yet implemented.

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on examples of how to build your own application.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS from sources. 


There are package for ubuntu xenial available in this repository:

* [lapps-0.6.3-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.6.3-amd64.deb) (stable build with decoupled apps. New available options: auto-fragmentation of outbound messages, inbound message size limit (service specific), possibility to limit amount of connections per worker)


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

RAW protocol behaviour does not specified and not affected by LAppS (excluding service frames, those are never sent to the Lua Applications). It is for application to define how to handle inbound requests and how to react on them.

LAppS protocol defines a framework similar to JSON-RPC or gRPC, with following key differences:
  * Transport is the WebSockets
  * Exchange between server and client is encoded with CBOR
  * There are  Out of Order Notifications (OON) available to serve as a server originated events. This makes it very easy to implement any kind of subscribe-observe mechanisms or the broadcasts.
  * LAppS defines channels as a means to distinguish type of the event streams. For example channel 0 (CCH) is reserved for request-response stream. All other channels are defined by application and may or may not be synchron/asynchron, ordered or unordered (see the examples for OON primer with broadcast)


# Conformance (and regression test results)

[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)


# Further development

Please see the [Project Page](https://github.com/ITpC/LAppS/projects/1) to glimpse on what is going on with the project.

Right now the experimental branch 0.7.0 with significant performance improvements is in development.

