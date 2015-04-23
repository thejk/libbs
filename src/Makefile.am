MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = bs
lib_LTLIBRARIES = libbs.la

bs_SOURCES = bs.c libbs.h compiler_stuff.h
bs_LDADD = libbs.la

libbs_la_SOURCES = libbs.h compiler_stuff.h libbs.c
libbs_la_CFLAGS = @LIBUSB_CFLAGS@
libbs_la_LIBADD = @LIBUSB_LIBS@
