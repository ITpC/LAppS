# 0.7.x branch changes

76d693c deployment descriptor and stop condition
ee62516 formatting
49ce263 auto save of the config file in pretty print
9cbe03d control of internal services lifetime moved from InternalAppContext into InternalApplication
080bbb1 deployment for internal apps is fixed
04e9840 0.7.0 packages links are added
c734f37 build separation fix
f8caee7 comments fixed
7e4d2e2 build separation
b7e2606 build sequence fix
c202894 Release separation for AVX2 and SSSE3 builds
d2d0c3b autobahn testsuite results
ba7e8df 0.7.0 proper. no console
bedc0a4 64 bit-xor for fragmented messages with fragments larger 8192 bytes
03563e1 0.7.0 prelimenary build
454dc1f libbz2-dev was missing. fixed
c237293 lar was missing. fixed
0146b10 console.WIP
e553ede bump docker infrastructure to build LAppS 0.7.0
1132740 removed gprof linking
2b2ce8e removed commented lines
a493848 more optimizations
7980cd6 4x performance improvement on large payloads (>1024 bytes)
975a374 instance unique id and name forwarding fix
0c0d915 WS stream parser refactoring and cleanup
e1bec18 added ignore of a benchmark/benchmark_nontls binary file
01f35e3 removed remarked line
8520488 optional variable check `NONTLS' is added to have a possibility to run NONTLS benchmarking
6d9ad75 exception error message fix
bf5169c members declaration indent fix
d01d66d comment fixed
1b3f89a simplify hash acquiring code
0af611a Closes: #11, Instance ID support for internal services
cae3e57 moving nonce singleton declaration to a separate file
bf35500 Closes: #10 , support for unique id of the running instances of the service <internal=false>
8a0b7f4 fix: #9
ffab485 fix: #8
721d5f3 Merge branch 'master' of github.com:ITpC/LAppS
159d26b console WIP
c29ae09 murmur hash and time_now modules are added
5ea66f8 fix: #6
dbdeaec added LAPPS_APPS_DIR environment variable
17431ce fix: #7
2966653 fix: #6
f5a6e82 removed debuging line
92d4c13 fixed default config
4bb1237 Service-level network ACL has been added (on a Shakespeer level at handshake)
973ff81 Global network ACL (filter on TCPListener level) is implemented)
1001222 pam_auth:login() arguments length check added to avoid pam calls with empty passwords
983cebe console app, initial commit
2bf1e85 PAM authentication module is added for console app
3fe98fc Upstream autobahn testsuite report for latest fixes
23e763e added filtering of temporary files
5368bb7 connection termination and proper failing fix
f3c8e97 benchmarking fix
62c463a gperftool profiler is removed from library dependencies
b45330b fixed an error caused by yesterdays commit with optimization on fragmented messages
ccfe12e slight optimizations. ring buffer for messages is added
97b85cf 0.7.0.1
f9edec2 console app, initial commit
e684834 authentication module (just some lines not finished yet)
59557e7 Non disruptive restart of deployed applications (connections are persist and moved to new applications). This must be tested more, much more. Apps stream can be disrupted anyways.
4c91c1c removed second copy of echo.lar
11d821c Deployer is working. Example of an archived service is added to examples/ dir
dd922ac Separate deployer for local and network deployment with LAR support. Not fully tested
fc33456 ApplicationRegistry is counting instances now, unRegApp method is added, CMA for inbound messages is corrected, in-message buffer is adjusted by CMA
b87fb3d cma fix
7708534 0.7.1:WIP
309535e 0.7.1:WIP
c23b26f reader for preads option
8db18b4 preads option is added for parallel reads
9b68b74 removed useless file
07fa99a 0.7.1RC parallel send, dynamic service start/stop infrastructure
e83c6b0 removed workerscache, std::mutex is replaced with itc::sys::AtomicMutex
b9a11cd std::mutex replaced with itc::sys::AtomicMutex
c202136 benchmark fix
4ba9d29 ignore binaries, test results and profiling output
16940e6 removed commented line
df29b10 changed from posix mutex to AtomicMutex
158788b removed useless mutex
886aecb removed useless mutex
753d272 0.7.0 work in progress
35f8701 uWebsockets examples used in benchmarking are added
2f16caf width update for tables
cd183ad WIP: ongoing work on performance optimization
fdc50fe cleaning the code
bdab8ab Merge pull request #2 from ITpC/0.7-highperf
65cf903 Merge pull request #1 from ITpC/0.6.3-stable
4f676ba (origin/0.6.3-stable) license file for nlohmann json
0b532f0 (origin/0.7-highperf) VERSION bump
bd32488 LAppS-0.7.0 experimental high performance branch

# 0.6 and before

00adcfd toward perfopts
fe8f581 removed useless file
4b81e4d Merge branch 'master' of github.com:ITpC/LAppS
3260774 incorrect exit out of switch
e0f42be ckmob
a3a5239 LibreSSL updated to 2.7.3
b09c77f package and runenv script bump
bb0a0a2 0.6.3 stability improvments, slight performance improvments
facdd8b 0.6.3 regression test results
9e4b765 correced
9d420a5 defaults for demo
c5871f6 nebeans configuration auto-update
ce56657 side-notes correction
c330f5d link to deb package is fixed
959c937 running xavante with client-app conf
f16f060 removed test line
46202fe fix for xavante stop behavior
1d7ea7d fix for xavante
0d8270c one more fix for deb
90c8938 config file for xavante
61c9b26 xavante service
e249929 LAppS-0.6.2 hotfix, environment variables
ca853e1 LAppS-0.6.2 fix: config validation
2c29307 Config validation fixed
8520334 shell-scripts to build and run LAppS in docker
967c217 shell script to build docker runenv for LAppS-0.6.2
3ff4b29 fixing dockerfile for runenv 0.6.2
f52daba deb package for LAppS-0.6.2 is added
ebdb3f1 broadcast_blob example added in examples installation
f5060a8 auto_fragment default value: false
b82942c VERSION bump
e35dbd6 LAppS-0.6.2. New features: option for inbound message limit per service; optioanl outbound messages auto-fragmentation; option for workers to limit amount of connections
f339cc7 changelog update
1ecebfd Dockerfile for LAppS runtime is prepared. See the changes in dockerfiles/Dockerfile.lapps-runenv.0.6.1
c973a2f deb package for 0.6.0 is removed
19ebb99 deb package refresh
3b38a92 LAppS-0.6.1
1485ab6 time_broadcast intallation is added
714f822 code refining towards the dynamic deployment
f018cb8 version 0.6.0 package is added. 0.5.1 is removed
5938753 changelog bump
c4e56ab decoupled apps support
133d97b useless declaration is removed
2f9f3c4 formatting
c4dbba4 useless declaration is removed
53c7057 broadcast moved to new example for decoupled apps: time_broadcast. echo_lapps subscribes clients on request to broadcast channel, but the broadcast itself is done from time_broadcast service
de8a526 decoupled apps implementation
0f2214d module for usleep - microseconds sleep, nsleep nanoseconds sleep, sleep seconds granular sleep
5d3a159 bcast_create() fix
2ef2eb0 0.6 branch is in development. features: internal apps
2c413ba Internal apps are addded
49d6b73 refactoring common use methods moved to abstract/ApplicationContext.h
d59504e added profiling directory to gitignore
f1e24a2 error messages changed. nothing significant
5c97169 perftest results added (requested by habrahabr.ru user)
59ec0ff default parameters tuning for test on 4 cores i7
011f097 regression test bump
69827cf Special note for performance results is added
f2c2b2d result for nginx test has been added
e9b0c63 location section of nginx.conf used for test
78519e0 deb package version bump
b5ddb07 deb package version bump
07ecb5f auto-build
26f3696 test results bomped
6d34371 checking for nullptr added
491bac8 default for listeners changed to 3
18c90ed mWorkersCache initialization
bbffdd8 remarked useless locking
5b1a515 deadlock regression fixed for onDisconnect method
f490766 removed some debugging code
c5fa5e7 bcast fix-bump
c33d045 bcast-fix
3f3b366 png resolution changed
cb588d3 LAppS arch image
ff092ad Merge branch 'master' of github.com:ITpC/LAppS Issue templates merged with local changes
c75460a TODO moved to project's kanban
79811ae Update issue templates
c4b12d6 Update issue templates
465c4ce package is bumped to lapps-0.5.1
a840cdb build-deb postinst script added
4f14e5b build-deb fix2
3506bd7 build-deb fix1
9501265 build-deb make target
d51779f Broadcast fixed
05433d4 fixed permission for bcast module to be available only in an application context with LAppS protocol
329f400 comments
ec1ba54 LAppS-0.5 Ubuntu xenail amd64 package
33806b0 deb package for ubuntu xenial is provided
7306efc echo_lapps example installation fixed2
3a90a0f echo_lapps example installation fixed
bb72b1b echo_lapps example installation added
c42b158 deb package control file is added
c2d314b sigsegv fix
e4c4c01 -march=native, -mtune=generic
d250879 comments changed. usless notification is removed
0274669 demonization argument check is fixed
8fae691 demonization with -d command line argument is added
d17a368 wsServer is moved to LAppS namespace
b757979 bcast module is added
69b8ddd ud2json() and get_userdata_value() are moved to include/modules/UserDataAdapter.h
d14b815 removed commented code
f2206a3 moved to LAppS namespace
ce1b7df TODO fixed
0a85066 wsServer::run() moved to main()
3cb07df removed run() method. connection weighting based on configuration parameter is implemented
28c8450 wsServer configuration attribute connection_weight was added (affects load balancing)
cb4d75f postpone application instance shutdown, until it is released by application
981d218 performance results with load balancer are added
6d2f8ef Balancer for inbound connections is added
0d1f7e4 Event Queue Size attribute is added
462a76b Stats update is partially  enabled for the balancer
9cfa396 size() method is added
a669995 removed commented line
7729527 improvments done according to documentation
5f1f903 LAppS protocol echo app updated to fit specification
6e48146 side notes updated. Performance is degraded because of EventBus use. Will be fixed later
ac1ff45 params specification detalization
5b76ff6 using 'auto' instead of type specification
9e67822 using 'auto' instead of type specification
4b95cd7 fix: dereference of unassigned shared_ptr on socket close with no handshake
f5e0b35 LAppS-protocol implemented. echo_lapps demo application is added to demonstrate LAppS stack
7b9626c automatic build-configuration change
abb5734 changed startup notification from log-debug to log-info level
ddd6d35 added 2 interface methods: submitError, submitResponses (batch submission)
192024b changed includes
ba68205 added new constructor for ServerMessage
8eea240 added messages batch-submission
928521f Added support for LAppS protocol messaging (input messages validation)
675792a error notifications changed. `this\' submitted to ApplicationContext as a parent
e8348a3 formatting
d1a146f added ws::send() for sending from within lua apps, with RAW and LAppS protocols support
5ff1f51 LAppS protocol formatting fixed
26f8359 LAppS protocol specification is updated
0af299e testing multi-instance work
4eb1cba shutdown notification changed from log-debug to log-info level
546c3c7 batch update impl
1f5c41a getWorker() method is added
61d8776 separate messages header constructor
bfa956e batch update impl
2fa62e8 batch update impl
4f1c062 duplicated code moved to method cleanLuaStack()
7636f58 workers caching
8edb8e1 include/WSStreamProcessor.h enum range check fix; worker id is size_t now - fixed
8fb84bf state change bugfix (allow sames state re-set)
d5f3ee6 corrected Makefile
522ef0a reordering config. removed workers restriction
9b19df8 Spec markup update
ca514a3 Docker files for base, build and run environments
a7f2f0d fix install target
c6bb141 changes for default config to run within docker instance
175b7c7 changes for default config to run within docker instance
024f45d added targets for docker-build
28bdb53 install and install-examples targets are added
de1034d added TODO
1f283e4 Refactoring. lua echo app is running now
2e73eac Refactoring: Shaker runs IO over thread pool. LAppS protocol specification is added (no working implementation yet). nljson module is added. LAppS is capable to load applications now. Code is just compiling and run until first app load, everything needs excessive testing, debugging and fixing
664d799 Code refactoring: handshake phase moved out of workers to listeners, stats added, internal communication queues
081739f LAppS Core v0.0.1
4014c16 socket's close on different WebSocket states
1076b59 libtls thread safety, TLSServerContext is shared between workers now. lockfuncs and idfuncs for libtls are added
f5991c1 excluded private from respository
8e6380c excluded nbproject/private/ from respository
111c1e5 1. Fix: tls server context and config are required to be singletons 2. Refactoring: rename ConnectionsWorker to WSWorker
54c9452 Modified link to Autobahn TestSuite Results
8b8b758 Added link to Autobahn Results
b97a243 removed build files
a90ed89 Autobahn Testsuite Results
d4f3602 Initial import, WebSockets Protocol Implementation, non-SSL and TLSv1.2
d8c0f6f Initial commit
