#
# Makefile for QED
#
TARGET = qed_new.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = src

installdir = /tmp/qed

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
#INCLUDES += -I/usr/GEM/include
CFLAGS += $(INCLUDES)
#CFLAGS += -D_GNU_SOURCE
#CFLAGS += -DDEBUG
CFLAGS += -g
#CFLAGS += -mshort

# default definitions
OBJS = $(COBJS:.c=.o)
# LIBS += -L/usr/GEM/lib
LIBS += -lcflib -lgem
LIBS += -liio
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES

install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/qed.rsc $(srcdir)/qed.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
