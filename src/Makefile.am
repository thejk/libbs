MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = bs lsbs vmbs
lib_LTLIBRARIES = libbs.la

bs_SOURCES = bs.c libbs.h compiler_stuff.h
bs_CFLAGS = @DEFINES@ -DVERSION="\"@VERSION@\""
bs_LDADD = libbs.la

lsbs_SOURCES = lsbs.c libbs.h compiler_stuff.h extra_compiler_stuff.h
lsbs_CFLAGS = @DEFINES@
lsbs_LDADD = libbs.la

vmbs_SOURCES = vmbs.c libbs.h compiler_stuff.h extra_compiler_stuff.h
vmbs_CFLAGS = @DEFINES@ -DVERSION="\"@VERSION@\"" @PULSEAUDIO_CFLAGS@
vmbs_LDADD = libbs.la @PULSEAUDIO_LIBS@

libbs_la_SOURCES = libbs.h compiler_stuff.h libbs.c
libbs_la_CFLAGS = @LIB_DEFINES@ @LIBUSB_CFLAGS@
libbs_la_LIBADD = @LIBUSB_LIBS@
