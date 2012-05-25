all:
	c++ -g -I/usr/include/cppconn main.c++ GtkCallbackStubs.c++ WindowRegistry.c++ -o main -ljpeg -ljson $(shell pkg-config --cflags --libs gtk+-2.0 gmodule-2.0) -lboost_program_options -lboost_regex -lmysqlcppconn -lexiv2 -lcrypto++ -lvte

clean:
	rm -f main
