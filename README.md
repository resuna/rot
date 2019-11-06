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

