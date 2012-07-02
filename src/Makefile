CSRC =
COBJ = $(CSRC:%.c=%.o)
CDEP = $(COBJ:%.c:%.d)

CXXSRC = main.cc GtkCallbackStubs.cc WindowRegistry.cc
CXXOBJ = $(CXXSRC:%.cc=%.o)
CXXDEP = $(CXXSRC:%.cc=%.d)

CXXFLAGS = -g -I/usr/include/cppconn $(shell pkg-config --cflags gtk+-3.0 gmodule-2.0 json exiv2 libcrypto++ xerces-c)

CPPFLAGS += -MD -MP

LDFLAGS = -ljpeg -lboost_program_options -lboost_regex -lmysqlcppconn -ljson_spirit $(shell pkg-config --libs gtk+-3.0 gmodule-2.0 json exiv2 libcrypto++ xerces-c)

main:	$(COBJ) $(CXXOBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	-rm -f $(COBJ) $(CDEP) $(CXXOBJ) $(CXXDEP) main

show:
	@/bin/echo CSRC is $(CSRC)
	@/bin/echo COBJ is $(COBJ)
	@/bin/echo CDEP is $(CDEP)
	@/bin/echo CXXSRC is $(CXXSRC)
	@/bin/echo CXXOBJ is $(CXXOBJ)
	@/bin/echo CXXDEP is $(CXXDEP)
	@/bin/echo CXXFLAGS is $(CXXFLAGS)
	@/bin/echo CPPFLAGS is $(CPPFLAGS)
	@/bin/echo LDFLAGS is $(LDFLAGS)

-include $(CDEP) $(CXXDEP)
