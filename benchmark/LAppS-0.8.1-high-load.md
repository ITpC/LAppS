One thing about WebSockets is that you need a lot of resources on the client's side to generate high enough load for the server to actually eat up all the CPU resources.

There are several challenges you have to overcome because the WebSockets protocol is more CPU demanding on the client's side than on the server's side. At the same time you need a lot of RAM to store information about open connections if you have millions of them.

I've been lucky enough to get a couple of new servers for a limited period of time at my disposal for the hardware "burnout" tests. So I decided to use my Lua Application Server - [LAppS](https://github.com/ITpC/LAppS) to do both jobs: test the hardware and perform the LAppS high load tests.

<cut />Let's see the details

#Challenges

First of all WebSockets on the client's side require a pretty fast RNG for traffic masking unless you want to fake it. I wanted the tests to be realistic, so I discarded the idea of faking the RNG by replacing it with a constant. This leaves the only option - a lot of CPU power. As I said, I have two servers: one with dual Intel Gold 6148 CPUs (40 cores in total) 256GB RAM (DDR4-2666) and one with quad Intel Platinum 8180 CPUs (112 cores in total) 384GB RAM (DDR4-2666). I'll be referring to them as Server A and Server B hereafter.

The second challenge - there are no libraries or test suites for WebSockets that are fast enough. Which is why I was forced to develop a WebSockets client module for LAppS last year (**cws**). 

#The tests setup

Both servers have dual port 10GbE cards. Port 1 of each card is aggregated to a bonding interface and these ports on either cards are connected to each other in RR mode. Port 2 of each card is aggregated to a bonding interface in an Active-Backup mode, connected over an Ethernet switch. Each hardware server was running the RHEL 7.6. I had to install gcc-8.3 from the sources to have a modern compiler instead of the default gcc-4.8.5. Not the best setup for the performance tests, yet beggars can't be choosers.

CPUs on Server A (2xGold 6148) have higher frequency than on Server B (4xPlatinum 8180). At the same time Server B has more cores, so I decided to run an echo server on Server A and the echo clients on Server B.

I wanted to see how LAppS behaves under high load with millions of connections and what maximum amount of echo requests per second it can serve. These are actually two different sets of tests because with the connections amount growth the server and clients are doing an increasingly complex job.

Before this I have made all the tests on my home PC which I use for development. The results of these tests will be used for comparison. My home PC has an Intel Core i7-7700 CPU 3.6GHz (4.0GHz Turbo) with 4 cores and 32GB of DDR4-2400 RAM. This PC runs Gentoo with 4.14.72 kernel.

<spoiler title="Specter and Meltdown patch levels">

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
# cat /sys/kernel/debug/x86/pti_enabled
1
# cat /sys/kernel/debug/x86/retp_enabled
0
# cat /sys/kernel/debug/x86/ibrs_enabled
1
```

I'll make an extra note with the results of the tests of Servers A and B whether these patches are enabled or not.

On my Home PC the patches are never disabled.

</spoiler>

## Installing LAppS 0.8.1

<spoiler title="Installing prerequisites and the LAppS">

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

Let's make two kinds of binaries with SSL support and statistics gathering and without (assuming we are within ${WORKSPACE}/LAppS directory):
```bash
# make clean
# make CONF=Release.AVX2
# make CONF=Release.AVX2.NO_STATS.NO_TLS
```
The resulting binaries are:
  * dist/Release.AVX2/GNU-Linux/lapps.avx2
  * dist/Release.AVX2.NO_STATS.NO_TLS/GNU-Linux/lapps.avx2.nostats.notls

Please note that the WebSockets client module for Lua is always built with SSL support. Whether it is enabled or not depends on the URI you are using for connection (ws:// or wss://) in runtime. 

</spoiler>

## Test 1. Top performance on home PC. Configuration.
### Self signed certificates
<spoiler title="certgen.sh script">
```bash
#!/bin/bash
  
openssl genrsa -out example.org.key 2048
openssl req -new -key example.org.key -out example.org.csr

openssl genrsa -out ca.key 2048
openssl req -new -x509 -key ca.key -out ca.crt
openssl x509 -req -in example.org.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out example.org.crt
cat example.org.crt ca.crt > example.org.bundle.crt
```
</spoiler>

Running this script from within the /opt/lapps/conf/ssl directory will create all required files. Here is the script's output and what I've typed in:

<spoiler title="certgen.sh output">
```text
# certgen.sh 
Generating RSA private key, 2048 bit long modulus
.................................................................................................................................................................+++++
.....................................+++++
e is 65537 (0x010001)
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:KZ
State or Province Name (full name) [Some-State]:none
Locality Name (eg, city) []:Almaty
Organization Name (eg, company) [Internet Widgits Pty Ltd]:NOORG.DO.NOT.FORGET.TO.REMOVE.FROM.BROWSER
Organizational Unit Name (eg, section) []:
Common Name (e.g. server FQDN or YOUR name) []:
Email Address []:

Please enter the following 'extra' attributes
to be sent with your certificate request
A challenge password []:
An optional company name []:
Generating RSA private key, 2048 bit long modulus
...+++++
............................................................................+++++
e is 65537 (0x010001)
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:KZ
State or Province Name (full name) [Some-State]:none
Locality Name (eg, city) []:Almaty
Organization Name (eg, company) [Internet Widgits Pty Ltd]:none
Organizational Unit Name (eg, section) []:
Common Name (e.g. server FQDN or YOUR name) []:*.example.org
Email Address []:
Signature ok
subject=C = KZ, ST = none, L = Almaty, O = NOORG.DO.NOT.FORGET.TO.REMOVE.FROM.BROWSER
Getting CA Private Key
```
</spoiler>

### LAppS configuration

Here is the WebSockets configuration file which is set for TLS 1.3, includes the above generated certificates and configured with 3 IOWorkers (see detailed description of variables in the LAppS wiki).

<spoiler title="/opt/lapps/etc/conf/ws.json">
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
</spoiler>

Configuring services: the echo server and the echo client (benchmark), each with four instances.
<spoiler title="/opt/lapps/etc/conf/lapps.json">
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
</spoiler>

The echo service is pretty trivial: 
<spoiler title="echo service source code">
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
</spoiler>

The benchmark service creates as many as **benchmark.max_connections** to **benchmark.target** and then just runs until you stop the LAppS. There is no pause in the connections' establishment. The **cws** module API resembles the Web API for WebSockets. Once all **benchmark.max_connections** are established the benchmark prints the amount of Sockets connected. Once the connection is established the benchmark sends a  **benchmark.message** to the server. After the server replies the anonymous **onmessage** method of **cws** object is invoked, which just sends the same message back to the server. 

<spoiler title="benchmark service source code">
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
    print(benchmark.messages_counter.." messages receved per "..slice.."ms")
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
</spoiler>

Now we need to place the service scripts into /opt/lapps/apps/<servicename>/<servicename>.lua:
  * /opt/lapps/apps/benchmark/benchmark.lua
  * /opt/lapps/apps/echo/echo.lua

We are ready to run our benchmark now. Just run: ```bash rm lapps.log; ./dist/Release.AVX2/GNU-Linux/lapps.avx2 > log``` from within the LAppS directory and wait for 5 mins, then press Ctrl-C once, to stop the LAppS (it will not stop immediatelly, it will shoutdown the connections first).

Ok, we've got a text file with something like this inside:
<spoiler title="benchmark output">
echo::onStart
echo::onStart
echo::onStart
echo::onStart
running
1 messages receved per 3196ms
1 messages receved per 3299ms
1 messages receved per 3299ms
1 messages receved per 3305ms
Sockets connected: 100
Sockets connected: 100
Sockets connected: 100
Sockets connected: 100
134597 messages receved per 1000ms
139774 messages receved per 1000ms
138521 messages receved per 1000ms
139404 messages receved per 1000ms
140162 messages receved per 1000ms
139337 messages receved per 1000ms
140088 messages receved per 1000ms
139946 messages receved per 1000ms
141204 messages receved per 1000ms
137988 messages receved per 1000ms
141805 messages receved per 1000ms
134733 messages receved per 1000ms
...
</spoiler>

Let's clean it up like this:
  * open the file in vim
  * type 4/Sockets<Enter>4<Enter><Ctrl>-G and see what line you are on
  * type 1G<line number you were on in previous step>dd```. All the trash lines will be removed
  * Now type :%s/ms//g<Enter>:%g/^$/d<Enter> to remove all the *ms* substrings and remove empty lines
  * type ZZ

File is saved and you are back in shell now, and the log file is ready for the next step:
```awk
# awk -v avg=0 '{rps=($1/$5)*1000;avg+=rps;}END{print (avg/NR)*4}' log
```

As result you will see the average amount of echo responses per second for all four benchmark service instances.  

On my PC this number is: 563854

Let's do the same without SSL support and see the difference:
  * change value of **tls** variable in ws.json to false
  * change **benchmark.targe** in benchmark.lua from wss:// to ws://
  * run ```bash rm lapps.log; ./dist/Release.AVX2.NO_STATS.NO_TLS/GNU-Linux/lapps.avx2.nostats.notls > log``` from within LAppS directory, and repeat above steps.

I've got: 721236

The difference in performance with SSL and without SSL is about 22%.



