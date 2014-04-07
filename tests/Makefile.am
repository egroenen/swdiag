## Process this file with automake to produce Makefile.in

TESTS = check_obj check_sched
check_PROGRAMS = check_obj check_sched

check_obj_SOURCES = check_obj.c $(top_builddir)/src/swdiag_obj.h
check_obj_CFLAGS = @CHECK_CFLAGS@ -I../include
check_obj_LDADD = $(top_builddir)/libswdiag.la @CHECK_LIBS@ -lm -lrt -lpthread

check_sched_SOURCES = check_sched.c $(top_builddir)/src/swdiag_sched.h
check_sched_CFLAGS = @CHECK_CFLAGS@ -I../include
check_sched_LDADD = $(top_builddir)/libswdiag.la @CHECK_LIBS@ -lm -lrt -lpthread
