# LAppS - Lua Application Server

This is an attempt to provide very easy to use Lua Application Server working over WebSockets protocol (RFC 6455). LAppS is an application server for micro-services architecture. It is build to be highly scalable vertically. The docker cloud infrastructure (kubernetes or swarm) shall be used for horizontal scaling.

RFC 6455 is fully implemented.  See the Conformance section.

RFC 7692 (compression extensions) not implemented due concerns about [BREACH attacks](https://en.wikipedia.org/wiki/BREACH). It is possible to have per-message compression on the [LAppS protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md) level without being affected by BREACH attacks.

LAppS is an easy way to develop low-latency web applications. Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on examples of how to build your own application.

Please see [LAppS wiki](https://github.com/ITpC/LAppS/wiki) on how to build and run LAppS from sources. 


There are package for ubuntu xenial available in this repository:

* [lapps-0.6.3-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.6.3-amd64.deb) (stable build without perf optimizations, inter-service communications, ACLs, etc)
* [lapps-0.7.0-ssse3-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-ssse3-amd64.deb) (prelimenary build for AMD/Intel CPUs with SSSE3 and above support)
* [lapps-0.7.0-avx2-amd64.deb](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-avx2-amd64.deb) (prelimenary build for AMD/Intel CPUs with AVX2 support)

# Features

* Fastest WebSocket stream parser in industry (faster then uWebSockets) (see the test results bellow)
* Easy API for rapid development of backend services in lua
* High vertical scalability for requests parallelization
* Up to 1 million client connections on one system
* Higly tunable through simple JSON configuration files
* Requests multiplexing on [application level protocol](https://github.com/ITpC/LAppS/blob/master/LAppS_Protocol_Specification.md)
+ Copy-less high performance communications between services using MQR (an embedded shared queues) within one LAppS process
* Two level Network ACL: Server-wide and service-specific


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

Prelimenary builds of 0.7.0 are available now. Build separation has allowd me to provide 2 different deb packages, [generic](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-ssse3-amd64.deb) one (CPU required to support SSSE3 instructions or above) and the [AVX2 build](https://github.com/ITpC/LAppS/raw/master/packages/lapps-0.7.0-avx2-amd64.deb) for CPUs with AVX2 or AVX512 support.


# Test results for 0.7.1 branch

## Test layout

* Test PC: 
  * Single CPU: Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz, cpu family: 158, stepping 9, microcode: 0x5e (all latest Spectre and meltdown patches are enabled) 4 cores
  * RAM: DIMM DDR4 Synchronous Unbuffered (Unregistered) 2400 MHz (0,4 ns), Kingston KHX2400C15/16G (4 modules)
* NOTE: for these tests TLS support in LAppS is disabled. The iptables is disabled on host machine as well.
* LAppS was ran with 3 workers and 2 echo service instances
* uWebSockets was ran with 4 threads.
* LAppS Server side:
  * [LAppS echo-service for RAW protocol](https://github.com/ITpC/LAppS/blob/master/examples/echo/echo.lua) 
* uWebSockets Server side:
  * [multithreaded_echo.cpp](https://github.com/ITpC/LAppS/blob/master/benchmark/uWS/multithreaded_echo.cpp)
* Client Side:
  * [LappS benchmark service](https://github.com/ITpC/LAppS/blob/master/apps/benchmark/benchmark.lua)
  * 4 Client instances were running in both cases.
* Payload in both cases: 64 bytes

How the tests were running:

  In both cases, client and server were running on the same phisical host and the same CPU. In benchmark.lua parameters for amount of clients as well as the URL pointing to the servers endpoint are hardcoded. Amount of clients is set in a for loop on line 26 (for i=0,99). This gives 400 client connections (remember 4 client instnaces of benchmark.lua were running)
  The URL have to be changed as well. LAppS URL: ws://localhost:5083/echo and uWebSockets URL ws://localhost:5080

  Saving results into file:

```bash
 export RESULT_FILE=out.lapps.nostats.notls.noiptables.3w.2e 
```

  Performing the test (from within build directory)

```bash
./dist/Release.AVX2.NO_STATS.NO_TLS/GNU-Linux/lapps.avx2.nostats.notls >  ${RESULT_FILE} ; egrep "^[1-9]" ${RESULT_FILE} | sed -e 's/ms[1-9]/ms\n$/g' | sed -e '/^$/d' > ${RESULT_FILE}.flt; tail -n $(( $(wc -l ${RESULT_FILE}.flt | awk '{print $1}') - 8 )) ${RESULT_FILE}.flt > ${RESULT_FILE}; awk -v avg=0 "{avg+=\$1}END{print \"Server average RPS: \" (avg/$(wc -l ${RESULT_FILE}|awk '{print $1}'))*4}" ${RESULT_FILE}
``` 

  After several minutes of runnnung press Ctrl-C to stop the test and see the results.

## Results:

| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.1 - 3 workers, 2 echo services| 400 | **693927** | 1734.81  | 64 |
|uWebSockets-latest  - 4 threads | 400 | **750351** | 1875.87 | 64 |

There are several notes to think on:

  1. WebSocket Clients are agressively using RNG so there are some blockings on /dev/urandom are involved
  2. According to perf Lua footprint in LAppS takes about 10% of performance 
    * First and last strings in following snippet:
```text
     6.11%  lapps.avx2.nost  libluajit-5.1.so.2.0.5    [.] lj_str_new
     5.51%  lapps.avx2.nost  lapps.avx2.nostats.notls  [.] LAppS::IOWorker<false, false>::execute
     4.72%  lapps.avx2.nost  lapps.avx2.nostats.notls  [.] cws_eventloop
     4.09%  lapps.avx2.nost  lapps.avx2.nostats.notls  [.] LAppS::LuaReactiveService<(LAppS::ServiceProtocol)0>::execute
     3.91%  lapps.avx2.nost  libluajit-5.1.so.2.0.5    [.] lj_BC_TGETS
```
  3. In the same time in the above test results LAppS is slower only on about 7.5%
  4. The total number of packets on the host machine in test-runtime in both cases is a factor of two higher then the RPS of the server (benchmark shows only amount of packets, received by the clients)


This makes LAppS WenSockets stack most performant in the industry.
