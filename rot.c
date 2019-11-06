#include <stdio.h>
#include <ctype.h>
#include <term.h>
#include <string.h>
#include <stdlib.h>

/* cc rot.c -O -o rot -ltermlib */

/*              -- Miscellaneous defines --                                  */
#define FALSE 0
#define TRUE 1
#define MAXCOL 80
#define MAXLI 24

#define USLEEP 50000L
#if USLEEP > 0
# include <unistd.h>
#endif

#define SLEEP 0
#define INSOMNIA 5

void drop();
void fdropf();
void outs();
int min();


int lastx, lasty;
struct _c {
        struct _c *c_next;
        int c_line, c_column;
        char c_mark;
} *clist;

/*              -- Global variables --                                       */
char *tent;                                               /* Pointer to tbuf */
char PC;                                                    /* Pad character */
char *UP, *BC;                                /* Upline, backsapce character */
short ospeed;                                       /* Terminal output speed */
int tglen;

char *cm,                                                   /* Cursor motion */
     *cl,                                                    /* Clear vuscreen */
     *ti,                                                   /* Init terminal */
     *te;                                                  /* Reset terminal */
int  li,                                                  /* lines on vuscreen */
     co;                                                    /* columns ditto */
char vuscreen[MAXLI+1][MAXCOL];
char nuscreen[MAXLI+1][MAXCOL];

int main(ac, av)
int ac;
char **av;
{
        srand(getpid());
        tinit(getenv("TERM"));
        if(av[1])
                while(*++av)
                        dropf(*av);
        else
                fdropf(stdin);
        tend();
	return 0;
}

at(x, y, c)
int x, y;
char c;
{
#ifdef DEBUG
        _at(x, y);
#else
        if(y==lasty) {
                if(x!=lastx) {
                        if(x<lastx && lastx-x<tglen)
                                while(x<lastx) {
                                        putchar('\b');
                                        lastx--;
                                }
                        else if(x>lastx && x-lastx<tglen)
                                while(x>lastx) {
                                        putchar(nuscreen[lasty][lastx]);
                                        lastx++;
                                }
                        else
                                _at(x, y);
                }
        } else
                _at(x, y);
#endif
        c &= ~0200;
        putchar(c);
        if(c >= ' ' && c != '\177')
                lastx++;
        if(lastx>=co) {
                lastx -= co;
                lasty++;
        }
}

_at(x, y)
int x, y;
{
        outs(tgoto(cm, x, y));
        lastx = x;
        lasty = y;
}

void fixpad(s)
char *s;
{
	char *t = s;
	if(!s) return;

	while(*s) {
		if(s[0] == '$' && s[1] == '<') {
			while(*s && *s != '>')
				s++;
			if(*s)
				s++;
		} else {
			*t++ = *s++;
		}
	}
	*t = 0;
}

tinit(name)
char *name;
{
        static char junkbuf[1024], *junkptr;
        char tbuf[1024];
	char *pc;
        int  intr();

        junkptr = junkbuf;

        tgetent(tbuf, name);

        pc = tgetstr("pc", &junkptr); if (pc) PC=*pc;
        UP = tgetstr("up", &junkptr); fixpad(UP);
        BC = tgetstr("bc", &junkptr); fixpad(BC);
        cm = tgetstr("cm", &junkptr); while(isdigit(*cm)) cm++; fixpad(cm);
        cl = tgetstr("cl", &junkptr); fixpad(cl);
        ti = tgetstr("ti", &junkptr); fixpad(ti);
        te = tgetstr("te", &junkptr); fixpad(te);
        li = min(tgetnum("li"), MAXLI);
        co = min(tgetnum("co"), MAXCOL);
        tglen = strlen(tgoto(cm, co-1, li-1));
}

tend()
{
        outs(te);
        _at(0, li-1);
        putchar('\n');
        fflush(stdout);
}

void readscreen(fp)
FILE *fp;
{
        int line, column, p;
        char tmp[256];

        for(line=0; line<li; line++)
                for(column=0; column<co; column++)
                        nuscreen[line][column] = vuscreen[line][column] = ' ';
        for(column=0; column<co; column++)
                nuscreen[li][column] = vuscreen[li][column] = '*';
        line=0;
        while(line<li) {
                if(!fgets(tmp, 256, fp))
                        return;

                for(column=0, p=0; tmp[p]; p++) {
                        tmp[p] &= ~0200;
                        if(tmp[p] < ' ' || tmp[p] == 127)
                                switch(tmp[p]) {
                                        case '\t':
                                                while(++column % 8)
                                                        continue;
                                                break;
                                        case '\n':
                                                column = 0;
                                                line++;
                                                break;
                                        default:
                                                nuscreen[line][column] = '^';
                                                column++;
                                                if(column>=co) {
                                                        column -= co;
                                                        line++;
                                                }
                                                nuscreen[line][column] =
                                                        (tmp[p]+'@') & 127;
                                                column++;
                                                break;
                                }
                        else {
                                nuscreen[line][column] = tmp[p];
                                column++;
                        }
                        if(column >= co) {
                                column -= co;
                                line++;
                        }
                        if(line >= li)
                                break;
                }
        }
        for(column=0; column<co; column++)
                nuscreen[line][column] = vuscreen[li][column] = '*';
}

drawscreen()
{
        lastx = lasty = 0;
        outs(cl);
        update();
}

update() /* copy new vuscreen back to old vuscreen */
{
        int l, c;

        for(l=0; l<li; l++)
                for(c=0; c<co; c++)
                        if(vuscreen[l][c] != nuscreen[l][c]) {
                                if((vuscreen[l][c] & ~0200) !=
                                   (nuscreen[l][c] & ~0200))
                                        at(c, l, nuscreen[l][c]);
                                vuscreen[l][c] = nuscreen[l][c];
                        }
}

void drop(line, column)
int line, column;
{
        struct _c *hold;

        if(line<0 || line>=li || column<0 || column>=co ||
           (line>=li-2 && column >= co-1) || /* scroll potential */
           vuscreen[line][column]==' ' || /* empty */
           vuscreen[line][column] & 0200) /* already in list */
                return;
        if(vuscreen[line+1][column]!=' ' &&
           (column==co-1 ||vuscreen[line+1][column+1]!=' ') &&
           (column==0 ||vuscreen[line+1][column-1]!=' '))
                return;

        hold = (struct _c *) malloc(sizeof(struct _c));
        hold -> c_next = clist;
        hold -> c_column = column;
        hold -> c_line = line;
        hold -> c_mark = 0;
        vuscreen[line][column] |= 0200;
        clist = hold;
}

/* Go through the character list and drop each falling character 1 cell */
drops()
{
        int l, c;
        struct _c *hold;
        for(hold = clist; hold; hold=hold->c_next) {
                int line = hold->c_line, column=hold->c_column;
		/* Already hit bottom? */
                if(line>= li-2 && column>=co-1) {
                        nuscreen[line][column] &= ~0200;
                        vuscreen[line][column] &= ~0200;
                        hold->c_mark = 1;
                        continue;
                }
		/* Tag all neighbors */
                drop(line+1, column);
                drop(line, column+1);
                drop(line-1, column);
                drop(line, column-1);
		/* if the cell below is space, drop vertically */
                if(nuscreen[line+1][column]==' ') {
                        nuscreen[line+1][column] = vuscreen[line][column];
                        nuscreen[line][column] = ' ';
                        line++;
                } else if(rand()&01000) { /* half the time... */
			/* see if clear down and left, then down and right */
                        if(column>0 && nuscreen[line][column-1] == ' ' &&
                            nuscreen[line+1][column-1]==' ') {
                                nuscreen[line][column-1] =
                                        vuscreen[line][column];
                                nuscreen[line][column] = ' ';
                                column--;
                        }
                        else if(column<co-1 &&
                                nuscreen[line][column+1] == ' ' &&
                                nuscreen[line+1][column+1]==' ') {
                                        nuscreen[line][column+1] =
                                                vuscreen[line][column];
                                        nuscreen[line][column] = ' ';
                                        column++;
                        }
                        else { /* Nope, quit dropping this one */
                                vuscreen[line][column] &= ~0200;
                                nuscreen[line][column] &= ~0200;
                                hold -> c_mark = 1;
                        }
                } else {
			/* see if clear down and right, else down and left */
                        if(column<co-1 && nuscreen[line][column+1] == ' ' &&
                            nuscreen[line+1][column+1]==' ') {
                                nuscreen[line][column+1] =
                                        vuscreen[line][column];
                                nuscreen[line][column] = ' ';
                                column++;
                        }
                        else if(column>0 && nuscreen[line][column-1] == ' ' &&
                            nuscreen[line+1][column-1]==' ') {
                                nuscreen[line][column-1] =
                                        vuscreen[line][column];
                                nuscreen[line][column] = ' ';
                                column--;
                        }
                        else {
				/* can't drop, off list */
                                nuscreen[line][column] &= ~0200;
                                vuscreen[line][column] &= ~0200;
                                hold -> c_mark = 1;
                        }
                }
                hold -> c_column = column;
                hold -> c_line = line;
#if 0
                fflush(stdout);
#endif
        }

        while(clist && clist->c_mark) {
                struct _c *p = clist;
                clist = clist -> c_next;
                free(p);
        }
        hold = clist;
        while(hold && hold->c_next) {
                if(hold->c_next->c_mark) {
                        struct _c *p = hold->c_next;
                        hold->c_next = p->c_next;
                        free(p);
                } else hold=hold->c_next;
	}
}

droplet(line, column)
int line, column;
{
        int ret;
	int insomnia = INSOMNIA;

        while(column>=0 && vuscreen[line][column]!=' ')
                column--;
        column++;
        while(column<co && vuscreen[line][column]!=' ')
                drop(line, column++);
        ret = clist != 0;
        while(clist) {
                drops();
                update();
		fflush(stdout);
#if USLEEP > 0
		usleep(USLEEP);
#else
# if SLEEP > 0
		if (insomnia > 0)
			insomnia--;
		else {
			sleep(SLEEP);
			insomnia = INSOMNIA;
		}
# endif
#endif
        }
	return ret;
}

dropscreen()
{
        int column, line;
        int rubbish = 0, count = 0;

        do {
                int start, limit, incr;
                count++;
                rubbish = 0;
                if(count&1) { start=li-2; limit=0; incr = -1; }
                else { start=0; limit=li-2; incr=1; }
                for(line=start; line!=limit && !rubbish; line+=incr) {
                        if(line&1)
                                for(column=0; column<co && !rubbish; column++)
                                        rubbish += droplet(line, column);
                        else
                                for(column=co-1; column>=0 && !rubbish; column--)
                                        rubbish += droplet(line, column);
                }
        } while(rubbish);
}

dropf(file)
char *file;
{
        FILE *fp;

        if(!(fp = fopen(file, "r"))) {
                perror(file);
                return -1;
        } fdropf(fp);
}

void fdropf(fp)
FILE *fp;
{
        int i;

        while(!feof(fp)) {
                readscreen(fp);
                drawscreen();
                for(i=0; i<20; i++)
                        droplet((rand()>>4) % li, (rand()>>4) % co);
                dropscreen();
        }
}

void outs(s)
char *s;
{
        if (s) fputs(s, stdout);
}

min(a, b)
int a, b;
{
        if(a<b) return a;
        return b;
}
