#
# Makefile for qed
#

SHELL = /bin/sh
SUBDIRS = src

srcdir = .
top_srcdir = .
subdir = 

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '#'
	@echo '# - install'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '# - doc'
	@echo '#'
	@echo '# To install single CPU binary:'
	@echo '# make install cpu=<CPU> installdir=<DIR>'
	@echo '#'

all-here install-here: doc

doc: doc/qed-en.hyp doc/qed-nl.hyp doc/qed-de.hyp

doc/qed-en.hyp: doc/qed-en.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

doc/qed-de.hyp: doc/qed-de.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

doc/qed-nl.hyp: doc/qed-nl.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

install:
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

distclean::
	rm -f doc/*.hyp doc/*.ref

# default overwrites

# default definitions
