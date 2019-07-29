#
# Makefile for QED
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = src

installdir = /opt/GEM
cpu = all

default: help

include $(srcdir)/QEDDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

install-here:

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(qedtargets)'
	@echo '#'
	@echo '# - install'
	@echo '# - strip'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a ColdFire binary: -> make col'
	@echo '#'
	@echo '# To install single CPU binary: ->'
	@echo '# make install cpu=<CPU> installdir=<DIR>'
	@echo '#'

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(qedtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

strip:
	@set -x; \
	for i in $(qedtargets); do \
		$(STRIP) .compile_$$i/qed_new.app; \
	done

install: $(cpu)
	$(MKDIR) -p $(installdir)
ifeq ($(cpu), all)
	@set -x; \
	for i in $(qedtargets); do \
		$(CP) .compile_$$i/qed_new.app $(installdir)/qed$$i.app; \
		chmod 755 $(installdir)/qed$$i.app; \
		$(STRIP) $(installdir)/qed$$i.app; \
	done
else
	$(CP) .compile_$(cpu)/qed_new.app $(installdir)/qed.app
	chmod 755 $(installdir)/qed.app
	$(STRIP) $(installdir)/qed.app
endif

clean::
	for i in $(qedtargets); do rm -rf .compile_$$i; done

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
