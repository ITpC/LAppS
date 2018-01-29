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
	${OBJECTDIR}/src/lookupAppError.o \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pipe -Wall -pthread -D_REENTRANT -D_THREAD_SAFE -O3 -fPIC -fomit-frame-pointer -fstack-check -fstack-protector-strong -mfpmath=sse -msse2avx -ftree-vectorize -funroll-loops -DBZ_NO_STDIO -DLOG_DEBUG -DMAX_BUFF_SIZE=256 -DTSAFE_LOG=1 -DLOG_FILE=\"lapps.log\" -DAPP_NAME=\"LAppS\"
CXXFLAGS=-pipe -Wall -pthread -D_REENTRANT -D_THREAD_SAFE -O3 -fPIC -fomit-frame-pointer -fstack-check -fstack-protector-strong -mfpmath=sse -msse2avx -ftree-vectorize -funroll-loops -DBZ_NO_STDIO -DLOG_DEBUG -DMAX_BUFF_SIZE=256 -DTSAFE_LOG=1 -DLOG_FILE=\"lapps.log\" -DAPP_NAME=\"LAppS\"

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../libressl/lib ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a ../ITCLib/dist/Debug/GNU-Linux/libitclib.a ../utils/dist/Debug/GNU-Linux/libutils.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ../ITCFramework/dist/Debug/GNU-Linux/libitcframework.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ../ITCLib/dist/Debug/GNU-Linux/libitclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ../utils/dist/Debug/GNU-Linux/libutils.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lapps ${OBJECTFILES} ${LDLIBSOPTIONS} -lcryptopp -ltls -lcrypto

${OBJECTDIR}/src/getLog.o: src/getLog.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../ITCFramework/include -I../ITCLib/include -I../utils/include -Iinclude -I../libressl/include -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/getLog.o src/getLog.cpp

${OBJECTDIR}/src/lookupAppError.o: src/lookupAppError.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../ITCFramework/include -I../ITCLib/include -I../utils/include -Iinclude -I../libressl/include -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/lookupAppError.o src/lookupAppError.cpp

${OBJECTDIR}/src/main.o: src/main.cpp nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../ITCFramework/include -I../ITCLib/include -I../utils/include -Iinclude -I../libressl/include -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../ITCFramework && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../ITCLib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../utils && ${MAKE}  -f utils-Makefile.mk CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
