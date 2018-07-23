#
#  There exist several targets which are by default empty and which can be 
#  used for execution of your targets. These targets are usually executed 
#  before and after some main targets. They are: 
#
#     .build-pre:              called before 'build' target
#     .build-post:             called after 'build' target
#     .clean-pre:              called before 'clean' target
#     .clean-post:             called after 'clean' target
#     .clobber-pre:            called before 'clobber' target
#     .clobber-post:           called after 'clobber' target
#     .all-pre:                called before 'all' target
#     .all-post:               called after 'all' target
#     .help-pre:               called before 'help' target
#     .help-post:              called after 'help' target
#
#  Targets beginning with '.' are not intended to be called on their own.
#
#  Main targets can be executed directly, and they are:
#  
#     build                    build a specific configuration
#     clean                    remove built files from a configuration
#     clobber                  remove all built files
#     all                      build all configurations
#     help                     print help mesage
#  
#  Targets .build-impl, .clean-impl, .clobber-impl, .all-impl, and
#  .help-impl are implemented in nbproject/makefile-impl.mk.
#
#  Available make variables:
#
#     CND_BASEDIR                base directory for relative paths
#     CND_DISTDIR                default top distribution directory (build artifacts)
#     CND_BUILDDIR               default top build directory (object files, ...)
#     CONF                       name of current configuration
#     CND_PLATFORM_${CONF}       platform name (current configuration)
#     CND_ARTIFACT_DIR_${CONF}   directory of build artifact (current configuration)
#     CND_ARTIFACT_NAME_${CONF}  name of build artifact (current configuration)
#     CND_ARTIFACT_PATH_${CONF}  path to build artifact (current configuration)
#     CND_PACKAGE_DIR_${CONF}    directory of package (current configuration)
#     CND_PACKAGE_NAME_${CONF}   name of package (current configuration)
#     CND_PACKAGE_PATH_${CONF}   path to package (current configuration)
#
# NOCDDL


# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin


# build
build: .build-post

.build-pre:
# Add your pre 'build' code here...

.build-post: .build-impl
# Add your post 'build' code here...


# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# build tests
build-tests: .build-tests-post

.build-tests-pre:
# Add your pre 'build-tests' code here...

.build-tests-post: .build-tests-impl
# Add your post 'build-tests' code here...

install: build
	mkdir -p /opt/lapps/etc/conf
	mkdir -p /opt/lapps/deploy
	mkdir -p /opt/lapps/tmp
	mkdir -p /opt/lapps/etc
	mkdir -p /opt/lapps/conf/ssl
	mkdir -p /opt/lapps/bin
	mkdir -p /opt/lapps/lib
	mkdir -p /opt/lapps/apps
	mkdir -p /opt/lapps/share
	install -m 0755 ${CND_ARTIFACT_PATH_${CONF}} /opt/lapps/bin

install-examples: install
	mkdir -p /opt/lapps/apps/echo
	mkdir -p /opt/lapps/apps/echo_lapps
	mkdir -p /opt/lapps/apps/time_broadcast
	mkdir -p /opt/lapps/apps/broadcast_blob
	install -m 0644 ${CND_BASEDIR}/examples/echo/ssl/* /opt/lapps/conf/ssl/
	install -m 0644 ${CND_BASEDIR}/examples/echo/echo.lua /opt/lapps/apps/echo/
	install -m 0644 ${CND_BASEDIR}/examples/echo_lapps/* /opt/lapps/apps/echo_lapps/
	install -m 0644 ${CND_BASEDIR}/examples/time_broadcast/* /opt/lapps/apps/time_broadcast/
	install -m 0644 ${CND_BASEDIR}/examples/broadcast_blob/* /opt/lapps/apps/broadcast_blob/

clone-luajit:
	cp -RpP /usr/local/lib/* /opt/lapps/lib/
	cp -RpP /usr/local/man/* /opt/lapps/man/
	cp -RpP /usr/local/share/* /opt/lapps/share/

clone-libressl:
	cp -RpP ${CND_BASEDIR}/../libressl/etc/* /opt/lapps/etc/
	cp -RpP ${CND_BASEDIR}/../libressl/lib/* /opt/lapps/lib/
	cp -RpP ${CND_BASEDIR}/../libressl/share/* /opt/lapps/share/

build-deb: install-examples clone-luajit clone-libressl
	mkdir -p /opt/distrib/lapps-${VERSION}-amd64/opt/lapps
	mkdir -p /opt/distrib/lapps-${VERSION}-amd64/etc/ld.so.conf.d
	mkdir -p /opt/distrib/lapps-${VERSION}-amd64/DEBIAN
	mkdir -p /opt/lapps/packages
	cp ${CND_BASEDIR}/dpkg/control /opt/distrib/lapps-${VERSION}-amd64/DEBIAN/
	install -m 0755 ${CND_BASEDIR}/dpkg/postinst /opt/distrib/lapps-${VERSION}-amd64/DEBIAN/
	cp ${CND_BASEDIR}/dpkg/lapps.conf /opt/distrib/lapps-${VERSION}-amd64/etc/ld.so.conf.d/lapps.conf
	cp -RpP /opt/lapps/[^p]* /opt/distrib/lapps-${VERSION}-amd64/opt/lapps/
	cd /opt/distrib && dpkg-deb --build lapps-${VERSION}-amd64
	cp /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/
	
# run tests
test: .test-post

.test-pre: build-tests
# Add your pre 'test' code here...

.test-post: .test-impl
# Add your post 'test' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...



# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk
