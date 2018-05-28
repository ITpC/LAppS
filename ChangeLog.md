* 23-28.05.2018 - 0.7.0 - replaced mutexes with userspace locks based on atomic flags (CAS locks with forced context switch). EventBus is removed from workers. IO is non blocking. fixed bug in benchmark. 0.7.0 requires ITCLib/sys/atomic_mutex.h now. 
* 19.05.2018 - 0.6.3 branched-out and marked stable. 0.7.0 branch is in upstream - experimental work on performance improvements.
* 09.05.2018 - 0.6.2; New features: service configuration option max_inbound_message_size to limit size of inbound messages
* 09.05.2018 - 0.6.2; New features: workers configuration option: max_connections  to limit maximum amount of connections per worker (10 000 by default)
* 09.05.2018 - 0.6.2; New features: workers configuration option: auto_fragment, - to autofragment outbound messages (default value: false)
* 08.05.2018 - 0.6.1 LAppS stabilization improvments (development towards dynamic services deployment). 
* 08.05.2018 - deb package fixed (it was including previous versions deb packages).
* 08.05.2018 - Standalone/Decoupled apps are documented in wiki
* 08.05.2018 - runtime environment Dockerfile for 0.6.1 is added
* 07.05.2018 - Decoupled apps aka InternalApps are added. no regression so far. 0.6.0 may be considered as a new experimental version.
* 07.05.2018 - InternalApps forward development. VERSION bump to 0.6.0
* 07.05.2018 - 0.5.3 considered stable.
