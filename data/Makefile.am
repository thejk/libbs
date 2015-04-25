MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = 50-blinkstick.rules

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbs.pc

if INSTALL_UDEV_RULES
RULES = 50-blinkstick.rules
endif

udevrulesdir = /etc/udev/rules.d
udevrules_DATA = $(RULES)
