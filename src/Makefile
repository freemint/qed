#
# Makefile for QED
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = src

installdir = /tmp/qed

default: all-here

include $(srcdir)/QEDDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(qedtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

strip:
	$(STRIP) $(TARGET)
	
install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/qed.rsc $(srcdir)/qed.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)

$(qedtargets):
	$(MAKE) buildqed qed=$@

#
# multi target stuff
#

ifneq ($(qed),)

compile_dir = .compile_$(qed)
qedtarget = _stmp_$(qed)
realtarget = $(qedtarget)

$(qedtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildqed: $(realtarget)
