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
CFLAGS += -funsigned-char $(INCLUDES) $(GENERAL)
CFLAGS += $(OPTS) $(WARN)
#CFLAGS += -ggdb -O0 -DDEBUG_LOGFILE=\"i:\\\\qed.log\" -DPREDEF_DEBUG_LEVEL=255 $(WARN)

# default definitions
OBJS = $(COBJS:.c=.o)
# LIBS += -L/usr/GEM/lib
LIBS += -lcflib -lgem
LIBS += -liio
GENFILES = $(TARGET)

$(OBJS): Makefile

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STACK) --fix 32k $@

include $(top_srcdir)/DEPENDENCIES

strip:
	$(STRIP) $(TARGET)
	
install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/qed.rsc $(srcdir)/qed.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
