One thing about WebSockets is that you need a lot of resources on the client's side to generate high enough load for the server to actually eat up all the CPU resources.

There are several challenges you have to overcome because the WebSockets protocol is more CPU demanding on the client's side than on the server's side. At the same time you need a lot of RAM to store information about open connections if you have millions of them.

I've been lucky enough to get a couple of new servers for a limited period of time at my disposal for the hardware "burnout" tests. So I decided to use my Lua Application Server - [LAppS](https://github.com/ITpC/LAppS) to do both jobs: test the hardware and perform the LAppS high load tests.

<cut />Let's see the details

#Challenges

First of all WebSockets on the client's side require a pretty fast RNG for traffic masking unless you want to fake it. I wanted the tests to be realistic, so I discarded the idea of faking the RNG by replacing it with a constant. This leaves the only option - a lot of CPU power. As I said, I have two servers: one with dual Intel Gold 6148 CPUs (40 cores in total) 256GB RAM (DDR4-2666) and one with quad Intel Platinum 8180 CPUs (112 cores in total) 384GB RAM (DDR4-2666). I'll be referring to them as Server A and Server B hereafter.

The second challenge - there are no libraries or test suites for WebSockets that are fast enough. Which is why I was forced to develop a WebSockets client module for LAppS (**cws**). 

#The tests setup

Both servers have dual port 10GbE cards. Port 1 of each card is aggregated to a bonding interface and these ports on either cards are connected to each other in RR mode. Port 2 of each card is aggregated to a bonding interface in an Active-Backup mode, connected over an Ethernet switch. Each hardware server was running the RHEL 7.6. I had to install gcc-8.3 from the sources to have a modern compiler instead of the default gcc-4.8.5. Not the best setup for the performance tests, yet beggars can't be choosers.

CPUs on Server A (2xGold 6148) have higher frequency than on Server B (4xPlatinum 8180). At the same time Server B has more cores, so I decided to run an echo server on Server A and the echo clients on Server B.

I wanted to see how LAppS behaves under high load with millions of connections and what maximum amount of echo requests per second it can serve. These are actually two different sets of tests because with the connections amount growth the server and clients are doing an increasingly complex job.

Before this I have made all the tests on my home PC which I use for development. The results of these tests will be used for comparison. My home PC has an Intel Core i7-7700 CPU 3.6GHz (4.0GHz Turbo) with 4 cores and 32GB of DDR4-2400 RAM. This PC runs Gentoo with 4.14.72 kernel.

"Specter and Meltdown patch levels"

  * Home PC
```text
/sys/devices/system/cpu/vulnerabilities/l1tf:Mitigation: PTE Inversion; VMX: conditional cache flushes, SMT vulnerable
/sys/devices/system/cpu/vulnerabilities/meltdown:Mitigation: PTI
/sys/devices/system/cpu/vulnerabilities/spec_store_bypass:Mitigation: Speculative Store Bypass disabled via prctl and seccomp
/sys/devices/system/cpu/vulnerabilities/spectre_v1:Mitigation: __user pointer sanitization
/sys/devices/system/cpu/vulnerabilities/spectre_v2:Vulnerable: Minimal generic ASM retpoline, IBPB, IBRS_FW
```
  * Servers A and B
```text
/sys/kernel/debug/x86/ibpb_enabled : 1
/sys/kernel/debug/x86/pti_enabled : 1
/sys/kernel/debug/x86/ssbd_enabled : 1
/sys/kernel/debug/x86/ibrs_enabled : 1
/sys/kernel/debug/x86/retp_enabled : 3
```

I'll make an extra note with the results of the tests of Servers A and B whether these patches are enabled or not.

On my Home PC patch level is never changed.


## Installing LAppS 0.8.1

LAppS depends on the luajit-2.0.5 or higher, the libcrypto++8.2 and the wolfSSL-3.15.7 libraries and they have to be installed from sources in RHEL 7.6 and likely in any other linux distribution.
 
The prefix for installation is /usr/local. Here is the pretty much self-explanatory Dockerfile part for wolfSSL installation
```Dockerfile
ADD https://github.com/wolfSSL/wolfssl/archive/v3.15.7-stable.tar.gz ${WORKSPACE}

RUN tar xzvf v3.15.7-stable.tar.gz

WORKDIR ${WORKSPACE}/wolfssl-3.15.7-stable

RUN ./autogen.sh

RUN ./configure CFLAGS="-pipe -O2 -march=native -mtune=native -fomit-frame-pointer -fstack-check -fstack-protector-strong -mfpmath=sse -msse2avx -mavx2 -ftree-vectorize -funroll-loops -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DWC_RSA_BLINDING" --prefix=/usr/local --enable-tls13 --enable-openssh --enable-aesni --enable-intelasm --enable-keygen --enable-certgen --enable-certreq --enable-curve25519 --enable-ed25519 --enable-intelasm --enable-harden

RUN make -j40 all

RUN make install

```

And here is the part for the libcrypto++ installation
```Dockerfile
ADD https://github.com/weidai11/cryptopp/archive/CRYPTOPP_8_2_0.tar.gz ${WORKSPACE}

RUN rm -rf ${WORKSPACE}/cryptopp-CRYPTOPP_8_2_0

RUN tar xzvf ${WORKSPACE}/CRYPTOPP_8_2_0.tar.gz

WORKDIR ${WORKSPACE}/cryptopp-CRYPTOPP_8_2_0

RUN make CFLAGS="-pipe -O2 -march=native -mtune=native -fPIC -fomit-frame-pointer -fstack-check -fstack-protector-strong -mfpmath=sse -msse2avx -mavx2 -ftree-vectorize -funroll-loops" CXXFLAGS="-pipe -O2 -march=native -mtune=native -fPIC -fomit-frame-pointer -fstack-check -fstack-protector-strong -mfpmath=sse -msse2avx -mavx2 -ftree-vectorize -funroll-loops" -j40 libcryptopp.a libcryptopp.so

RUN make install

```

And the luajit
```Dockerfile
ADD http://luajit.org/download/LuaJIT-2.0.5.tar.gz ${WORKSPACE}

WORKDIR ${WORKSPACE}

RUN tar xzvf LuaJIT-2.0.5.tar.gz

WORKDIR ${WORKSPACE}/LuaJIT-2.0.5

RUN env CFLAGS="-pipe -Wall -pthread -O2 -fPIC -march=native -mtune=native -mfpmath=sse -msse2avx -mavx2 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fno-omit-frame-pointer" make -j40 all

RUN make install
```

One optional dependency of the LAppS, which may be ignored, is the new library from Microsoft [mimalloc](https://github.com/microsoft/mimalloc). This library  makes significant performance improvement (about 1%) but requires cmake-3.4 or higher. Given the limited amount of time for the tests, I decided to sacrifice mentioned performance improvement. 

On my home PC, I'll not disable *mimalloc* during the tests.

Lets checkout LAppS from the repository:
```Dockerfile
WORKDIR ${WORKSPACE}

RUN rm -rf ITCLib ITCFramework lar utils LAppS

RUN git clone https://github.com/ITpC/ITCLib.git

RUN git clone https://github.com/ITpC/utils.git

RUN git clone https://github.com/ITpC/ITCFramework.git

RUN git clone https://github.com/ITpC/LAppS.git

RUN git clone https://github.com/ITpC/lar.git

WORKDIR ${WORKSPACE}/LAppS
```

Now we need to remove "-lmimalloc" from all the Makefiles in **nbproject** subdirectory, so we can build LAppS (assuming our current directory is ${WORKSPACE}/LAppS)
```bash
# find ./nbproject -type f -name "*.mk" -exec sed -i.bak -e 's/-lmimalloc//g' {} \;
# find ./nbproject -type f -name "*.bak" -exec rm {} \;
```

And now we can build LAppS. LAppS provides several build configurations, which may or may not exclude some features on the server side:
  * with SSL support and with statistics gathering support
  * with SSL and without statistics gathering (though minimal statistics gathering will continue as it is used for dynamic LAppS tuning in runtime)
  * without SSL and without statistic gathering.

Before next step make sure you are the owner of the /opt/lapps directory (or run make installs with sudo)

Let's make two kinds of binaries with SSL support and statistics gathering and without (assuming we are within ${WORKSPACE}/LAppS directory):
```bash
# make clean
# make CONF=Release.AVX2 install
# make CONF=Release.AVX2.NO_STATS.NO_TLS install
```

The resulting binaries are:
  * dist/Release.AVX2/GNU-Linux/lapps.avx2
  * dist/Release.AVX2.NO_STATS.NO_TLS/GNU-Linux/lapps.avx2.nostats.notls

They will be installed into /opt/lapps/bin.

Please note that the WebSockets client module for Lua is always built with SSL support. Whether it is enabled or not depends on the URI you are using for connection (ws:// or wss://) in runtime. 

## Test 1. Top performance on home PC. Configuration for baseline.

I've already established that I get best performance when I configure four benchmark service instances with 100 connections each. In the same time I need only three IOWorkers and four echo service instances to achieve best performance. Remember? I have only 4 cores here.

The purpose of this test is just to establish a baseline for further comparison. Nothing exciting here really.

Bellow are the steps required to configure LAppS for the tests.

### Self signed certificates

certgen.sh script

```bash
#!/bin/bash
  
openssl genrsa -out example.org.key 2048
openssl req -new -key example.org.key -out example.org.csr

openssl genrsa -out ca.key 2048
openssl req -new -x509 -key ca.key -out ca.crt
openssl x509 -req -in example.org.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out example.org.crt
cat example.org.crt ca.crt > example.org.bundle.crt
```

Running this script from within the /opt/lapps/conf/ssl directory will create all required files. Here is the script's output and what I've typed in:

### LAppS configuration

Here is the WebSockets configuration file which is set for TLS 1.3, includes the above generated certificates and configured with 3 IOWorkers (see detailed description of variables in the LAppS wiki).

/opt/lapps/etc/conf/ws.json
```json
{
   "listeners" : 2,
  "connection_weight": 1.0,
  "ip" : "0.0.0.0",
  "port" : 5083,
  "lapps_config_auto_save" : true ,
  "workers" : {
    "workers": 3,
    "max_connections" : 40000,
    "auto_fragment" : false,
    "max_poll_events" : 256,
    "max_poll_wait_ms" : 10,
    "max_inbounds_skip" : 50,
    "input_buffer_size" : 2048
   },
  "acl" : {
    "policy" : "allow",
    "exclude" : []
  },
  "tls":true,
  "tls_server_version" : 4,
  "tls_client_version" : 4,
  "tls_certificates":{
    "ca":"/opt/lapps/conf/ssl/ca.crt",
    "cert": "/opt/lapps/conf/ssl/example.org.bundle.crt",
    "key": "/opt/lapps/conf/ssl/example.org.key"
  }
}
```

Configuring services: the echo server and the echo client (benchmark), each with four instances.

/opt/lapps/etc/conf/lapps.json"
```json
{
  "directories": {
    "app_conf_dir": "etc",
    "applications": "apps",
    "deploy": "deploy",
    "tmp": "tmp",
    "workdir": "workdir"
  },
  "services": {
    "echo": {
      "auto_start": true,
      "instances": 4,
      "internal": false,
      "max_inbound_message_size": 16777216,
      "protocol": "raw",
      "request_target": "/echo",
      "acl" : {
        "policy" : "allow",
        "exclude" : []
      }
    },
    "benchmark": {
      "auto_start": true,
      "instances" : 4,
      "internal": true,
      "preload": [ "nap", "cws", "time" ],
      "depends" : [ "echo" ]
    }
  }
}
```

The echo service is pretty trivial: 
```Lua
echo = {}

echo.__index = echo;

echo.onStart=function()
  print("echo::onStart");
end

echo.onDisconnect=function()
end

echo.onShutdown=function()
  print("echo::onShutdown()");
end

echo.onMessage=function(handler,opcode, message)

  local result, errmsg=ws:send(handler,opcode,message);
  if(not result)
  then
    print("echo::OnMessage(): "..errmsg);
  end
  return result;
end

return echo
```

The benchmark service creates as many as **benchmark.max_connections** to **benchmark.target** and then just runs until you stop the LAppS. There is no pause in the connections' establishment or echo requests bombardment. The **cws** module API resembles the Web API for WebSockets. Once all **benchmark.max_connections** are established the benchmark prints the amount of Sockets connected. Once the connection is established the benchmark sends a  **benchmark.message** to the server. After the server replies the anonymous **onmessage** method of the **cws** object is invoked, which just sends the same message back to the server. 

```Lua
benchmark={}
benchmark.__index=benchmark

benchmark.init=function()
end

benchmark.messages_counter=0;
benchmark.start_time=time.now();
benchmark.max_connections=100;
benchmark.target="wss://127.0.0.1:5083/echo";
benchmark.message="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

benchmark.meter=function()
  benchmark.messages_counter=benchmark.messages_counter+1;

  local slice=time.now() - benchmark.start_time;
  if( slice  >= 1000)
  then
    print(benchmark.messages_counter.." messages received per "..slice.."ms")
    benchmark.messages_counter=0;
    benchmark.start_time=time.now();
  end
end

benchmark.run=function()
  local n=nap:new();
  local counter=1;
  n:sleep(1);
  local array={};
  local start=time.now();
  while(#array < benchmark.max_connections) and (not must_stop())
  do
    local sock, err_msg=cws:new(benchmark.target,
    {
      onopen=function(handler)
        local result, errstr=cws:send(handler,benchmark.message,2);
        if(not result)
        then
          print("Error on websocket send at handler "..handler..": "..errstr);
        end
      end,
      onmessage=function(handler,message,opcode)
        benchmark.meter();
        cws:send(handler,message,opcode);
      end,
      onerror=function(handler, message)
        print("Client WebSocket connection is failed for socketfd "..handler..". Error: "..message);
      end,
      onclose=function(handler)
        print("Connection is closed for socketfd "..handler);
      end
    });

    if(sock ~= nil)
    then
      table.insert(array,sock);
    else
      print(err_msg);
      err_msg=nil;
      collectgarbage("collect");
    end

    -- poll events once per 10 outgoing connections 
    -- this will improve the connection establishment speed
    if counter == 10
    then
      cws:eventLoop();
      counter=1
    else
      counter = counter + 1;
    end
  end

  print("Sockets connected: "..#array);
  benchmark.start_time=time.now();

  while not must_stop()
  do
    cws:eventLoop();
  end

  for i=1,#array
  do
    array[i]:close();
    cws:eventLoop();
  end
end

return benchmark;
```

### Services installation

Now we need to place the service scripts into /opt/lapps/apps/<servicename>/<servicename>.lua:
  * /opt/lapps/apps/benchmark/benchmark.lua
  * /opt/lapps/apps/echo/echo.lua

We are ready to run our benchmark now. Just run: ```rm -f lapps.log; /opt/lapps/bin/lapps.avx2 > log``` from within the LAppS directory and wait for 5 mins, then press Ctrl-C once, to stop the LAppS (it will not stop immediately, it will shutdown the connections first), or twice (this will interrupt the shutdown sequence).

Ok, we've got a text file with something like this inside:
```text
echo::onStart
echo::onStart
echo::onStart
echo::onStart
running
1 messages received per 3196ms
1 messages received per 3299ms
1 messages received per 3299ms
1 messages received per 3305ms
Sockets connected: 100
Sockets connected: 100
Sockets connected: 100
Sockets connected: 100
134597 messages received per 1000ms
139774 messages received per 1000ms
138521 messages received per 1000ms
139404 messages received per 1000ms
140162 messages received per 1000ms
139337 messages received per 1000ms
140088 messages received per 1000ms
139946 messages received per 1000ms
141204 messages received per 1000ms
137988 messages received per 1000ms
141805 messages received per 1000ms
134733 messages received per 1000ms
...
```

Let's clean this log file like this:
```bash
# echo -e ':%s/ms/^V^M/g\n:%g/^$/d\nGdd1G4/Sockets\n4\ndggZZ' | vi $i
```
'^V^M' is visible representation of the following key hits: Ctrl-V Ctrl-V Ctrl-V Ctrl-M, so it will be pretty useless to just copy paste this bash line. Short explanation: 
  * we have to replace 'ms' symbols with end of line, because we do not need them, they will mess the calculations later on and 4 benchmarks working in parallel may print out their results in one line. 
  * we need to remove all empty lines afterwards
  * we remove the last line as well, because we stop the server 
  * in the **log** file there will be only fore lines consisting *Sockets connected: 100* (it is because we run only four benchmark services). So we skip 4 lines past the last of them, and than removing everything to the top of the file.
  * saving the file.

File is saved and you are back in shell now, and the log file is ready for the next step:

```bash
# awk -v avg=0 '{rps=($1/$5);avg+=rps;}END{print ((avg*1000)/NR)*4}' log
```
This awk single-liner calculates amount of the echo responses per ms, and accumulates the result in *avg* variable. After all the lines in log file are processed it multiplies *avg* to 1000 to get the total amount of the echo responses per second, dividing to number of lines and multipling to amount of benchmark services. This gives us the average number of echo responses per second for this test run. 

On my PC this number (ERps) is: **563854** 

Let's do the same without SSL support and see the difference:
  * change value of **tls** variable in ws.json to false
  * change **benchmark.target** in benchmark.lua from wss:// to ws://
  * run ```rm -f lapps.log; /opt/lapps/bin/lapps.avx2.nostats.notls > log``` from within LAppS directory, and repeat above steps.

I've got: **721236** responses per second

The difference in performance with SSL and without SSL is about 22%. Let's keep these numbers in mind for future reference.

With the same setup on Server A, I've got: **421905** ERpS with SSL and **443145** ERpS without SSL. The patches for Specter and Meltdown were disabled.
On the Server B I've got **270996** ERpS with SSL and **318522** ERpS without SSL with the patches enabled.  **385726** and **372126** without and with SSL. The patches for Specter and Meltdown were disabled as well.

The results are worse with the same setup because the CPUs on these servers have lesser frequency.

Please beware that the clients are very dependent on the data availability in /dev/urandom. It may take a while until the clients actually start running, if you already ran it once. So just wait for them to start working, then they are pretty fast at what they do. Just monitor with *top* if the LAppS instances actually doing any job at all. If /dev/urandom is exhausted then the LAppS will not eat up your CPUs until there is some data available. 

# Preparing for big tests

First of all we need to make some changes in kernel parameters and do not forget the ulimit for nr of open files. I used almost the same setup like in this [article](https://goroutines.com/10m).

Create a file with following content
```bash
:
sysctl -w fs.file-max=14000000
sysctl -w fs.nr_open=14000000
ulimit -n 14000000
sysctl -w net.ipv4.tcp_mem="100000000 100000000 100000000"
sysctl -w net.core.somaxconn=20000
sysctl -w net.ipv4.tcp_max_syn_backlog=20000
sysctl -w net.ipv4.ip_local_port_range="1025 65535"
```
Then use *source ./filename* on both servers to change the kernel parameters and the ulimit.

This time my purpose is to create several millions clients on one server and connect them to the second.

Server A will serve as the WebSockets echo service server side.
Server B will serve as the WebSockets echo service client side.

There is a limitation in LAppS imposed by LuaJIT. You can use only 2GB of RAM per LuaJIT VM. That is the limit of RAM you can use within single LAppS instance for all the services, as the services are the threads linked against one libluajit instance. If any of your services which is running under the single LAppS instance exceeds this limit  then all the services will be out of memory.

I found out that per single LAppS instance you can't establish more then 2 464 000 client WebSockets with the message size of 64 bytes. The message size may slightly change this limit, because LAppS passes the message to *cws* service by allocating the space for this message within the LuaJIT.

This implies that I have to start several LAppS instances with the same configuration on Server B to establish more then 2.4 millions of WebSockets. The Server A (the echo server) does not uses as much memory on LuaJIT side, so the one instance of the LAppS will take care of 12.3 million of WebSockets without any problem.

Let's prepare two different configs for the servers A and B.

Server A ws.json:
```JSON
{
  "listeners" : 224,
  "connection_weight": 1.0,
  "ip" : "0.0.0.0",
  "port" : 5084,
  "lapps_config_auto_save" : true ,
  "workers" : {
    "workers": 40,
    "max_connections" : 2000000,
    "auto_fragment" : false,
    "max_poll_events" : 256,
    "max_poll_wait_ms" : 10,
    "max_inbounds_skip" : 50,
    "input_buffer_size" : 2048
   },
  "acl" : {
    "policy" : "allow",
    "exclude" : []
  },
  "tls": true,
  "tls_server_version" : 4,
  "tls_client_version" : 4,
  "tls_certificates":{
    "ca":"/opt/lapps/conf/ssl/ca.crt",
    "cert": "/opt/lapps/conf/ssl/example.org.bundle.crt",
    "key": "/opt/lapps/conf/ssl/example.org.key"
  }
}
```

Server A lapps.json:
```JSON
{
  "directories": {
    "app_conf_dir": "etc",
    "applications": "apps",
    "deploy": "deploy",
    "tmp": "tmp",
    "workdir": "workdir"
  },
  "services": {
    "echo": {
      "auto_start": true,
      "instances": 40,
      "internal": false,
      "max_inbound_message_size": 16777216,
      "protocol": "raw",
      "request_target": "/echo",
      "acl" : {
        "policy" : "allow",
        "exclude" : []
      }
    }
  }
}
```

Server B ws.json:
```JSON
{
  "listeners" : 0,
  "connection_weight": 1.0,
  "ip" : "0.0.0.0",
  "port" : 5083,
  "lapps_config_auto_save" : true ,
  "workers" : {
    "workers": 0,
    "max_connections" : 0,
    "auto_fragment" : false,
    "max_poll_events" : 2048,
    "max_poll_wait_ms" : 10,
    "max_inbounds_skip" : 50,
    "input_buffer_size" : 2048
   },
  "acl" : {
    "policy" : "deny",
    "exclude" : []
  },
  "tls": true,
  "tls_server_version" : 4,
  "tls_client_version" : 4,
  "tls_certificates":{
    "ca":"/opt/lapps/conf/ssl/ca.crt",
    "cert": "/opt/lapps/conf/ssl/example.org.bundle.crt",
    "key": "/opt/lapps/conf/ssl/example.org.key"
  }
}
```

Server A has two interfaces:
  * bond0 - x.x.203.37 
  * bond1 - x.x.23.10

One is faster another is slower but it does not really matter. The server will be under heavy load anyways.

Lets prepare a template from our /opt/lapps/benchmark/benchmark.lua
```Lua
benchmark={}
benchmark.__index=benchmark

benchmark.init=function()
end

benchmark.messages_counter=0;
benchmark.start_time=time.now();
benchmark.max_connections=10000;
benchmark.target_port=0;
benchmark.target_prefix="wss://IPADDR:";
benchmark.target_postfix="/echo";
benchmark.message="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

benchmark.meter=function()
  benchmark.messages_counter=benchmark.messages_counter+1;

  local slice=time.now() - benchmark.start_time;
  if( slice  >= 1000)
  then
    print(benchmark.messages_counter.." messages received per "..slice.."ms")
    benchmark.messages_counter=0;
    benchmark.start_time=time.now();
  end
end

benchmark.run=function()
  local n=nap:new();
  local counter=1;
  n:sleep(1);
  local array={};
  local start=time.now();
  while(#array < benchmark.max_connections) and (not must_stop())
  do
    benchmark.target_port=math.random(5084,5307);
    local sock, err_msg=cws:new(benchmark.target_prefix..benchmark.target_port..benchmark.target_postfix,
    {
      onopen=function(handler)
        local result, errstr=cws:send(handler,benchmark.message,2);
        if(not result)
        then
          print("Error on websocket send at handler "..handler..": "..errstr);
        end
      end,
      onmessage=function(handler,message,opcode)
        benchmark.meter();
        cws:send(handler,message,opcode);
      end,
      onerror=function(handler, message)
        print("Client WebSocket connection is failed for socketfd "..handler..". Error: "..message);
      end,
      onclose=function(handler)
        print("Connection is closed for socketfd "..handler);
      end
    });

    if(sock ~= nil)
    then
      table.insert(array,sock);
    else
      print(err_msg);
      err_msg=nil;
      collectgarbage("collect"); -- force garbage collection on connection failure.
    end

    -- poll events once per 10 outgoing connections 
    -- this will improve the connection establishment speed
    if counter == 10
    then
      cws:eventLoop();
      counter=1
    else
      counter = counter + 1;
    end
  end

  print("Sockets connected: "..#array);
  benchmark.start_time=time.now();

  while not must_stop()
  do
    cws:eventLoop();
  end

  for i=1,#array
  do
    array[i]:close();
    cws:eventLoop();
  end
end

return benchmark;
```

Let's store Server A IP addresses into files IP1 and IP2 respectively and then:
```bash
for i in 1 2
do
  mkdir -p /opt/lapps/apps/benchmark${i}
  sed -e "s/IPADDR/$(cat IP1)/g" /opt/lapps/apps/benchmark/benchmark.lua > /opt/lapps/apps/benchmark${i}/benchmark${i}.lua
done
```

Now we modify the /opt/lapps/etc/conf/lapps.json on Server B to use these two benchmark services:
Server B lapps.json:
```json
{
  "directories": {
    "app_conf_dir": "etc",
    "applications": "apps",
    "deploy": "deploy",
    "tmp": "tmp",
    "workdir": "workdir"
  },
  "services": {
    "benchmark1": {
      "auto_start": true,
      "instances" : 112,
      "internal": true,
      "preload": [ "nap", "cws", "time" ]
    },
    "benchmark2": {
      "auto_start": true,
      "instances" : 112,
      "internal": true,
      "preload": [ "nap", "cws", "time" ]
    }
  }
}
```

Are we ready? No we are not. Because we are intent to generate 2 240 000 outgoing sockets to only two addresses and we need more ports on the server side. It is impossible to create more then 64k connections to the same ip:port pair (actually a little less then 64k).

On the Server A in the file LAppS/include/wsServer.h there is a function void startListeners(). In this function we will replace line 126 
```C++
LAppSConfig::getInstance()->getWSConfig()["port"],
```
with this:
```C++
static_cast<int>(LAppSConfig::getInstance()->getWSConfig()["port"])+i,
```
Rebuild LAppS:
```bash
make CONF=Release.AVX2 install
```
# Running the tests for 2 240 000 client WebSockets.

Start the LAppS on Server A, then start the LAppS on Server B and redirect the output to a file like this:
```bash
/opt/lapps/bin/lapps.avx2 > log
```

This may take a while till all the connections are established. Let's monitor the progress.

Do not use netstat to watch established connections, it is pointless while it runs indefinitely after like 150k connections which are established in some seconds. Look at the lapps.log on the Server A, in the directory which you were in when started LAppS. You may use following one-liner to see how the connections are established and how are they distributed among IOWorkers:

```bash
date;awk '/ will be added to connection pool of worker/{a[$22]++}END{for(i in a){ print i, a[i]; sum+=a[i]} print sum}' lapps.log | sort -n;date
```

Here is an idea on how rapidly these connections are established:

```text
375874
Sun Jul 21 20:33:07 +06 2019

650874
Sun Jul 21 20:34:42 +06 2019   2894 connections per second

1001874
Sun Jul 21 20:36:45 +06 2019   2974 connections per second

1182874
Sun Jul 21 20:37:50 +06 2019   2784 connections per second

1843874
Sun Jul 21 20:41:44 +06 2019   2824 connections per second

2207874
Sun Jul 21 20:45:43 +06 2019   3058 connections per second

```

On the Server B we may check amount of benchmarks finished establishing their connections:
```bash
# grep Sockets log | wc -l
224
```
After you see that the number is 224 let the servers work for a while, monitor the CPU and memory usage with top. You might see something like this (Server B at left and Server A at the right side):

![image](https://habrastorage.org/webt/e0/vy/sv/e0vysvislttrfhwpn_vexnwseli.png)

Or like this (Server B at left and Server A at the right side):

![](https://habrastorage.org/webt/p1/cj/qk/p1cjqk6fxmthbnnexc2ikco9d6a.png)

It is clearly the Server B, where the benchmark client services are running, is under heavy load. The Server A is under heavy load too but not always. Some times it chills while the Server B struggles with amount of tasks to work on. The traffic ciphering on TLS is very CPU intensive.

Let's stop the servers (press Ctrl-C several times) and modify the log as before but with respect to changed amount of benchmark services (224):

```bash
echo -e ':%s/ms/^V^M/g\n:%g/^$/d\nGdd1G224/Sockets\n224\ndggZZ' | vi $i
awk -v avg=0 '{rps=($1/$5);avg+=rps;}END{print ((avg*1000)/NR)*224}' log
```
You might want to delete several last lines as well, to account for print outs from stopping benchmark services. You've already got an idea on how to do this. So I'll post the results for different test scenarios and proceed with problems I've faced.

# Results for all the other tests (4.9 million and beyond)

![](https://habrastorage.org/webt/az/4a/at/az4aatexickfttbrom2gg3yrymu.png)

![Test #4](https://habrastorage.org/webt/ra/ka/_f/raka_fia7i2vnqhwh2p-5lnulc8.png)

![Test #5](https://habrastorage.org/webt/ln/z5/ml/lnz5mlaoz7aclxuyvjwjpwj25jm.png)

![Test #5 manualy balanced for CPU equal sharing](https://habrastorage.org/webt/vi/cy/fe/vicyfenhzk7uvcjsiohq950c9fa.png)

![Test #6](https://habrastorage.org/webt/7h/bg/ez/7hbgezb4zxuv3ijuemjmkilf110.png)

![Test #8](https://habrastorage.org/webt/gq/5_/uj/gq5_uje-nnkfhlqmilb7xnbu5tc.png)

![Test #12](https://habrastorage.org/webt/xa/tr/m9/xatrm9dbbaha13w9wjxzmecvxjm.png)

![Test #13](https://habrastorage.org/webt/-w/9t/i8/-w9ti8atzczna-9dvepihihlyqu.png)

# Problems. 

Marked in above table with red.

Running more then 224 benchmark instances on the Server B proved to be the bad idea, as the server on client side was incapable of evenly distribute the CPU time among processes/threads. First 224 benchmark instances which have established all their connections took most of the CPU resources and the rest of benchmark instances are lagging behind. Even using renice -20 does not help a lot(448 benchmark instances, 2 LAppS instances):
![448 benchmark instances](https://habrastorage.org/webt/a2/ms/bg/a2msbgjgzuwvzzdl332linmmq98.png)
The Server B (left side) is under very heavy load and the Server A still has free CPU resources.

So I doubled the **benchmark.max_connections**  instead of starting more of the separate LAppS instances.

Still for 12.3 million of WebSockets to run, I have started 5th LAppS instance (tests 5 and 6) without stopping four already running ones. And played the role of CFQ by manually  suspending and restarting prioritized processes with *kill -STOP/-CONT* or/and *renice* their priorities. You can use following template script for this:
```bash
while [ 1 ];
do
  kill -STOP <4 fast-processes pids>
  sleep 10
  kill -CONT <4 fast-processes pids>
  sleep 5
done
```

Welcome to 2019 RHEL 7.6! Honestly, I used the renice command first time since 2009. What is worst, - I used it almost unsuccessfully this time.

I had a problem with scatter-gather  engine of the NICs. So I disabled it for some tests, not actually marking this event into the table.

I had partial link disruptions under heavy load and the NIC driver bug,  so I had to discard related test results.

#End of story.

Honestly, the tests went much smoother than I've anticipated.

I'm convinced that I have not managed to load LAppS on Server A to its full potential (without SSL), because I had not enough CPU resources for the client side. Though with TLS 1.3 enabled, the LAppS on the Server A was utilizing almost all available CPU resources. 

I'm still convinced that LAppS is most scalable and fastest WebSockets open source server out there and the **cws** WebSockets client module is the only of it's kind, providing the framework for high load testing.

Please verify the results on your own hardware. 

Note of advice: Never use nginx or apache as the load balancer or as a proxy-pass for WebSockets , or you'll end up cutting the performance on order of magnitude. They are not build for WebSockets.
