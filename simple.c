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
		200, 200,
		1,
		BlackPixel(d,s),
		WhitePixel(d,s)
	);

	XSelectInput(d, w, ExposureMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(d, w);

	while (1) {
		XNextEvent(d, &e);
		if (e.type == Expose) {
			XFillRectangle(d, w, DefaultGC(d,s), 20, 20, 10, 10);
			XDrawString(d, w, DefaultGC(d,s), 10, 50, msg, strlen(msg));
		} else if (e.type == KeyPress) {
			// When a user press a key, (KeyPress, KeyRelease) event pairs are generated.
			// Since this application is meant to examine the keycodes, we don't want
			// to clutter the terminal with redundant information.
			//
			// We can't use the KeyRelease events as I hoped. If we measured the time between
			// two adjacent KeyPress events we may detect continuos presses, but I suspect
			// this is a machine dependent property
			//
			// XKeyEvent has a `time` field to be used in such case
			XKeyEvent* kev = &(e.xkey);
			printf("KeyPress event of keycode %d\n", kev->keycode);

			#define PRINT_MASK_COMPONENT(mask) printf("%12s: %d\n", #mask, (kev->state & mask) != 0)
			PRINT_MASK_COMPONENT(ShiftMask);
			PRINT_MASK_COMPONENT(LockMask);
			PRINT_MASK_COMPONENT(ControlMask);
			PRINT_MASK_COMPONENT(Mod1Mask);
			PRINT_MASK_COMPONENT(Mod2Mask);
			PRINT_MASK_COMPONENT(Mod3Mask);
			PRINT_MASK_COMPONENT(Mod4Mask);
			PRINT_MASK_COMPONENT(Mod5Mask);
			printf("\n");

			// I check only for the press of the `q` key and not for state modifiers
			// because on some machines a modmask key may be always pressed
			if (kev->keycode == 24) {
				break;
			}
		} else if (e.type == KeyRelease) {
		}
	}

	return 0;
}

