# modmask
Build a minimal X11 application to display special keys (Ctrl, ALT, Shift) keycodes

## Motivation
In October 2026 I started exploring the emacs editor, which makes heavy use of the ALT key.
However, some emacs keybindings are overridden by my window manager ([dwm](dwm.suckless.org)) keybindings,
so my plan is to modify dwm to use another key, but which one?

This application should help me map keys to numerical codes.
I decided to experiment with the X11 library by modifying [st](st.suckless.org), an X11 terminal emulator of the suckless philosophy.
The plan is to gradually **remove the terminal functionality** from the application while maintaining a *green state* (no compile or runtime errors)

## Stage 1 - codebase exploration
In this stage I started exmining the codebase and found two relevant functions:
  * `xinit(rows, cols)`: initializes the X system, creates a window and loads fonts and colormaps
  * `run()`: contains the event loop

the event loop has this structure:
```c
while (XPending(xw.dpy)) {
    XNextEvent(xw.dpy, &ev);

    if (XFilterEvent(&ev, None)) {
        continue;
    }

    if (handler[ev.type]) {
        (handler[ev.type])(&ev);
    }
```

with `header` being a map from event type to function pointer.
These are the events supported by the st terminal:
  * **KeyPress**: keyboard presses
  * **ClientMessage**: window close is a client message
  * **ConfigureNotify**: window resizing
  * **VisibilityNotify**
  * **UnmapNotify**
  * **Expose**
  * **FocusIn**
  * **FocusOut**
  * **MotionNotify**
  * **ButtonPress**: mouse related
  * **ButtonRelease**
  * **SelectionNotify**: clipboard related
  * **PropertyNotify**
  * **SelectionRequest**


## Stage 2 - feature removal
I wrote my own event loop and deleted the `run()` function.
This prompted me to delete the functions called by `run` and never called anywhere else.
But could I do it in an automatic way?

I propose a program that takes the LLVM intermediate representation of the sources and runs a graph reachability algorithm.
Here is a simplified definition of a *reachable* function:
> a function is reachable if it's executed in some execution path starting from main or if it's been registered as a callback

the last clause is important in X11 codebases, but it's inexact: a function may be registered as callback by an unreachable function.
In the interest of simplicity I manually handled those cases.

As a bonus, the tool outputs a format parsable by editorrs like vim that points to the source location of the function implementation.
```
x.c:100:0: `foo` is unreachable
st.c:200:0: `bar` is unreachable
```

Had a very pleasant experience with this tool (which is integrated in the build system)


## Stage 3 - giving up
I manage to delete more than 3000 LOC from the st codebase (thanks to vim macros).
In principle I could roll from that (`f66ef4f`), but there was still too much code, and I was grasping none of it.
Fortunately I found a [minimal X11 example](rosettacode.org/wiki/Window_creation/X11#C)

```c
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    Display *d;
    Window w;
    XEvent e;
    const char *msg = "Hello, World!";

    d = XOpenDisplay(NULL);
    if (d == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d,s), 10, 10, 100, 100, 1,
                            BlackPixel(d,s), WhitePixel(d,s));

    XSelectInput(d, w, ExposureMask | KeyPressMask);
    XMapWindow(d, w);

    while (1) {
        XNextEvent(d, &e);
        if (e.type == Expose) {
            XFillRectangle(d, w, DefaultGC(d,s), 20, 20, 10, 10);
            XDrawString(d, w, DefaultGC(d,s), 10, 50, msg, strlen(msg));
        }
        if (e.type == KeyPress) {
            break;
        }
    }

    return 0;
}

this is a better starting point for an X11 application :)

