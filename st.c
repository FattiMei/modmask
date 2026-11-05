/* See LICENSE for license details. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "st.h"
#include "win.h"

#if   defined(__linux)
 #include <pty.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
 #include <util.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
 #include <libutil.h>
#endif

/* Arbitrary sizes */
#define UTF_INVALID   0xFFFD
#define UTF_SIZ       4
#define ESC_BUF_SIZ   (128*UTF_SIZ)
#define ESC_ARG_SIZ   16
#define STR_BUF_SIZ   ESC_BUF_SIZ
#define STR_ARG_SIZ   ESC_ARG_SIZ

/* macros */
#define IS_SET(flag)		((term.mode & (flag)) != 0)
#define ISCONTROLC0(c)		(BETWEEN(c, 0, 0x1f) || (c) == 0x7f)
#define ISCONTROLC1(c)		(BETWEEN(c, 0x80, 0x9f))
#define ISCONTROL(c)		(ISCONTROLC0(c) || ISCONTROLC1(c))
#define ISDELIM(u)		(u && wcschr(worddelimiters, u))

enum term_mode {
	MODE_WRAP        = 1 << 0,
	MODE_INSERT      = 1 << 1,
	MODE_ALTSCREEN   = 1 << 2,
	MODE_CRLF        = 1 << 3,
	MODE_ECHO        = 1 << 4,
	MODE_PRINT       = 1 << 5,
	MODE_UTF8        = 1 << 6,
};

enum cursor_movement {
	CURSOR_SAVE,
	CURSOR_LOAD
};

enum cursor_state {
	CURSOR_DEFAULT  = 0,
	CURSOR_WRAPNEXT = 1,
	CURSOR_ORIGIN   = 2
};

enum charset {
	CS_GRAPHIC0,
	CS_GRAPHIC1,
	CS_UK,
	CS_USA,
	CS_MULTI,
	CS_GER,
	CS_FIN
};

enum escape_state {
	ESC_START      = 1,
	ESC_CSI        = 2,
	ESC_STR        = 4,  /* DCS, OSC, PM, APC */
	ESC_ALTCHARSET = 8,
	ESC_STR_END    = 16, /* a final string was encountered */
	ESC_TEST       = 32, /* Enter in test mode */
	ESC_UTF8       = 64,
};

typedef struct {
	Glyph attr; /* current char attributes */
	int x;
	int y;
	char state;
} TCursor;

typedef struct {
	int mode;
	int type;
	int snap;
	/*
	 * Selection variables:
	 * nb – normalized coordinates of the beginning of the selection
	 * ne – normalized coordinates of the end of the selection
	 * ob – original coordinates of the beginning of the selection
	 * oe – original coordinates of the end of the selection
	 */
	struct {
		int x, y;
	} nb, ne, ob, oe;

	int alt;
} Selection;

/* Internal representation of the screen */
typedef struct {
	int row;      /* nb row */
	int col;      /* nb col */
	Line *line;   /* screen */
	Line *alt;    /* alternate screen */
	int *dirty;   /* dirtyness of lines */
	TCursor c;    /* cursor */
	int ocx;      /* old cursor col */
	int ocy;      /* old cursor row */
	int top;      /* top    scroll limit */
	int bot;      /* bottom scroll limit */
	int mode;     /* terminal mode flags */
	int esc;      /* escape state flags */
	char trantbl[4]; /* charset table translation */
	int charset;  /* current charset */
	int icharset; /* selected charset for sequence */
	int *tabs;
	Rune lastc;   /* last printed char outside of sequence, 0 if control */
} Term;

/* CSI Escape sequence structs */
/* ESC '[' [[ [<priv>] <arg> [;]] <mode> [<mode>]] */
typedef struct {
	char buf[ESC_BUF_SIZ]; /* raw string */
	size_t len;            /* raw string length */
	char priv;
	int arg[ESC_ARG_SIZ];
	int narg;              /* nb of args */
	char mode[2];
} CSIEscape;

/* STR Escape sequence structs */
/* ESC type [[ [<priv>] <arg> [;]] <mode>] ESC '\' */
typedef struct {
	char type;             /* ESC type ... */
	char *buf;             /* allocated raw string */
	size_t siz;            /* allocation size */
	size_t len;            /* raw string length */
	char *args[STR_ARG_SIZ];
	int narg;              /* nb of args */
} STREscape;

static void execsh(char *, char **);
static void stty(char **);
static void sigchld(int);
static void ttywriteraw(const char *, size_t);

static void csidump(void);
static void csihandle(void);
static void csiparse(void);
static void csireset(void);
static void osc_color_response(int, int, int);
static int eschandle(uchar);
static void strdump(void);
static void strhandle(void);
static void strparse(void);
static void strreset(void);

static void tprinter(char *, size_t);
static void tdumpsel(void);
static void tdumpline(int);
static void tdump(void);
static void tclearregion(int, int, int, int);
static void tcursor(int);
static void tdeletechar(int);
static void tdeleteline(int);
static void tinsertblank(int);
static void tinsertblankline(int);
static int tlinelen(int);
static void tmoveto(int, int);
static void tmoveato(int, int);
static void tnewline(int);
static void tputtab(int);
static void tputc(Rune);
static void treset(void);
static void tscrollup(int, int);
static void tscrolldown(int, int);
static void tsetattr(const int *, int);
static void tsetchar(Rune, const Glyph *, int, int);
static void tsetdirt(int, int);
static void tsetscroll(int, int);
static void tswapscreen(void);
static void tsetmode(int, int, const int *, int);
static int twrite(const char *, int, int);
static void tfulldirt(void);
static void tcontrolcode(uchar );
static void tdectest(char );
static void tdefutf8(char);
static int32_t tdefcolor(const int *, int *, int);
static void tdeftran(char);
static void tstrsequence(uchar);

static void drawregion(int, int, int, int);

static void selnormalize(void);
static void selscroll(int, int);
static void selsnap(int *, int *, int);

static size_t utf8decode(const char *, Rune *, size_t);
static Rune utf8decodebyte(char, size_t *);
static char utf8encodebyte(Rune, size_t);
static size_t utf8validate(Rune *, size_t);

static char *base64dec(const char *);
static char base64dec_getc(const char **);

static ssize_t xwrite(int, const char *, size_t);

/* Globals */
static Term term;
static Selection sel;
static CSIEscape csiescseq;
static STREscape strescseq;
static int iofd = 1;
static int cmdfd;
static pid_t pid;

static const uchar utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const uchar utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const Rune utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const Rune utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};



void *
xmalloc(size_t len)
{
	void *p;

	if (!(p = malloc(len)))
		die("malloc: %s\n", strerror(errno));

	return p;
}

void *
xrealloc(void *p, size_t len)
{
	if ((p = realloc(p, len)) == NULL)
		die("realloc: %s\n", strerror(errno));

	return p;
}

















void
selinit(void)
{
	sel.mode = SEL_IDLE;
	sel.snap = 0;
	sel.ob.x = -1;
}





int
selected(int x, int y)
{
	if (sel.mode == SEL_EMPTY || sel.ob.x == -1 ||
			sel.alt != IS_SET(MODE_ALTSCREEN))
		return 0;

	if (sel.type == SEL_RECTANGULAR)
		return BETWEEN(y, sel.nb.y, sel.ne.y)
		    && BETWEEN(x, sel.nb.x, sel.ne.x);

	return BETWEEN(y, sel.nb.y, sel.ne.y)
	    && (y != sel.nb.y || x >= sel.nb.x)
	    && (y != sel.ne.y || x <= sel.ne.x);
}





void
selclear(void)
{
	if (sel.ob.x == -1)
		return;
	sel.mode = SEL_IDLE;
	sel.ob.x = -1;
	tsetdirt(sel.nb.y, sel.ne.y);
}

void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

void
sigchld(int a)
{
	int stat;
	pid_t p;

	if ((p = waitpid(pid, &stat, WNOHANG)) < 0)
		die("waiting for pid %hd failed: %s\n", pid, strerror(errno));

	if (pid != p)
		return;

	if (WIFEXITED(stat) && WEXITSTATUS(stat))
		die("child exited with status %d\n", WEXITSTATUS(stat));
	else if (WIFSIGNALED(stat))
		die("child terminated due to signal %d\n", WTERMSIG(stat));
	_exit(0);
}












void
tsetdirt(int top, int bot)
{
	int i;

	if (term.row <= 0)
		return;

	LIMIT(top, 0, term.row-1);
	LIMIT(bot, 0, term.row-1);

	for (i = top; i <= bot; i++)
		term.dirty[i] = 1;
}

void
tfulldirt(void)
{
	tsetdirt(0, term.row-1);
}

void
tcursor(int mode)
{
	static TCursor c[2];
	int alt = IS_SET(MODE_ALTSCREEN);

	if (mode == CURSOR_SAVE) {
		c[alt] = term.c;
	} else if (mode == CURSOR_LOAD) {
		term.c = c[alt];
		tmoveto(c[alt].x, c[alt].y);
	}
}

void
treset(void)
{
	uint i;

	term.c = (TCursor){{
		.mode = ATTR_NULL,
		.fg = defaultfg,
		.bg = defaultbg
	}, .x = 0, .y = 0, .state = CURSOR_DEFAULT};

	memset(term.tabs, 0, term.col * sizeof(*term.tabs));
	for (i = tabspaces; i < term.col; i += tabspaces)
		term.tabs[i] = 1;
	term.top = 0;
	term.bot = term.row - 1;
	term.mode = MODE_WRAP|MODE_UTF8;
	memset(term.trantbl, CS_USA, sizeof(term.trantbl));
	term.charset = 0;

	for (i = 0; i < 2; i++) {
		tmoveto(0, 0);
		tcursor(CURSOR_SAVE);
		tclearregion(0, 0, term.col-1, term.row-1);
		tswapscreen();
	}
}

void
tnew(int col, int row)
{
	term = (Term){ .c = { .attr = { .fg = defaultfg, .bg = defaultbg } } };
	tresize(col, row);
	treset();
}

void
tswapscreen(void)
{
	Line *tmp = term.line;

	term.line = term.alt;
	term.alt = tmp;
	term.mode ^= MODE_ALTSCREEN;
	tfulldirt();
}











/* for absolute user moves, when decom is set */


void
tmoveto(int x, int y)
{
	int miny, maxy;

	if (term.c.state & CURSOR_ORIGIN) {
		miny = term.top;
		maxy = term.bot;
	} else {
		miny = 0;
		maxy = term.row - 1;
	}
	term.c.state &= ~CURSOR_WRAPNEXT;
	term.c.x = LIMIT(x, 0, term.col-1);
	term.c.y = LIMIT(y, miny, maxy);
}



void
tclearregion(int x1, int y1, int x2, int y2)
{
	int x, y, temp;
	Glyph *gp;

	if (x1 > x2)
		temp = x1, x1 = x2, x2 = temp;
	if (y1 > y2)
		temp = y1, y1 = y2, y2 = temp;

	LIMIT(x1, 0, term.col-1);
	LIMIT(x2, 0, term.col-1);
	LIMIT(y1, 0, term.row-1);
	LIMIT(y2, 0, term.row-1);

	for (y = y1; y <= y2; y++) {
		term.dirty[y] = 1;
		for (x = x1; x <= x2; x++) {
			gp = &term.line[y][x];
			if (selected(x, y))
				selclear();
			gp->fg = term.c.attr.fg;
			gp->bg = term.c.attr.bg;
			gp->mode = 0;
			gp->u = ' ';
		}
	}
}













void
tsetscroll(int t, int b)
{
	int temp;

	LIMIT(t, 0, term.row-1);
	LIMIT(b, 0, term.row-1);
	if (t > b) {
		temp = t;
		t = b;
		b = temp;
	}
	term.top = t;
	term.bot = b;
}















































/*
 * returns 1 when the sequence is finished and it hasn't to read
 * more characters for this sequence, otherwise 0
 */






void
tresize(int col, int row)
{
	int i;
	int minrow = MIN(row, term.row);
	int mincol = MIN(col, term.col);
	int *bp;
	TCursor c;

	if (col < 1 || row < 1) {
		fprintf(stderr,
		        "tresize: error resizing to %dx%d\n", col, row);
		return;
	}

	/*
	 * slide screen to keep cursor where we expect it -
	 * tscrollup would work here, but we can optimize to
	 * memmove because we're freeing the earlier lines
	 */
	for (i = 0; i <= term.c.y - row; i++) {
		free(term.line[i]);
		free(term.alt[i]);
	}
	/* ensure that both src and dst are not NULL */
	if (i > 0) {
		memmove(term.line, term.line + i, row * sizeof(Line));
		memmove(term.alt, term.alt + i, row * sizeof(Line));
	}
	for (i += row; i < term.row; i++) {
		free(term.line[i]);
		free(term.alt[i]);
	}

	/* resize to new height */
	term.line = xrealloc(term.line, row * sizeof(Line));
	term.alt  = xrealloc(term.alt,  row * sizeof(Line));
	term.dirty = xrealloc(term.dirty, row * sizeof(*term.dirty));
	term.tabs = xrealloc(term.tabs, col * sizeof(*term.tabs));

	/* resize each row to new width, zero-pad if needed */
	for (i = 0; i < minrow; i++) {
		term.line[i] = xrealloc(term.line[i], col * sizeof(Glyph));
		term.alt[i]  = xrealloc(term.alt[i],  col * sizeof(Glyph));
	}

	/* allocate any new rows */
	for (/* i = minrow */; i < row; i++) {
		term.line[i] = xmalloc(col * sizeof(Glyph));
		term.alt[i] = xmalloc(col * sizeof(Glyph));
	}
	if (col > term.col) {
		bp = term.tabs + term.col;

		memset(bp, 0, sizeof(*term.tabs) * (col - term.col));
		while (--bp > term.tabs && !*bp)
			/* nothing */ ;
		for (bp += tabspaces; bp < term.tabs + col; bp += tabspaces)
			*bp = 1;
	}
	/* update terminal size */
	term.col = col;
	term.row = row;
	/* reset scrolling region */
	tsetscroll(0, row-1);
	/* make use of the LIMIT in tmoveto */
	tmoveto(term.c.x, term.c.y);
	/* Clearing both screens (it makes dirty all lines) */
	c = term.c;
	for (i = 0; i < 2; i++) {
		if (mincol < col && 0 < minrow) {
			tclearregion(mincol, 0, col - 1, minrow - 1);
		}
		if (0 < col && minrow < row) {
			tclearregion(0, minrow, col - 1, row - 1);
		}
		tswapscreen();
		tcursor(CURSOR_LOAD);
	}
	term.c = c;
}

void
resettitle(void)
{
	xsettitle(NULL);
}






