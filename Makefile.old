#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IMPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = xine

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
#CXX       = /soft/gcc-4.1-20060113/bin/g++ -I/soft/include -L/soft/lib
CXXFLAGS ?= -g -O3 -Wall -Woverloaded-virtual
#CXXFLAGS ?= -g3 -O0 -Wall -Woverloaded-virtual
#CXXFLAGS ?= -g3 -O0 -Wall -Woverloaded-virtual -Wformat=2 -Wextra

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Make sure that necessary options are included:

-include $(VDRDIR)/Make.global

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

INCLUDES += `pkg-config --cflags libxine` 

# where to create fifos (xine expects them at /tmp/vdr-xine)
VDR_XINE_FIFO_DIR ?= /tmp/vdr-xine

# can be used to detect inefficient OSD drawing
# 0 - do not verify whether the dirty area of a bitmap is really dirty
# 1 - verify that bitmap is really dirty and print a message on console when it is not
# 2 - additionally fail with a core dump
VDR_XINE_VERIFY_BITMAP_DIRTY ?= 0

# enable to fully support yaepg plugin
#VDR_XINE_SET_VIDEO_WINDOW = 1

# where are these utilities for image grabbing? (default: anywhere on your PATH)
#VDR_XINE_Y4MSCALER = /usr/bin/y4mscaler
#VDR_XINE_Y4MTOPPM  = /usr/bin/y4mtoppm
#VDR_XINE_PNMTOJPEG = /usr/bin/pnmtojpeg

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(shell (sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h ; sed -ne '/define VDRVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h) | sed -ne 1p)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

DEFINES += -DFIFO_DIR=\"$(VDR_XINE_FIFO_DIR)\"

DEFINES += -DVERIFY_BITMAP_DIRTY=$(VDR_XINE_VERIFY_BITMAP_DIRTY)

ifdef VDR_XINE_SET_VIDEO_WINDOW
DEFINES += -DSET_VIDEO_WINDOW
endif

ifdef VDR_XINE_Y4MSCALER
DEFINES += -DY4MSCALER=\"$(VDR_XINE_Y4MSCALER)\"
endif

ifdef VDR_XINE_Y4MTOPPM
DEFINES += -DY4MTOPPM=\"$(VDR_XINE_Y4MTOPPM)\"
endif

ifdef VDR_XINE_PNMTOJPEG
DEFINES += -DPNMTOJPEG=\"$(VDR_XINE_PNMTOJPEG)\"
endif

### The object files (add further files here):

OBJS = $(PLUGIN).o xineDevice.o xineLib.o xineOsd.o xineSettings.o xineSetupPage.o xineRemote.o xineExternal.o vdr172remux.o vdr172h264parser.o

### The main target:

all: libvdr-$(PLUGIN).so i18n xineplayer

### Implicit rules:

%.o: %.c Makefile
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) xineplayer.c > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='Reinhard Nissl <rnissl@gmx.de>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs) $(I18Npot)

### Targets:

libvdr-$(PLUGIN).so: $(OBJS) Makefile
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

xineplayer: xineplayer.o Makefile
	$(CXX) $(CXXFLAGS) $(LDFLAGS) xineplayer.o -o $@

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ $(PODIR)/*.mo $(PODIR)/*.pot xineplayer xineplayer.o
