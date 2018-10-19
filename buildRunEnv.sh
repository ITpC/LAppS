#!/bin/bash

if [ -f ./common.sh ]
then
  source ./common.sh

  export MANDATORY="UBUNTU BUILD"

  chk_args "$@"

  export UBUNTU_VERSION_CHOSEN=0

  for i in xenial bionic
  do
    if [ "${UBUNTU}" == "${i}" ]
    then
      export UBUNTU_VERSION_CHOSEN=1
    fi
  done

  export BUILD_TYPE_CHOSEN=0

  for i in avx2 generic
  do
    if [ "${BUILD}" == "${i}" ]
    then
      export BUILD_TYPE_CHOSEN=1
    fi
  done

  [ ${UBUNTU_VERSION_CHOSEN} ] || die "Use --ubuntu {xenial|bionic} - only these two ubuntu versions are supported"
  [ ${BUILD_TYPE_CHOSEN} ] || die "Use --build {avx2|generic} - only these two build types are supported"


  [ -f ./VERSION ] || die "No VERSION file in current directory"

  export VERSION=$(cat ./VERSION)

  docker build -t lapps:runenv.${VERSION}.${UBUNTU}.${BUILD} -f dockerfiles/Dockerfile.lapps-runenv.${VERSION}.${UBUNTU}.${BUILD} --force-rm  .

else
  echo "This file is supposed to be executed from within the LAppS build directory (clone of https://github.com/ITpC/LAppS.git)"
fi


