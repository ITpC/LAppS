FROM ubuntu:xenial

LABEL "co.new-web" "new WEB() LLP" version 1.0  maintainer "pk@new-web.co" description "LAppS run environment"

RUN apt-get update \
	&& apt-get upgrade -y \
  && apt-get update \
	&& apt-get dist-upgrade -y 

RUN apt-get install -y apt-utils 

RUN apt-get install -y luarocks

ENV WORKSPACE /tmp

WORKDIR ${WORKSPACE}

ADD https://github.com/ITpC/LAppS.builds/raw/master/xenial/lapps-0.7.1-avx2-amd64.deb ${WORKSPACE}/

WORKDIR ${WORKSPACE}

RUN ls -la ${WORKSPACE}/lapps-0.7.1-avx2-amd64.deb

RUN apt install -y ${WORKSPACE}/lapps-0.7.1-avx2-amd64.deb

RUN apt-get install -f -y

RUN echo "LAppS-0.7.1 is installed under /opt/lapps prefix. To run LAppS use /opt/lapps/bin/lapps [-d] command. -d is an optional argument do run LAppS as a deamon."

RUN echo "Several binaries are available: lapps, lapps.nostats, lapps.notstats.notls - for generic builds; and lapps.avx2, lapps.avx2.nostats, lapps.avx2.notstats.notls. See feature request #24 for details."

RUN echo "You may add/install libraries you want to use with LAppS from luarocks repository. You may twick this Dockerfile or do these operations within running container."
