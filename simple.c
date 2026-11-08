#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
	Display *d;
	Window w;
	XEvent e;
	const char *msg = "Press any key, `q` to quit";
	int s;

	d = XOpenDisplay(NULL);
	if (d == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	s = DefaultScreen(d);
	w = XCreateSimpleWindow(
		d, RootWindow(d,s),
		10, 10,
		100, 100,
		1,
		BlackPixel(d,s),
		WhitePixel(d,s)
	);

	XSelectInput(d, w, ExposureMask | KeyPressMask);
	XMapWindow(d, w);

	while (1) {
		XNextEvent(d, &e);
		if (e.type == Expose) {
			XFillRectangle(d, w, DefaultGC(d,s), 20, 20, 10, 10);
			XDrawString(d, w, DefaultGC(d,s), 10, 50, msg, strlen(msg));
		} else if (e.type == KeyPress) {
			XKeyEvent* kev = &(e.xkey);
			printf("keypress of state %d\n", kev->state);
		}
	}

	return 0;
}

