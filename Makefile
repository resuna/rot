# Fiddle with it, maybe use -ltermlib if you actually have that (and remove -DNOTERMINFO)

rot: rot.c Makefile
	cc rot.c -o rot -lcurses -DNO_TERMINFO
