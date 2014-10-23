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

CC = gcc
AR = ar
# Note that 'demo' target can be built even if pkg-config not present
PKGCONFIG = pkg-config
TOUCH = touch
PYTHON = python2.7
PYINCFLAGS = -I/usr/include/$(PYTHON)

CPPFLAGS = `$(PKGCONFIG) --cflags glib-2.0`
CFLAGS =  -Wall -g -fPIC
ifeq ($(OSTYPE),solaris)
  override LDLIBS += -lcfitsio -lm -lnsl -lsocket
else
  override LDLIBS += -lcfitsio -lm
endif
LDLIBS_GLIB = `$(PKGCONFIG) --libs glib-2.0`

export LD_RUN_PATH:= /usr/local/lib:/star/lib


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
EXES = oifits-check oifits-merge oifits-filter 
TEST_EXES = utest_oicheck utest_oimerge utest_oifilter
LIBRARIES = liboifits.a
PYTHONMODULES = _oifitsmodule.so \
 _oifiltermodule.so _oicheckmodule.so _oimergemodule.so

# List targets that don't require GLib first
default: $(OITABLE) $(LIBRARIES) $(EXES) $(TEST_EXES) $(PYTHONMODULES);

clean:
	rm -f $(OITABLE) $(LIBRARIES) $(EXES) $(TEST_EXES) *.o *.libexists
	rm -f $(PYTHONMODULES) *_wrap.c *.py *.pyc

# Library for table level I/O only - does not require GLib
liboitable.a: read_fits.o write_fits.o free_fits.o
	$(AR) -ruc $@ $^
write_fits.o: write_fits.c exchange.h
read_fits.o: read_fits.c exchange.h
free_fits.o: free_fits.c exchange.h

demo: demo.o liboitable.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

demo.o: demo.c exchange.h

#
# Targets requiring GLib
#
liboifits.a: read_fits.o write_fits.o free_fits.o oifile.o \
 oifilter.o oicheck.o oimerge.o
	$(AR) -ruc $@ $^
oifile.o: oifile.c oifile.h exchange.h glib-2.0.libexists
oifilter.o: oifilter.c oifilter.h oifile.h exchange.h glib-2.0.libexists
oicheck.o: oicheck.c oicheck.h oifile.h exchange.h glib-2.0.libexists
oimerge.o: oimerge.c oimerge.h oifile.h exchange.h glib-2.0.libexists
utest_oicheck.o: utest_oicheck.c oicheck.h oifile.h exchange.h glib-2.0.libexists
utest_oimerge.o: utest_oimerge.c oimerge.h oifile.h oicheck.h exchange.h glib-2.0.libexists
utest_oifilter.o: utest_oifilter.c oifilter.h oifile.h oicheck.h exchange.h glib-2.0.libexists

oifits-filter: oifits-filter.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

oifits-check: oifits-check.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

oifits-merge: oifits-merge.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oicheck: utest_oicheck.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oimerge: utest_oimerge.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

utest_oifilter: utest_oifilter.o liboifits.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) $(LDLIBS_GLIB)

# Python interface
# :TODO: use distutils instead, perhaps for $(CC) & $(LD) steps only
%_wrap.c %.py : %.i oifits_typemaps.i
	swig -python -o $@ $< 

_%module.so: %_wrap.o liboifits.a
	$(LD) -G $^ $(LDLIBS) $(LDLIBS_GLIB) -o $@

test: $(TEST_EXES) $(PYTHONMODULES)
	./utest_oicheck	
	./utest_oimerge
	./utest_oifilter
	$(PYTHON) oifits.py
	$(PYTHON) oifilter.py
	$(PYTHON) oicheck.py
	$(PYTHON) oimerge.py
