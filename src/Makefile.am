bin_PROGRAMS = PhotoSelect
PhotoSelect_SOURCES = main.cc WidgetRegistry.cc
AM_CPPFLAGS = $(PhotoSelect_CFLAGS) $(LIBCRYPTO_CFLAGS) -I/usr/include/cppconn -I/opt/local/include/cppconn
PhotoSelect_LDADD = $(PhotoSelect_LIBS) -ljpeg -lboost_program_options -lboost_regex -lmysqlcppconn -ljson_spirit
