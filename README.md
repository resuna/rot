# rot
rot - the world's worst pager (1980)

## Description

I wrote this around 1980, it's the oldest code I have that still exists and works.

It's the world's worst pager

## Building

### Original build instructions

cc rot.c -O -o rot -ltermlib

### Current build instructions

cc rot.c -O -o rot -lcurses

Because most systems don't have a separate TERMCAP library any more.

### Notes

This is still a Version 6 C program, and might even build on a PDP-11 in 1978 if you happen to have a
time machine. This means there's a boatload of errors due to the pre-ANSI function definitions. Deal with
them. I will accept pull requests for this but they'll be tossed into a seprate branch because this silly
thing is a historical artifact.

