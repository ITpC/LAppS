#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++ -std=c++17 -pthread
CXX=g++ -std=c++17 -pthread
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/getLog.o \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pipe -O2 -march=nocona -mtune=generic -mfpmath=sse -msse2 -fstack-check -fstack-protector-strong -ftree-vectorize -funroll-loops -std=c++17
CXXFLAGS=-pipe -O2 -march=nocona -mtune=generic -mfpmath=sse -msse2 -fstack-check -fstack-protector-strong -ftree-vectorize -funroll-loops -std=c++17

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -L/usr/local/lib/mimalloc-1.6 -Wl,-rpath,'/usr/local/lib/mimalloc-1.6' ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a ../ITCLib/dist/Debug/GNU-Linux/libitclib.a ../utils/dist/Debug/GNU-Linux/libutils.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug: ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug: ../ITCLib/dist/Debug/GNU-Linux/libitclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug: ../utils/dist/Debug/GNU-Linux/libutils.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++ -std=c++17 -pthread -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.debug ${OBJECTFILES} ${LDLIBSOPTIONS} -flto -lwolfssl -lpam -lmimalloc -lluajit-5.1 -lstdc++fs -lbz2

${OBJECTDIR}/src/getLog.o: src/getLog.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DAPP_NAME=\"LAppS\" -DDTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DLAPPS_TLS_ENABLE -DLOG_FILE=\"lapps.log\" -DLOG_INFO -DMAX_BUFF_SIZE=512 -DMIMALLOC -DSTATS_ENABLE -DTSAFE_LOG=1 -DWC_RSA_BLINDING -DWOLFSSL_TLS13 -I../ITCFramework/include -I../ITCLib/include -I../utils/include -Iinclude -I/usr/include/luajit-2.0 -Iinclude/modules -I/usr/local/include -I/usr/local/include/luajit-2.0 -I/usr/local/lib/mimalloc-1.6/include -I../lar -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/getLog.o src/getLog.cpp

${OBJECTDIR}/src/main.o: src/main.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DAPP_NAME=\"LAppS\" -DDTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DLAPPS_TLS_ENABLE -DLOG_FILE=\"lapps.log\" -DLOG_INFO -DMAX_BUFF_SIZE=512 -DMIMALLOC -DSTATS_ENABLE -DTSAFE_LOG=1 -DWC_RSA_BLINDING -DWOLFSSL_TLS13 -I../ITCFramework/include -I../ITCLib/include -I../utils/include -Iinclude -I/usr/include/luajit-2.0 -Iinclude/modules -I/usr/local/include -I/usr/local/include/luajit-2.0 -I/usr/local/lib/mimalloc-1.6/include -I../lar -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
