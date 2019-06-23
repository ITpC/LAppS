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
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release.SSE2.NO_STATS
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
CCFLAGS=-pipe -std=c++17 -Wall -pthread -O2 -fPIC -flto -march=core2 -mtune=generic -mfpmath=sse -msse2 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fomit-frame-pointer
CXXFLAGS=-pipe -std=c++17 -Wall -pthread -O2 -fPIC -flto -march=core2 -mtune=generic -mfpmath=sse -msse2 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fomit-frame-pointer

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -L../libressl/lib ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a ../ITCLib/dist/Debug/GNU-Linux/libitclib.a ../utils/dist/Debug/GNU-Linux/libutils.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats: ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats: ../ITCLib/dist/Debug/GNU-Linux/libitclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats: ../utils/dist/Debug/GNU-Linux/libutils.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++ -pipe -std=c++17 -Wall -pthread -O2 -fPIC -flto -march=core2  -mtune=generic -mfpmath=sse -msse2 -ftree-vectorize -funroll-loops -fstack-check -fstack-protector-strong -fomit-frame-pointer -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps.sse2.nostats ${OBJECTFILES} ${LDLIBSOPTIONS} -lcryptopp -lwolfssl -lluajit-5.1 -lbz2 -lstdc++fs -lpam

${OBJECTDIR}/src/getLog.o: src/getLog.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DAPP_NAME=\"LAppS\" -DLAPPS_TLS_ENABLE -DLOG_FILE=\"lapps.log\" -DLOG_INFO -DMAX_BUFF_SIZE=512 -DTSAFE_LOG=1 -DWOLFSSL_TLS13 -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DWC_RSA_BLINDING -I../libressl/include -I../ITCFramework/include -I../ITCLib/include -I../utils/include -I../lar -Iinclude -I/usr/include/luajit-2.0 -Iinclude/modules -I/usr/local/include -I/usr/local/include/luajit-2.0 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/getLog.o src/getLog.cpp

${OBJECTDIR}/src/main.o: src/main.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DAPP_NAME=\"LAppS\" -DLAPPS_TLS_ENABLE -DLOG_FILE=\"lapps.log\" -DLOG_INFO -DMAX_BUFF_SIZE=512 -DTSAFE_LOG=1 -DWOLFSSL_TLS13 -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DWC_RSA_BLINDING -I../libressl/include -I../ITCFramework/include -I../ITCLib/include -I../utils/include -I../lar -Iinclude -I/usr/include/luajit-2.0 -Iinclude/modules -I/usr/local/include -I/usr/local/include/luajit-2.0 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Release.SSE2

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Release.SSE2 clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
