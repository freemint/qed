#
# Makefile for QED
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = src

installdir = /opt/GEM

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
	@set -x; \
	for i in $(qedtargets); do \
		$(STRIP) .compile_$$i/qed_new.app; \
	done

install: all
	$(MKDIR) -p $(installdir)

	@set -x; \
	for i in $(qedtargets); do \
		cd .compile_$$i/; \
		$(CP) qed_new.app $(installdir)/qed$$i.app; \
		chmod 755 $(installdir)/qed$$i.app; \
		$(STRIP) $(installdir)/qed$$i.app; \
		cd ..; \
	done

	$(CP) "$(top_srcdir)/en/qed_en.rsc" "$(installdir)/qed.rsc"
	$(CP) "$(top_srcdir)/en/readme.txt" "$(installdir)"
	$(CP) "$(top_srcdir)/en/readme.old" "$(installdir)"
	$(CP) -r "$(top_srcdir)/en/syntax" "$(installdir)"
	$(CP) "$(top_srcdir)/dist/qed.cfg" "$(installdir)"
	$(CP) "$(top_srcdir)/dist/icons.rsc" "$(installdir)"
	$(CP) -r "$(top_srcdir)/dist/kurzel" "$(installdir)"
	$(CP) -r "$(top_srcdir)/dist/treiber" "$(installdir)"
	$(MKDIR) -p "$(installdir)/doc"
	$(CP) "$(top_srcdir)/doc/qed-en.hyp" "$(installdir)/doc/qed.hyp"
	$(CP) "$(top_srcdir)/doc/qed-en.ref" "$(installdir)/doc/qed.ref"

	$(MKDIR) -p "$(installdir)/de"
	$(CP) "$(top_srcdir)/src/qed.rsc" "$(installdir)/de"
	$(CP) -r "$(top_srcdir)/tools/syntax" "$(installdir)/de"
	$(CP) "$(top_srcdir)/dist/liesmich.txt" "$(installdir)/de"
	$(CP) "$(top_srcdir)/dist/liesmich.old" "$(installdir)/de"

	$(MKDIR) -p "$(installdir)/de/doc"
	$(CP) "$(top_srcdir)/doc/qed-de.hyp" "$(installdir)/de/doc/qed.hyp"
	$(CP) "$(top_srcdir)/doc/qed-de.ref" "$(installdir)/de/doc/qed.ref"

	$(MKDIR) -p "$(installdir)/fr"
	$(CP) "$(top_srcdir)/fr/qed_fr.rsc" "$(installdir)/fr/qed.rsc"
	$(CP) "$(top_srcdir)/fr/NdT.txt" "$(installdir)/fr"
	$(CP) -r "$(top_srcdir)/fr/syntax" "$(installdir)/fr"

	$(MKDIR) -p "$(installdir)/nl"
	$(CP) "$(top_srcdir)/nl/qed_nl.rsc" "$(installdir)/nl/qed.rsc"
	$(CP) "$(top_srcdir)/nl/leesmij.txt" "$(installdir)/nl"
	$(CP) -r "$(top_srcdir)/nl/syntax" "$(installdir)/nl"
	$(CP) -r "$(top_srcdir)/nl/kurzel" "$(installdir)/nl"

	$(MKDIR) -p "$(installdir)/nl/doc"
	$(CP) "$(top_srcdir)/doc/qed-nl.hyp" "$(installdir)/nl/doc/qed.hyp"
	$(CP) "$(top_srcdir)/doc/qed-nl.ref" "$(installdir)/nl/doc/qed.ref"

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
