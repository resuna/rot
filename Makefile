# Fiddle with it, maybe use -ltermlib if you actually have that (and remove -DNOTERMINFO)
# Remove -DBIGMEM on a PDP-11

rot: rot.c Makefile
	cc rot.c -o rot -lcurses -DNO_TERMINFO -DBIGMEM
