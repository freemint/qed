#
# Makefile for freemint
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
	@echo '#'
	@echo '# To install single CPU binary:'
	@echo '# make install cpu=<CPU> installdir=<DIR>'
	@echo '#'

all-here: doc/qed-en.hyp doc/qed-nl.hyp doc/qed-de.hyp

doc/qed-en.hyp: doc/qed-en.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

doc/qed-de.hyp: doc/qed-de.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

doc/qed-nl.hyp: doc/qed-nl.stg
	if test "$(HCP)" = ""; then echo "hcp not available" >&2; else \
	$(HCP) -o $@ $<; fi

distclean::
	rm -f doc/*.hyp doc/*.ref

# default overwrites

# default definitions
