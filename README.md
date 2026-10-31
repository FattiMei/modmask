# modmask
An exercise in minimally AI-assisted software development.

## Specification
Starting from the [st](suckless.org/st) terminal emulator, modify the source code to produce a program that listens just for keyboard events.
The program must detect key presses and print which modifiers keys have been pressed (Shift, Caps Lock, Alt, Windows key).

## Motivation
The dwm window manager makes heavy use of the ALT key (referenced in the source code as `Mod1Mask`).
Since also emacs competes for that key, I wanted to change it to something else, but I don't know the exact keycodes.
