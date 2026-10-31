# modmask
An exercise in minimally AI-assisted software development.

## Specification
Starting from the [st](suckless.org/st) terminal emulator, modify the source code to produce a program that listens just for keyboard events.
The program must detect key presses and print which modifiers keys have been pressed (Shift, Caps Lock, Alt, Windows key).

## Motivation
The dwm window manager makes heavy use of the ALT key (referenced in the source code as `Mod1Mask`).
Since also emacs competes for that key, I wanted to change it to something else, but I don't know the exact keycodes.


# Log
In the `x.c` source:
  * `xinit()` creates a display, window and manages the fonts
  * `run()` contains the event loop. `handler` maps event types to functions

### XIM (X Input Manager?)
In the initialization function, `ximopen(Display*)` is called.
It opens an input manager with `XOpenIM(Display*,...)` with the comment "input methods".
Removing this function doesn't alter the terminal functionality.

### Event handling
```c
while (XPending(xw.dpy)) {
      XNextEvent(xw.dpy, &ev);
      
      if (XFilterEvent(&ev, None)) continue;
      
      if (handler[ev.type]) (handler[ev.type])(&ev);
}
```

Here is a list of events supported by `st`:
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

I don't know what `XFilterEvent` does.

