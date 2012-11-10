#
# Makefile to test that each .h file in a directory can be #included by itself. I.e. that it
# is not dependent on another .h file being #included ahead of it
#

# set hpath to the directory with .h files to test
hpath := ../src

hfiles := $(notdir $(wildcard $(hpath)/*.h))
basehfiles := $(basename $(hfiles))
targetccfiles:= $(addsuffix .test.cc, $(basehfiles))
targets:= $(addsuffix .test, $(basehfiles))

.INTERMEDIATE:	$(targets)

CFLAGS += -I/opt/local/include/cppconn \
          -I/usr/include/atk-1.0 \
          -I/usr/include/cairo \
          -I/usr/include/cppconn \
          -I/usr/include/freetype2 \
          -I/usr/include/gdk-pixbuf-2.0 \
          -I/usr/include/gio-unix-2.0 \
          -I/usr/include/glib-2.0 \
          -I/usr/include/gtk-3.0 \
          -I/usr/include/json \
          -I/usr/include/libpng12 \
          -I/usr/include/pango-1.0 \
          -I/usr/include/pixman-1 \
          -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
CFLAGS += -std=gnu++0x
LDFLAGS += -lxerces-c

# Rule to convert a .h file in hpath to a test.cc file
%.test.cc : $(hpath)/%.h
	echo '#include "'$<'"' > $@
	echo 'main() {printf("Running '$<'\\n");}' >> $@

#rule to compile a test.cc file
%: %.cc
	c++ $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

all:	$(targets)
	for i in $(targets) ; do ./$$i ; done

clean:
	@rm -f $(targetccfiles)
	@rm -f $(targets)
