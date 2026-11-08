INCS = -I /usr/X11/include
LIBS = -L /usr/X11/lib -lX11

all: simple

simple: simple.c
	gcc -o $@ $^ $(INCS) $(LIBS)

clean:
	rm -f simple
