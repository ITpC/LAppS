FROM ubuntu:xenial

LABEL "co.new-web" "new WEB() LLP" version 1.0  maintainer "pk@new-web.co" description "LAppS build environment"

RUN apt-get update \
	&& apt-get dist-upgrade -y 

RUN apt-get install -y g++-multilib

RUN apt-get install -y apt-utils 

RUN apt-get install -y vim 

RUN apt-get install -y curl 

RUN apt-get install -y wget 

RUN apt-get install -y git 

RUN apt-get install -y make

RUN apt-get install -y autotools-dev

RUN apt-get install -y libcrypto++-dev

RUN apt-get install -y libpam0g-dev

RUN apt-get install -y libbz2-dev

ENV WORKSPACE /root/workspace

RUN mkdir ${WORKSPACE}

ADD http://luajit.org/download/LuaJIT-2.0.5.tar.gz ${WORKSPACE}

WORKDIR ${WORKSPACE}

RUN tar xzvf LuaJIT-2.0.5.tar.gz

WORKDIR ${WORKSPACE}/LuaJIT-2.0.5

RUN env CFLAGS="-pipe -Wall -pthread -O2 -fPIC -march=core2 -mtune=generic -mfpmath=sse -mssse3 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fno-omit-frame-pointer" make clean all install

RUN echo "LuaJIT-2.0.5 has been build and installed"

WORKDIR ${WORKSPACE}

ADD https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-2.9.1.tar.gz ${WORKSPACE}

RUN tar xzvf libressl-2.9.1.tar.gz

WORKDIR ${WORKSPACE}/libressl-2.9.1

RUN env CFLAGS="-pipe -Wall -pthread -O2 -fPIC -march=core2 -mtune=generic -mfpmath=sse -mssse3 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fno-omit-frame-pointer" ./configure --prefix=${WORKSPACE}/libressl

RUN make all install

WORKDIR ${WORKSPACE}

RUN git clone https://github.com/ITpC/ITCLib.git

RUN git clone https://github.com/ITpC/utils.git

RUN git clone https://github.com/ITpC/ITCFramework.git

RUN git clone https://github.com/ITpC/LAppS.git

RUN git clone https://github.com/ITpC/lar.git

WORKDIR ${WORKSPACE}/LAppS

RUN make clean

