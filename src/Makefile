all:
	c++ -g main.c++ PhotoSelectWindow.c++ WindowRegistry.c++ -o main -ljpeg -ljson $(shell pkg-config --cflags --libs gtk+-2.0 gmodule-2.0)

clean:
	rm -f main
