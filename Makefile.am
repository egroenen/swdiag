lib_LTLIBRARIES = libswdiag.la

bin_PROGRAMS =

check_PROGRAMS =

###############################################################################

libswdiag_la_LDFLAGS = \
    $(AM_LDFLAGS) \
    -version-number $(PROJECT_version_major):$(PROJECT_version_minor):$(PROJECT_version_point) $(X_libswdiag_la_LDFLAGS)

libswdiag_la_SOURCES = \
    src/swdiag_api.c                     \
    src/swdiag_api.h                     \
    src/swdiag_cli.h                     \
    src/swdiag_cli_local.c               \
    src/swdiag_linux.c                   \
    src/swdiag_linux.h                   \
    src/swdiag_obj.c                     \
    src/swdiag_obj.h                     \
    src/swdiag_rci.c                     \
    src/swdiag_rci.h                     \
    src/swdiag_sched.c                   \
    src/swdiag_sched.h                   \
    src/swdiag_sequence.c                \
    src/swdiag_sequence.h                \
    src/swdiag_syslog.c                  \
    src/swdiag_thread.c                  \
    src/swdiag_thread.h                  \
    src/swdiag_trace.c                   \
    src/swdiag_trace.h                   \
    src/swdiag_types.h                   \
    src/swdiag_util.c                    \
    src/swdiag_util.h                    \
    src/swdiag_xos.c                     \
    src/swdiag_xos.h                    

if ADD_RPC
#
# Add RPC Network support, unconditional for now.
#
#libswdiag_la_SOURCES += \
#    src/swdiag_unix_rpc.h                \
#    src/swdiag_unix_rpc_xdr.c            \
#    src/swdiag_unix_rpc_clnt.c           \
#    src/swdiag_unix_rpc_svc.c            \
#    src/swdiag_unix_rpc.x

CLEANFILES = \
    src/swdiag_unix_rpc.h                \
    src/swdiag_unix_rpc_xdr.c            \
    src/swdiag_unix_rpc_clnt.c           \
    src/swdiag_unix_rpc_svc.c            
endif

#
# Shouldn't really add the examples to libswdiag, use
# a different target that is not installed.
#
#libswdiag_la_SOURCES += \
#    src/examples/example1_polledtest.c   

if ADD_PLATFORM_POSIX
    libswdiag_la_SOURCES += \
    src/swdiag_posix.c                   
endif
if ADD_PLATFORM_WIN32
    libswdiag_la_SOURCES += 

endif

if ADD_UTIL

#bin_PROGRAMS += swdiagcli
bin_PROGRAMS += swdiag
#bin_PROGRAMS += swdiagtest

#swdiagcli_SOURCES = src/swdiag_unix_rpc.h \
#                    src/swdiag_unix_rpc_clnt.c \
#                    src/swdiag_unix_rpc_xdr.c \
#                    util/swdiag_unix_cli.c \
#                    util/swdiag_stubs.c

#swdiag_SOURCES    = src/swdiag_unix_rpc.h \
#                    src/swdiag_unix_rpc_clnt.c \
#                    src/swdiag_unix_rpc_xdr.c \
#                    src/swdiag_unix_rpc_svc.c \
#                    src/swdiag_unix_rpc_master.c \
#                    src/swdiag_unix_rpc_slave.c \
#                    src/swdiag_linux_rpc_thread.c \
#                    util/swdiag_unix_main.c \
#                    util/swdiag.c

swdiag_SOURCES    = util/swdiag_unix_main.c \
                    util/mongoose/mongoose.c \
                    util/swdiag.c

#swdiagtest_SOURCES= src/swdiag_unix_rpc.h \
#                    src/swdiag_unix_rpc_clnt.c \
#                    src/swdiag_unix_rpc_xdr.c \
#                    src/swdiag_unix_rpc_svc.c \
#                    src/swdiag_unix_rpc_master.c \
#                    src/swdiag_unix_rpc_slave.c \
#                    src/swdiag_linux_rpc_thread.c \
#                    util/swdiag_unix_main.c \
#                    util/swdiag_test.c

swdiagcli_LDADD   = libswdiag.la $(X_LDFLAGS) -lnsl -lpthread -lrt
swdiag_LDADD      = libswdiag.la $(X_LDFLAGS) -lnsl -lpthread -lrt -ldl
swdiagtest_LDADD  = libswdiag.la $(X_LDFLAGS) -lnsl -lpthread -lrt

endif

#mp4art_SOURCES       = util/impl.h util/mp4art.cpp

#mp4art_LDADD       = libswdiag.la $(X_LDFLAGS)


###############################################################################

SUFFIXES=.x

RPCGEN := rpcgen

%.h : %.x
	$(RPCGEN) $(X_RPCGENOPTS) -o $@ -h $< 

%_xdr.c: %.x %.h
	$(RPCGEN) $(X_RPCGENOPTS) -o $@ -c $< 

%_clnt.c : %.x %.h
	$(RPCGEN) $(X_RPCGENOPTS) -o $@ -l $< 

%_svc.c : %.x %.h
	$(RPCGEN) $(X_RPCGENOPTS) -o $@ -m $< 

###############################################################################

DEJATOOL = main

TESTLOGDIR = $(builddir)/testlog

override RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir $$srcdir/testsuite --outdir $(TESTLOGDIR)

###############################################################################

libswdiagincdir = $(pkgincludedir)

libswdiaginc_HEADERS = \
    include/project.h \
    \
    include/swdiag_client.h

###############################################################################

EXTRA_DIST = project/project.m4

###############################################################################

clean-local:

distclean-local:
	rm -f $(top_builddir)/Makefile
	rm -f $(top_builddir)/testlog/*.log
	rm -f $(top_builddir)/testlog/*.sum

###############################################################################

AM_CPPFLAGS = $(strip $(MK_CXX_ARCH) $(X_CXX_ARCH) $(MK_CXX_I) $(X_CXX_I))

AM_LDFLAGS = $(strip $(MK_CXX_ARCH) $(X_CXX_ARCH))

AM_CXXFLAGS = $(strip $(MK_CXX_W) $(X_CXX_W))

LIBS := $(LIBS) $(X_MINGW_LIBS)

###############################################################################

MK_CXX_ARCH =
MK_CXX_W    = -Wall -Wformat
MK_CXX_D    =

MK_CXX_I = \
    -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_srcdir)/src \
    -I$(top_builddir)/src

###############################################################################

##
## workaround: DejaGNU adds a hard-coded dependency on Makefile
## and need to create logdir
##
Makefile: ${Makefile}
	touch $@
	$(mkdir_p) $(TESTLOGDIR)

dist-hook:

###############################################################################

##
## include GNUmakefiles which are not full-fledged automake citizens but
## they are aware of automake variables and targets. Note that we purposely
## use a GNUmakefile extension (-include) to prevent automake from parsing
## the file.
##
#-include $(top_srcdir)/doc/GNUmakefile.mk