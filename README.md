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


# Prelimenary performance results for 0.7.0-avx2 build

## Test layout

* Test PC: 
  * Single CPU: Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz, cpu family: 158, stepping 9, microcode: 0x5e (all latest Spectre and meltdown patches are enabled) 4 cores
  * RAM: DIMM DDR4 Synchronous Unbuffered (Unregistered) 2400 MHz (0,4 ns), Kingston KHX2400C15/16G (4 modules)
* LAppS running with 1 worker and 1 service instance to match a single-threaded uWebSockets instance.
* All tests are done with TLS enabled
* Same PC was used to run clients and servers (I do not have infrastructure to run tests on separate systems)
* [Client code for echo server with RAW WebSockets protocol](https://github.com/ITpC/LAppS/blob/master/benchmark/echo_client_tls.cpp)
* [bash script for testruns with RAW WebSockets protocol](https://github.com/ITpC/LAppS/blob/master/benchmark/runBenchmark.sh) it calculates average rps from clients data. As longer it runs as more correct the average rps is calculated.
* [Client code for echo server with LAppS protocol](https://github.com/ITpC/LAppS/blob/master/benchmark/lapps_ebm.cpp)
* [bash script for testruns with LAppS protocol](https://github.com/ITpC/LAppS/blob/master/benchmark/runLAppSEBM.sh)
* [LAppS echo-service for RAW protocol](https://github.com/ITpC/LAppS/blob/master/examples/echo/echo.lua)
* [LAppS echo-service for LAppS protocol](https://github.com/ITpC/LAppS/tree/master/examples/echo_lapps) with running [broadcast service](https://github.com/ITpC/LAppS/blob/master/examples/time_broadcast/time_broadcast.lua)
* Test execution for RAW WebSockets echo server (within benchmark directory): ./runBenchmark.sh NUMBER_OF_CLIENTS PAYLOAD_SIZE
  * NOTE: LAppS or uWS_epoll must be started before you run the benchmark
* uWebSockets library echo server source code with TLS support [altered uWS.cpp from uWebSockets distribution](https://github.com/ITpC/LAppS/blob/master/benchmark/uWS/uWS.cpp)
* LAppS protocol echo server (very simplified) simulation with uWebSockets server [source code](https://github.com/ITpC/LAppS/blob/master/benchmark/uWS/uWSlapps.cpp) - requires [nlohmann json](https://github.com/nlohmann/json)

## RAW echo server test-case

| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.0| 240 | 84997 | 354.154 | 128|
|uWebSockets (latest)| 240 | 74172.7 | 309.053 |128|
|LAppS 0.7.0| 240 | 83627.4 | 348.447 | 512|
|uWebSockets (latest)| 240 | 71024.4 | 295.935 | 512|
|LAppS 0.7.0| 240 | 79270.1 | 330.292 | 1024|
|uWebSockets (latest)| 240 | 66499.8 | 277.083 | 1024|
|LAppS 0.7.0| 240 | 51621 | 215.087 | 8192|
|uWebSockets (latest)| 240 | 45341.6 |  188.924| 8192|


## LAppS protocol echo server results

| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.0| 240 | 36862.4 | 153.593 | 128|
|uWebSockets (latest)| 240 | 5830.18 | 24.2924 |128|

Here i have stopped the tests. uWebSockets with some additional work (CBOR encoding/decoding, doing less job then LAppS though) is an order of magnitude slower then LAppS.

Just to show LAppS scalability on the same CPU, I ran last test with 3 IOWorker threads and 2 instances of echo_lapps service

| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.0| 240 |  56449.9 | 235.208 | 128|

And with 3 IOWorker threads and 3 instances of echo_lapps service (CPU is already over high load because of 6 parallel CBOR encodings/decodings - 3 on server side 3 on client side. Actually it is only 4 cores CPU so effectively only 4 parallel tasks are running)


| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.0| 240 | 68809.4 | 286.706 | 128|


# Upstream (since 523a991cc692d804a6ba405466aba6b393fab4d4) performance

There is a WebSocket client implementation available in LAppS now. It is available as a **cws** module for preloading into your services.

Performance tests with this module shows much more significant performance improvment.

## Test layout

* Same PC as in above test.
* LAppS running in TLS mode with **echo** service enabled
* Benchmarking is implemented as an internal service [benchmark](https://github.com/ITpC/LAppS/blob/master/apps/benchmark/benchmark.lua), as you may see this service sends 1024 byte messages each time it receives a message from server side (the **echo** service).
  * Configuration for this service (paste within **services** section of the **lapps.conf**):
  ```json
  "benchmark" : {
      "auto_start" : true,
      "instances": 4,
      "internal": true,
      "preload" : [ "cws", "time" ]
    }
  ```
* echo-service configuration (again paste it within **services** section of **lapps.conf**):
  ```json
    "echo": {
      "auto_start": true,
      "instances": 2,
      "internal": false,
      "max_inbound_message_size": 16777216,
      "preload": null,
      "protocol": "raw",
      "request_target": "/echo",
      "extra_headers" : {
        "Service" : "echo",
        "Strict-Transport-Security" : "max-age=31536000; includeSubDomains"
      }
    }
    ```
## Collect the results
  The instances of the **benchmark** service are printing measurements once per second on stdout. Let the service run for a while (several minutes), then stop the LAppS.
  Copy lines those looks like: "63267 messages receved per 1000ms" into separate file. Let suppose you called it a **stats**
  There will be 4 such lines per second (each **benchmark** service prints its own results)
  Count the average rps (average messages received by all 4 **benchmark** services):
  ```bash
    awk -v avg=0 "{avg+=\$1}END{print (avg/$(wc -l stats))*4}" stats
  ```
## Results:

| Server | Clients | Server rps | rps per client| payload (bytes)|
|:---|:---:|:---:|:---:|---:|
|LAppS 0.7.0-523a991cc692d804a6ba405466aba6b393fab4d4-TLS| 400 | **257828** | 644.57 | 1024 |

This makes it more then 500k packets per second or more then 4.4Gbit/s network utilization. Further improvements on single CPU machines may be reached by using **dpkg**.

NOTE: Please beware, upstream is broken for tests you can use 0.7.0 up to 523a991cc692d804a6ba405466aba6b393fab4d4 commit.
