# Makefile for OIFITSlib library and command-line utilities
#
# This file is part of OIFITSlib.
#
# OIFITSlib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# OIFITSlib is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with OIFITSlib.  If not, see
# http://www.gnu.org/licenses/

.PHONY: default install clean

CC = gcc
#CC = cc
AR = ar
# Note that 'demo' target can be built even if pkg-config not present
PKGCONFIG = pkg-config
TOUCH = touch
INSTALL = install
PYTHON = python2.7
PYINCFLAGS = `$(PYTHON)-config --cflags`

CPPFLAGS = `$(PKGCONFIG) --cflags glib-2.0`
CFLAGS =  -Wall -g -fPIC -std=c99
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  # Add MacPorts directories to search paths. Libraries installed by Homebrew
  # are accessed from /usr/local which is in gcc's default search paths, so
  # if you have a required library installed in both systems, the Homebrew
  # one will take precedence.
  CPPFLAGS += -I/opt/local/include
  override LDLIBS += -L/opt/local/lib -lcfitsio -lm
  ifeq ($(CC),cc)
    SHAREDFLAGS = -bundle
  else
    SHAREDFLAGS = -shared
  endif
else ifeq ($(UNAME_S),SunOS)
  override LDLIBS += -lcfitsio -lm -lnsl -lsocket
  export LD_RUN_PATH:= /usr/local/lib:/star/lib
  # assume gcc
  SHAREDFLAGS = -shared
else
  override LDLIBS += -lcfitsio -lm
  SHAREDFLAGS = -shared
endif
LDLIBS_GLIB = `$(PKGCONFIG) --libs glib-2.0`

# Where to install things
prefix = /usr/local
includedir = $(prefix)/include
bindir = $(prefix)/bin
libdir = $(prefix)/lib


# Rules to make .o file from .c file
%_wrap.o : %_wrap.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PYINCFLAGS) -c $<

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

# Rule to make executable from .o file
# $^ picks up all dependencies
% : %.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# Rule to create empty target file indicating presence of a library
%.libexists:
	$(PKGCONFIG) --exists $*
	$(TOUCH) $@


OITABLE = liboitable.a demo
EXES = oifits-check oifits-merge oifits-filter oifits-upgrade
TEST_EXES = utest_datemjd utest_oifile \
 utest_oicheck utest_oimerge utest_oifilter
LIBRARIES = liboifits.a
INCFILES = chkmalloc.h datemjd.h \
 exchange.h oifile.h oicheck.h oifilter.h oimerge.h
PYTHONMODULES = _oifitsmodule.so \
 _oifiltermodule.so _oicheckmodule.so _oimergemodule.so

# List targets that don't require GLib first
default: $(OITABLE) $(LIBRARIES) $(EXES) $(TEST_EXES) $(PYTHONMODULES);

install: $(OITABLE) $(LIBRARIES) $(INCFILES) $(EXES)
	$(INSTALL) -m 755 $(EXES) $(bindir)
	$(INSTALL) -m 644 $(OITABLE) $(LIBRARIES) $(libdir)
	$(INSTALL) -m 644 $(INCFILES) $(includedir)

clean:
	rm -f $(OITABLE) $(LIBRARIES) $(EXES) $(TEST_EXES) *.o *.libexists
	rm -f $(PYTHONMODULES) *_wrap.c *.py *.pyc

# Library for table level I/O only - does not require GLib
liboitable.a: read_fits.o write_fits.o alloc_fits.o free_fits.o chkmalloc.o
	$(AR) -ruc $@ $^
write_fits.o: write_fits.c exchange.h chkmalloc.h
read_fits.o: read_fits.c exchange.h chkmalloc.h
alloc_fits.o: alloc_fits.c exchange.h chkmalloc.h
free_fits.o: free_fits.c exchange.h
chkmalloc.o: chkmalloc.c chkmalloc.h

demo: demo.o liboitable.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

demo.o: demo.c exchange.h

#
# Targets requiring GLib
#
liboifits.a: read_fits.o write_fits.o alloc_fits.o free_fits.o chkmalloc.o \
 datemjd.o oifile.o oifilter.o oicheck.o oimerge.o
	$(AR) -ruc $@ $^
datemjd.o: datemjd.c datemjd.h
oifile.o: oifile.c oifile.h exchange.h chkmalloc.h datemjd.h glib-2.0.libexists
oifilter.o: oifilter.c oifilter.h oifile.h exchange.h chkmalloc.h glib-2.0.libexists
oicheck.o: oicheck.c oicheck.h oifile.h exchange.h glib-2.0.libexists
oimerge.o: oimerge.c oimerge.h oifile.h exchange.h chkmalloc.h datemjd.h glib-2.0.libexists
utest_datemjd.o: utest_datemjd.c datemjd.h glib-2.0.libexists
utest_oifile.o: utest_oifile.c oifile.h exchange.h glib-2.0.libexists
utest_oicheck.o: utest_oicheck.c oicheck.h oifile.h exchange.h glib-2.0.libexists
utest_oimerge.o: utest_oimerge.c oimerge.h oifile.h oicheck.h exchange.h glib-2.0.libexists
utest_oifilter.o: utest_oifilter.c oifilter.h oifile.h oicheck.h exchange.h glib-2.0.libexists

oifits-filter: oifits-filter.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

oifits-check: oifits-check.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

oifits-merge: oifits-merge.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

oifits-upgrade: oifits-upgrade.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_datemjd: utest_datemjd.o datemjd.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oifile: utest_oifile.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oicheck: utest_oicheck.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oimerge: utest_oimerge.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oifilter: utest_oifilter.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

# Python interface
# :TODO: use distutils instead, perhaps for compile and link steps only
%_wrap.c %.py : %.i oifits_typemaps.i
	swig -python -o $@ $< 

_%module.so: %_wrap.o liboifits.a
	$(CC) $(SHAREDFLAGS) $^ `$(PYTHON)-config --ldflags` $(LDLIBS) $(LDLIBS_GLIB) -o $@

test: $(TEST_EXES) $(PYTHONMODULES)
	./utest_oifile
	./utest_oicheck	
	./utest_oimerge
	./utest_oifilter
	$(PYTHON) oifits.py
	$(PYTHON) oifilter.py
	$(PYTHON) oicheck.py
	$(PYTHON) oimerge.py
