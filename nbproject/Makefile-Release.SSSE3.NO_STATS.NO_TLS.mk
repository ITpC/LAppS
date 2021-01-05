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
CND_CONF=Release.SSSE3.NO_STATS.NO_TLS
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pipe -std=c++17 -pthread -O2 -march=nocona -mtune=generic -mfpmath=sse -mssse3 -fstack-check -fstack-protector-strong -ftree-vectorize -funroll-loops
CXXFLAGS=-pipe -std=c++17 -pthread -O2 -march=nocona -mtune=generic -mfpmath=sse -mssse3 -fstack-check -fstack-protector-strong -ftree-vectorize -funroll-loops

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -L/usr/local/lib/mimalloc-1.6 -Lllhttp/build/out/Default/obj.target -Wl,-rpath,'/usr/local/lib/mimalloc-1.6' ../ITCLib/dist/Debug/GNU-Linux/libitclib.a ../utils/dist/Debug/GNU-Linux/libutils.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: llhttp/build/out/Default/obj.target/libllhttp.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ../ITCLib/dist/Debug/GNU-Linux/libitclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ../utils/dist/Debug/GNU-Linux/libutils.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++ -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps ${OBJECTFILES} ${LDLIBSOPTIONS} -std=c++17 -pthread -flto -lwolfssl -lpam -lmimalloc -lluajit-5.1 -lstdc++fs -lbz2 -lfmt

${OBJECTDIR}/src/main.o: src/main.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Wall -DAPP_NAME=\"LAppS\" -DDTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DLOG_FILE=\"lapps.log\" -DLOG_INFO -DMAX_BUFF_SIZE=512 -DMIMALLOC -DTSAFE_LOG=1 -DWC_RSA_BLINDING -DWOLFSSL_TLS13 -I../ITCLib/include -I../utils/include -Iinclude -I/usr/include/luajit-2.0 -Iinclude/modules -I/usr/local/include -I/usr/local/include/luajit-2.0 -I/usr/local/lib/mimalloc-1.6/include -I../lar -Illhttp/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Release
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Release
	cd ../lar && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug clean
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Release clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Release clean
	cd ../lar && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
