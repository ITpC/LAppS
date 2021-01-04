
<a href="https://scan.coverity.com/projects/itpc-lapps">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/14982/badge.svg"/>
</a>

# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling. LAppS has [most scallable WebSockets server](https://habr.com/ru/post/460847/) out there.

LAppS is the same thing to WebSockets as the Apache or Nginx are to HTTP. LAppS does not supports HTTP (though it supports HTTP Upgrade GET request as per RFC 6455). The only supported scripting language so far is the Lua.

RFC 6455 is fully implemented.  See the Conformance section.

RFC 7692 (compression extensions) is not implemented due to concerns about [BREACH attacks](https://en.wikipedia.org/wiki/BREACH). It is possible to have per-message compression on the [LAppS protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md) level without being affected by BREACH attacks.

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) and the examples on how to build your own applications.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS from sources. 

# Latest stable release

0.9.5 04.01.2021 (with wolfssl-4.6.0)
0.9.4 01.01.2021 (with wolfssl-4.5.0)

## Binaries

  * Ubuntu bionic or focal packages are available [here](https://github.com/ITpC/LAppS.builds).

# Features

* Fastest WebSocket stream parser in industry
* Easy API for rapid development of backend services in lua, C or C++
* High vertical scalability for requests parallelization
* Several million client connections on one system 
* Higly tunable through simple JSON configuration files
* Requests multiplexing on [application level protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md)
+ Copy-less high performance communications between services using MQR (an embedded shared queues) within one LAppS process
* Two level Network ACL: Server-wide and service-specific


# Services

Services are the Lua Applications. Each service may run parallel copies of itself (instances) to achieve maximum performance. The application have a choice of two protocols for clinet-server communications: RAW and LAppS.

RAW protocol behaviour is not specified and is not affected by LAppS (excluding control frames, those are never sent to the Lua Applications). It is for application to define how to handle inbound requests and how to react on them.

An optional [LAppS protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md) defines a framework similar to JSON-RPC, with following key differences:
  * Transport is the WebSockets
  * Exchange between server and client is encoded with CBOR
  * There are  Out of Order Notifications (OON) available to serve as a server originated events. This makes it very easy to implement any kind of PUB/SUB services or the broadcasts.
  * LAppS defines channels as a means to distinguish type of the event streams for purposes of multiplexing data streams within WebSocket stream. For example channel 0 (CCH) is reserved for request-response stream. All other channels are defined by application and may or may not be synchron/asynchron, ordered or unordered (see the examples for OON primer with broadcast)

# Conformance (and regression test results)

[Autobahn TestSuite Results. No extensions are implemented yet](http://htmlpreview.github.io/?https://github.com/ITpC/LAppS/blob/master/autobahn-testsuite-results/index.html)


# Further development

Roadmap will be ready soon. Next releases will include:
  * improved statistics on LAppS I/O
  * Support for services written in Python, PHP and JavaScript

# Dependencies
  * [ITCLib](git@github.com:ITpC/ITCLib.git) - LAppS suplimentary library (boost licensed)
  * [lar](git@github.com:ITpC/lar.git) - LAppS/Lazy archiver (boost licensed)
  * wolfSSL-4.6.0-stable (or later version)
  * luajit-2.0.5 or later version
  * libfmt-7.1.3 or later version for log-formatting
  * [mimalloc library 1.6](https://github.com/microsoft/mimalloc)


# Prerequisites
  * GCC or Clang with C++17 support

