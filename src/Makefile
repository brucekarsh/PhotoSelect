all:
	c++ $(CFLAGS) -g -I/usr/include/cppconn main.c++ GtkCallbackStubs.c++ WindowRegistry.c++ -o main -ljpeg -ljson $(shell pkg-config --cflags --libs gtk+-3.0 gmodule-2.0) -lboost_program_options -lboost_regex -lmysqlcppconn -lexiv2 -lcrypto++ -lvte -lxerces-c -ljson_spirit

clean:
	rm -f main
