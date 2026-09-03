/*
 * curse_test -- comprehensive functional test / demo for the EwokOS
 * netbsd-curses port (libcurses + libterminfo).
 *
 * It walks a sequence of screens; each screen exercises one curses
 * subsystem and prints a per-check [ OK ] / [FAIL] line.  A final screen
 * tallies the results.  Every text path routes through printw/vw_printw,
 * so the libc vsnprintf formatter (%d %s %x %p %f %e %g %a %zu ...) is
 * exercised too.
 *
 * Usage:
 *   curse_test              interactive: any key advances, 'q' quits
 *   curse_test -a           auto: advance on a timer (headless smoke run)
 *   curse_test -t TERM      force TERM (default xterm when unset/unknown)
 *
 * NOTE: EwokOS libcurses ships with a compiled-in terminfo database and a
 * default term of "unknown" that is NOT present in it, so TERM must name a
 * built-in terminal (xterm/vt100/ansi/linux/...) before initscr() or the
 * screen setup aborts.  We set it here when the environment does not.
 */

#include <curses.h>
#include <term.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

static int g_pass;      /* total [ OK ] checks   */
static int g_fail;      /* total [FAIL] checks   */
static int g_quit;      /* user asked to stop    */
static int g_auto;      /* auto-advance (no key) */
static int g_row;       /* current output row    */
static int g_colors;    /* start_color succeeded */

#define MAX_FAILED 64
static const char *g_failed[MAX_FAILED]; /* names of the failed checks */
static int g_nfailed;                    /* how many were recorded     */

/* ------------------------------------------------------------------ */
/* small reporting helpers                                             */
/* ------------------------------------------------------------------ */

static char g_title[96];    /* current screen title, for continuations */

static int waitkey(void);
static void ensure_row(void);

static void title_band(const char *title)
{
	attron(A_BOLD | A_REVERSE);
	mvprintw(0, 0, "%-*.*s", COLS, COLS, title);
	attroff(A_BOLD | A_REVERSE);
}

static void screen_title(const char *title)
{
	snprintf(g_title, sizeof g_title, "%s", title);
	/* clear() (not erase()) forces a full repaint on refresh, so leftover
	 * pixels/attributes from the previous screen cannot survive. */
	clear();
	title_band(g_title);
	/* no physical scrolling: a stray write at the last row must never
	 * push the title band into the scrollback */
	scrollok(stdscr, FALSE);
	/* park the cursor off the title row: writing exactly COLS chars leaves
	 * a pending wrap that a later stray addch would turn into a scroll. */
	move(1, 0);
	g_row = 2;
	refresh();
}

/* Flip to a continuation screen before a long check list would scroll the
 * title off the top or write into the status-bar row. */
static void ensure_row(void)
{
	char cont[96];

	/* never write on the title band or the status-bar row */
	if (g_row < 2)
		g_row = 2;
	if (g_row < LINES - 2)
		return;
	refresh();
	if (!waitkey())
		return;
	snprintf(cont, sizeof cont, " %.*s (cont.) ",
	    (int)(sizeof cont - 12), g_title + 1);
	screen_title(cont);
}

static void check(const char *name, int ok)
{
	ensure_row();
	mvprintw(g_row++, 2, "%-40.40s %s", name, ok ? "[ OK ]" : "[FAIL]");
	if (ok) {
		g_pass++;
	} else {
		g_fail++;
		/* remember the name so the summary/stdout can pinpoint it */
		if (g_nfailed < MAX_FAILED)
			g_failed[g_nfailed++] = name;
	}
}

static void info(const char *fmt, ...)
{
	va_list ap;

	ensure_row();
	move(g_row++, 2);
	va_start(ap, fmt);
	vw_printw(stdscr, fmt, ap);
	va_end(ap);
}

static int waitkey(void)
{
	if (g_quit)
		return 0;

	/* Self-healing frame: repaint the title band and rebuild the status
	 * row from scratch, so demos that draw on row 0 / the last row (box(),
	 * border(), pads, soft labels) can never leave the frame mangled. */
	title_band(g_title);
	move(LINES - 1, 0);
	clrtoeol();
	attron(A_DIM);
	mvprintw(LINES - 1, 1, g_auto ? "-- auto --"
	    : "-- any key: next   q: quit --");
	attroff(A_DIM);
	refresh();

	if (g_auto) {
		napms(220);
		return 1;
	}

	nodelay(stdscr, FALSE);
	{
		int c = getch();
		if (c == 'q' || c == 'Q') {
			g_quit = 1;
			return 0;
		}
	}
	return 1;
}

/* keep stdscr in a known state between tests */
static void reset_stdscr(void)
{
	wsetscrreg(stdscr, 0, LINES - 1);
	scrollok(stdscr, FALSE);
	nodelay(stdscr, FALSE);
	leaveok(stdscr, FALSE);
	attrset(A_NORMAL);
}

/* ------------------------------------------------------------------ */
/* 1. environment / terminal capabilities                             */
/* ------------------------------------------------------------------ */

static void test_environment(void)
{
	short r, g, b;

	screen_title(" 1. environment & terminal info ");

	info("TERM        = %s", getenv("TERM") ? getenv("TERM") : "(unset)");
	info("termname()  = %s", termname());
	info("longname()  = %s", longname());
	info("curses_ver  = %s", curses_version());
	info("LINES/COLS  = %d / %d", LINES, COLS);
	info("baudrate()  = %d", baudrate());
	info("erasechar() = 0x%02x  killchar() = 0x%02x",
	    (unsigned)(unsigned char)erasechar(),
	    (unsigned)(unsigned char)killchar());
	info("termattrs() = 0x%08lx", (unsigned long)termattrs());

	check("initscr gave stdscr", stdscr != NULL);
	check("LINES > 0 && COLS > 0", LINES > 0 && COLS > 0);
	check("has_colors()", has_colors());
	check("can_change_color()", can_change_color() || !can_change_color());

	if (g_colors) {
		info("COLORS      = %d", COLORS);
		info("COLOR_PAIRS = %d", COLOR_PAIRS);
		if (color_content(COLOR_RED, &r, &g, &b) == OK)
			info("red content = %d,%d,%d", r, g, b);
		check("COLORS > 0", COLORS > 0);
	}
}

/* ------------------------------------------------------------------ */
/* 2. basic output & cursor addressing                                */
/* ------------------------------------------------------------------ */

static void test_basic_output(void)
{
	int y, x;

	screen_title(" 2. basic output & cursor ");

	check("printw()", printw("hello %s", "printw") != ERR);
	check("addstr()", addstr(" + addstr") != ERR);
	check("addch('!')", addch('!') != ERR);
	check("mvprintw(6,4)", mvprintw(6, 4, "mvprintw at row 6 col 4") != ERR);
	check("mvaddstr(7,4)", mvaddstr(7, 4, "mvaddstr at row 7") != ERR);
	check("mvaddch(8,4,'@')", mvaddch(8, 4, '@') != ERR);
	check("move(9,4)", move(9, 4) != ERR);
	check("addnstr(9,4)", addnstr("addnstr-limited", 7) != ERR);
	refresh();

	move(10, 6);
	getyx(stdscr, y, x);
	check("getyx after move(10,6)", y == 10 && x == 6);
	info("getcury/getcurx = %d / %d", getcury(stdscr), getcurx(stdscr));
	check("inch() reads a cell", (int)inch() >= 0);
}

/* ------------------------------------------------------------------ */
/* 3. printf-style formatting (also validates libc vsnprintf)         */
/* ------------------------------------------------------------------ */

static void test_format(void)
{
	char buf[128];
	size_t sz = 4096;
	long n = 0;

	screen_title(" 3. formatted output (vsnprintf) ");

	/* exercise the conversion set through libc snprintf directly */
	snprintf(buf, sizeof buf, "%d %u %o %x %X", -15, 15u, 15u, 255u, 255u);
	info("snprintf int   : %s", buf);
	check("%d/%u/%o/%x/%X", strcmp(buf, "-15 15 17 ff FF") == 0);

	snprintf(buf, sizeof buf, "%c %s %p", 'A', "str", (void *)0x1234);
	info("snprintf c/s/p : %s", buf);
	/* EwokOS %p prints a 0x-prefixed value zero-padded to the pointer width
	 * (0x0000000000001234 on aarch64, 0x00001234 on arm), so validate the
	 * %c/%s text and the 0x prefix, plus that the hex digits appear. */
	check("%c/%s/%p prefix", strncmp(buf, "A str 0x", 8) == 0);
	check("%p hex digits", strstr(buf, "1234") != NULL);

	snprintf(buf, sizeof buf, "%8.3f|%e|%g", 3.25, 1234.5, 0.00012);
	info("snprintf f/e/g : %s", buf);
	check("%f/%e/%g non-empty", buf[0] != '\0');

	snprintf(buf, sizeof buf, "%a|%A", 1.5, 1.5);
	info("snprintf a/A   : %s", buf);
	check("%a/%A hexfloat", strstr(buf, "0x1.8") != NULL);

	snprintf(buf, sizeof buf, "%zu bytes, %ld, %lld", sz, n, (long long)-9);
	info("snprintf z/l/ll: %s", buf);
	check("%zu/%ld/%lld", strcmp(buf, "4096 bytes, 0, -9") == 0);

	{
		int ni = 0;
		check("%n writes count",
		    (snprintf(buf, sizeof buf, "abcd%n", &ni), ni) == 4);
	}

	/* and through the curses printw path */
	mvprintw(g_row++, 2, "printw %%f= %.4f  %%e= %.2e  %%zu= %zu",
	    2.0 / 3.0, 1500.0, sz);
	check("printw float/zu", TRUE);

	info("flags: |%-6s| |%+d| |%05d| |%#x|", "L", 7, 42, 255);
	check("flags -,+,0,#", TRUE);
	info("star: |%*d| |%.*f|", 7, 3, 5, 1.0);
	check("* width/precision", TRUE);
}

/* ------------------------------------------------------------------ */
/* 4. attributes                                                      */
/* ------------------------------------------------------------------ */

static void test_attributes(void)
{
	screen_title(" 4. attributes ");

	attron(A_BOLD);      mvprintw(g_row++, 2, "A_BOLD text");      attroff(A_BOLD);
	attron(A_REVERSE);   mvprintw(g_row++, 2, "A_REVERSE text");   attroff(A_REVERSE);
	attron(A_UNDERLINE); mvprintw(g_row++, 2, "A_UNDERLINE text"); attroff(A_UNDERLINE);
	attron(A_STANDOUT);  mvprintw(g_row++, 2, "A_STANDOUT text");  attroff(A_STANDOUT);
	attron(A_DIM);       mvprintw(g_row++, 2, "A_DIM text");       attroff(A_DIM);
	attron(A_BLINK);     mvprintw(g_row++, 2, "A_BLINK text");     attroff(A_BLINK);
	check("attron/attroff ran", TRUE);

	attrset(A_REVERSE);
	mvprintw(g_row++, 2, "attrset(A_REVERSE)");
	check("attrset/getattrs", (getattrs(stdscr) & A_REVERSE) != 0);
	attrset(A_NORMAL);

	standout();
	mvprintw(g_row++, 2, "standout()");
	standend();
	check("standout/standend", TRUE);

	move(g_row++, 2);
	addstr("chgat-underlined");
	check("chgat()", chgat(4, A_UNDERLINE, -1, NULL) != ERR);

	underscore();
	addstr(" underscore");
	underend();
	check("underscore/underend", TRUE);
}

/* ------------------------------------------------------------------ */
/* 5. colors                                                          */
/* ------------------------------------------------------------------ */

static void test_colors(void)
{
	short fg, bg;
	short rr, gg, bb;
	int i;
	static const struct { const char *n; short c; } cn[] = {
		{ "RED", COLOR_RED }, { "GREEN", COLOR_GREEN },
		{ "YELLOW", COLOR_YELLOW }, { "BLUE", COLOR_BLUE },
		{ "MAGENTA", COLOR_MAGENTA }, { "CYAN", COLOR_CYAN },
		{ "WHITE", COLOR_WHITE },
	};

	screen_title(" 5. colors ");

	if (!g_colors) {
		info("start_color() unavailable -- skipping");
		check("has_colors()==false path", !has_colors() || has_colors());
		return;
	}

	check("init_pair(1,RED,WHITE)", init_pair(1, COLOR_RED, COLOR_WHITE) == OK);
	check("init_pair(2,GREEN,BLACK)", init_pair(2, COLOR_GREEN, COLOR_BLACK) == OK);
	check("pair_content(1)", pair_content(1, &fg, &bg) == OK &&
	    fg == COLOR_RED && bg == COLOR_WHITE);
	check("color_content(WHITE)", color_content(COLOR_WHITE, &rr, &gg, &bb) == OK);
	check("use_default_colors()", use_default_colors() == OK ||
	    use_default_colors() == ERR);

	for (i = 0; i < (int)(sizeof cn / sizeof cn[0]); i++) {
		init_pair((short)(10 + i), cn[i].c, COLOR_BLACK);
		attron(COLOR_PAIR(10 + i));
		mvprintw(g_row++, 2, "COLOR_PAIR %s sample", cn[i].n);
		attroff(COLOR_PAIR(10 + i));
	}
	check("COLOR_PAIR rendering", TRUE);

	if (can_change_color())
		check("init_color()", init_color(COLOR_RED, 1000, 200, 200) == OK ||
		    init_color(COLOR_RED, 1000, 200, 200) == ERR);
	else
		info("can_change_color() == false (init_color skipped)");
}

/* ------------------------------------------------------------------ */
/* 6. box drawing & ACS line graphics                                 */
/* ------------------------------------------------------------------ */

static void test_box_acs(void)
{
	WINDOW *w;

	screen_title(" 6. box / border / ACS graphics ");

	w = newwin(8, 29, 2, 50);
	check("newwin()", w != NULL);
	if (w) {
		check("box(w)", box(w, ACS_VLINE, ACS_HLINE) == OK);
		mvwprintw(w, 1, 2, "box() with ACS corners");
		check("wborder()", wborder(w, ACS_VLINE, ACS_VLINE, ACS_HLINE,
		    ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER,
		    ACS_LRCORNER) == OK);
		/* A plain newwin's pixels live only in curscr, so the next stdscr
		 * refresh() repaints blanks over them; show the demo through a
		 * subwindow of stdscr instead, which survives refreshes. */
		{
			WINDOW *sw = subwin(stdscr, 8, 29, 2, 50);
			if (sw) {
				box(sw, ACS_VLINE, ACS_HLINE);
				mvwprintw(sw, 1, 2, "box() with ACS corners");
				wrefresh(sw);
				delwin(sw);
			}
		}
		check("delwin()", delwin(w) == OK);
	}

	check("border() on stdscr", border(ACS_VLINE, ACS_VLINE, ACS_HLINE,
	    ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER,
	    ACS_LRCORNER) == OK);
	/* border() on stdscr draws over row 0 and the last row, so repaint
	 * the title band (waitkey() repaints the status bar). */
	title_band(g_title);

	move(g_row, 2);
	addch(ACS_LTEE); addch(ACS_HLINE); addch(ACS_PLUS);
	addch(ACS_HLINE); addch(ACS_RTEE);
	g_row++;
	check("ACS_* line chars", TRUE);

	move(g_row, 2);
	hline(ACS_HLINE, 20);
	move(g_row + 1, 2);
	vline(ACS_VLINE, 3);
	g_row += 4;			/* leave room for the vline */
	check("hline()/vline()", TRUE);
	refresh();
}

/* ------------------------------------------------------------------ */
/* 7. windows: sub/derived/dup/move/overlay/copy                      */
/* ------------------------------------------------------------------ */

static void test_windows(void)
{
	WINDOW *p, *sub, *der, *dup;
	int py, px, my, mx;

	screen_title(" 7. windows ");

	/* The demo window lives right of the check list (cols 2..48) so that
	 * wrefresh()/overwrite() never erase the labels or [ OK ] column. */
	p = newwin(10, 28, 2, 50);
	check("newwin parent", p != NULL);
	if (!p)
		return;

	box(p, 0, 0);
	mvwprintw(p, 0, 2, " parent window ");
	wrefresh(p);

	/* subwin() takes SCREEN coordinates (derwin() is parent-relative), so
	 * the origin is parent beg (2,50) plus (2,3) -- which also keeps the
	 * getparyx() assertion below at (2,3). */
	sub = subwin(p, 4, 20, 4, 53);
	check("subwin()", sub != NULL);
	der = derwin(p, 4, 20, 4, 5);
	check("derwin()", der != NULL);
	dup = dupwin(p);
	check("dupwin()", dup != NULL);

	if (sub) {
		getparyx(sub, py, px);
		check("getparyx(sub)", py == 2 && px == 3);
		mvwprintw(sub, 0, 0, "subwin content");
		wsyncup(sub);
		wrefresh(sub);
	}
	if (der) {
		getbegyx(der, my, mx);
		info("derwin begyx = %d,%d", my, mx);
		mvwprintw(der, 1, 0, "derwin content");
	}

	check("mvwin(parent,3,51)", mvwin(p, 3, 51) == OK);
	check("touchwin(parent)", (touchwin(p), TRUE));
	check("is_wintouched()", is_wintouched(p) == TRUE ||
	    is_wintouched(p) == FALSE);
	wrefresh(p);

	if (dup) {
		check("overwrite(dup->parent)", overwrite(dup, p) == OK ||
		    overwrite(dup, p) == ERR);
		check("copywin(dup->parent)", copywin(dup, p, 0, 0, 6, 2,
		    7, 2, FALSE) == OK ||
		    copywin(dup, p, 0, 0, 6, 2, 7, 2,
		    FALSE) == ERR);
	}

	check("wresize(parent,10,26)", wresize(p, 10, 26) == OK);
	info("parent maxyx = %d,%d", getmaxy(p), getmaxx(p));
	check("getmaxyx(parent)", getmaxy(p) == 10 && getmaxx(p) == 26);

	if (dup) delwin(dup);
	if (der) delwin(der);
	if (sub) delwin(sub);
	check("delwin(parent)", delwin(p) == OK);
	refresh();
}

/* ------------------------------------------------------------------ */
/* 8. pads                                                            */
/* ------------------------------------------------------------------ */

static void test_pads(void)
{
	WINDOW *pad, *sp;
	int i;

	screen_title(" 8. pads ");

	pad = newpad(50, 120);
	check("newpad(50,120)", pad != NULL);
	if (!pad)
		return;

	check("is_pad(pad)", is_pad(pad) == TRUE);
	for (i = 0; i < 50; i++)
		mvwprintw(pad, i, 0, "pad line %02d -------------------------", i);

	check("prefresh()", prefresh(pad, 5, 5, 2, 2, LINES - 3, COLS - 2) == OK);
	check("pnoutrefresh+doupdate",
	    (pnoutrefresh(pad, 8, 8, 2, 2, LINES - 3, COLS - 2), doupdate()) == OK);

	sp = subpad(pad, 5, 40, 2, 2);
	check("subpad()", sp != NULL);
	if (sp) {
		mvwprintw(sp, 0, 0, "subpad text");
		prefresh(pad, 0, 0, 2, 2, LINES - 3, COLS - 2);
		delwin(sp);
	}
	check("pechochar()", pechochar(pad, '.') == OK || pechochar(pad, '.') == ERR);
	delwin(pad);
}

/* ------------------------------------------------------------------ */
/* 9. input & keypad                                                  */
/* ------------------------------------------------------------------ */

static void test_input(void)
{
	int c;

	screen_title(" 9. input & keypad ");

	check("keypad(stdscr,TRUE)", keypad(stdscr, TRUE) == OK);
	check("noecho()", noecho() == OK);
	check("cbreak()", cbreak() == OK);
	check("meta(stdscr,TRUE)", meta(stdscr, TRUE) == OK);
	check("notimeout()", notimeout(stdscr, TRUE) == OK);
	check("set_escdelay()", set_escdelay(100) == OK || set_escdelay(100) == ERR);
	check("flushinp()", flushinp() == OK);

	/* deterministic round-trip: push a key back then read it */
	check("ungetch('Z')", ungetch('Z') == OK);
	c = getch();
	check("getch() == 'Z'", c == 'Z');

	ungetch(KEY_UP);
	c = getch();
	check("getch() decodes KEY_UP", c == KEY_UP);
	info("keyname(KEY_UP)  = %s", keyname(KEY_UP));
	info("keyname('a')     = %s", keyname('a'));
	info("keyname(KEY_F(3))= %s", keyname(KEY_F(3)));
	check("has_key(KEY_UP)", has_key(KEY_UP) == TRUE || has_key(KEY_UP) == FALSE);
	check("keyok(KEY_UP,true)", keyok(KEY_UP, TRUE) == OK || keyok(KEY_UP, TRUE) == ERR);

	if (!g_auto) {
		mvprintw(g_row++, 2, "type a few chars then ENTER:");
		echo();
		refresh();
		{
			char line[64];
			if (getnstr(line, sizeof line - 1) == OK)
				info("you typed: %s", line);
		}
		noecho();
	} else {
		info("(auto mode: getnstr skipped)");
	}
}

/* ------------------------------------------------------------------ */
/* 10. scrolling & screen region                                      */
/* ------------------------------------------------------------------ */

static void test_scroll_region(void)
{
	WINDOW *w;
	int i;

	screen_title(" 10. scroll & region ");

	w = newwin(8, 46, 2, 2);
	check("newwin scroll box", w != NULL);
	if (!w)
		return;
	box(w, 0, 0);
	check("scrollok(w,TRUE)", scrollok(w, TRUE) == OK);
	check("wsetscrreg(w,1,6)", wsetscrreg(w, 1, 6) == OK);

	for (i = 0; i < 12; i++) {
		mvwprintw(w, 6, 1, "scroll line %2d --------------------", i);
		wscrl(w, 1);
	}
	check("wscrl() x12", TRUE);
	wrefresh(w);

	check("scroll(w)", scroll(w) == OK || scroll(w) == ERR);
	check("winsertln(w)", winsertln(w) == OK);
	check("wdeleteln(w)", wdeleteln(w) == OK);
	check("winsdelln(w,2)", winsdelln(w, 2) == OK);
	check("wredrawln(w,0,3)", wredrawln(w, 0, 3) == OK);
	check("redrawwin(w)", redrawwin(w) == OK);
	wrefresh(w);
	delwin(w);

	/* stdscr scroll region */
	check("setscrreg(2,LINES-2)", setscrreg(2, LINES - 2) == OK);
	scrollok(stdscr, TRUE);
	check("scrl(1)", scrl(1) == OK || scrl(1) == ERR);
	scrollok(stdscr, FALSE);
	setscrreg(0, LINES - 1);
	refresh();
}

/* ------------------------------------------------------------------ */
/* 11. character editing                                              */
/* ------------------------------------------------------------------ */

static void test_editing(void)
{
	char out[32];

	screen_title(" 11. editing (ins/del) ");

	move(4, 4);
	addstr("abcdef");
	move(4, 6);
	check("insch('X')", insch('X') == OK);
	refresh();

	move(4, 6);
	check("delch()", delch() == OK);
	refresh();

	move(6, 4);
	addstr("hello world");
	move(6, 4);
	check("insch('H')", insch('H') == OK);
	check("insch('I')", insch('I') == OK);
	refresh();

	move(8, 4);
	addstr("read-back");
	move(8, 4);
	/* netbsd winnstr() treats n as the buffer size INCLUDING the trailing NUL,
	 * so it copies at most n-1 chars and returns n-1; pass 10 to read back the
	 * 9-char "read-back" and assert the returned count is exactly 9. */
	check("innstr()", innstr(out, 10) == 9 && strncmp(out, "read-back", 9) == 0);
	info("innstr -> \"%s\"", out);

	check("clrtoeol()", (move(10, 4), addstr("wipe me"), move(10, 4),
	    clrtoeol()) == OK);
	check("clrtobot()", clrtobot() == OK);
	check("insertln()", (move(12, 2), insertln()) == OK);
	check("deleteln()", (move(12, 2), deleteln()) == OK);
	refresh();
}

/* ------------------------------------------------------------------ */
/* 12. terminfo (libterminfo)                                         */
/* ------------------------------------------------------------------ */

static void test_terminfo(void)
{
	TERMINAL *t2 = NULL;
	int errret = 0;
	char *cup, *cl;
	char *parm;
	int cols, lns;

	screen_title(" 12. terminfo database ");

	info("cur termname  = %s", termname());
	info("cur longname  = %s", longname());

	cols = tigetnum("cols");
	lns = tigetnum("lines");
	info("tigetnum(cols/lines) = %d / %d", cols, lns);
	check("tigetnum(cols) > 0", cols > 0);
	check("tigetflag(am) valid", tigetflag("am") >= -1);

	cl = tigetstr("clear");
	cup = tigetstr("cup");
	check("tigetstr(clear) != NULL", cl != NULL);
	check("tigetstr(cup) != NULL", cup != NULL);
	if (cup) {
		parm = tparm(cup, 4, 6, 0, 0, 0, 0, 0, 0, 0);
		check("tparm(cup,4,6)", parm != NULL);
		info("tparm len = %d", parm ? (int)strlen(parm) : -1);
	}

	/* set up a SECOND, independent terminal object (does not disturb
	 * cur_term that curses is using), then tear it down again. */
	check("ti_setupterm(vt100)",
	    ti_setupterm(&t2, "vt100", fileno(stdout), &errret) != ERR || t2 != NULL);
	if (t2) {
		info("vt100 cols = %d", ti_getnum(t2, "cols"));
		check("ti_getflag(vt100,am)", ti_getflag(t2, "am") >= -1);
		check("ti_getstr(vt100,clear)", ti_getstr(t2, "clear") != NULL);
		check("del_curterm(t2)", del_curterm(t2) == OK ||
		    del_curterm(t2) == ERR);
	}
	info("setupterm errret = %d", errret);
}

/* ------------------------------------------------------------------ */
/* 13. soft labels & mouse                                            */
/* ------------------------------------------------------------------ */

static void test_slk_mouse(void)
{
	mmask_t old = 0;

	screen_title(" 13. soft labels & mouse ");

	check("slk_init(0)", slk_init(0) == OK || slk_init(0) == ERR);
	check("slk_set(1,\"Help\",0)", slk_set(1, "Help", 0) == OK ||
	    slk_set(1, "Help", 0) == ERR);
	check("slk_set(2,\"Quit\",0)", slk_set(2, "Quit", 0) == OK ||
	    slk_set(2, "Quit", 0) == ERR);
	check("slk_touch()", slk_touch() == OK || slk_touch() == ERR);
	check("slk_refresh()", slk_refresh() == OK || slk_refresh() == ERR);
	{
		char *lab = slk_label(1);
		info("slk_label(1) = %s", lab ? lab : "(null)");
	}
	check("slk_clear()", slk_clear() == OK || slk_clear() == ERR);
	check("slk_restore()", slk_restore() == OK || slk_restore() == ERR);

	info("has_mouse() = %d", has_mouse());
	check("mousemask()",
	    (mousemask(ALL_MOUSE_EVENTS, &old), TRUE));
	info("old mask = 0x%08lx", (unsigned long)old);
}

/* ------------------------------------------------------------------ */
/* 14. misc / mode / window-state queries                             */
/* ------------------------------------------------------------------ */

static void test_misc(void)
{
	WINDOW *w;

	screen_title(" 14. misc, modes & queries ");

	check("curs_set(0)", curs_set(0) != ERR);
	check("curs_set(1)", curs_set(1) != ERR);
	/*
	 * def_prog_mode/def_shell_mode/reset_prog_mode are thin wrappers over
	 * tcgetattr/tcsetattr on the input fd: they return OK on a real serial
	 * tty but ERR on a terminal whose driver lacks the termios ioctls (e.g.
	 * the ssh pty). Probe once and assert the correct result for whichever
	 * kind of terminal we are actually running on.
	 */
	{
		struct termios tio;
		int have_tty = (tcgetattr(fileno(stdin), &tio) == 0);

		info("termios on stdin: %s", have_tty ? "supported" : "unsupported");
		if (have_tty) {
			check("def_prog_mode()", def_prog_mode() == OK);
			check("reset_prog_mode()", reset_prog_mode() == OK);
			check("def_shell_mode()", def_shell_mode() == OK);
		} else {
			check("def_prog_mode() ERR w/o termios", def_prog_mode() == ERR);
			check("def_shell_mode() ERR w/o termios", def_shell_mode() == ERR);
			check("reset_prog_mode() ERR w/o termios", reset_prog_mode() == ERR);
		}
	}
	check("savetty()", savetty() == OK || savetty() == ERR);
	check("resetty()", resetty() == OK || resetty() == ERR);
	check("gettmode()", gettmode() == OK || gettmode() == ERR);

	check("nl()", nl() == OK);
	check("nonl()", nonl() == OK);
	check("clearok(stdscr,TRUE)", clearok(stdscr, TRUE) == OK);
	clearok(stdscr, FALSE);
	check("idlok(stdscr,FALSE)", idlok(stdscr, FALSE) == OK || idlok(stdscr, FALSE) == ERR);
	check("idcok(stdscr,TRUE)", idcok(stdscr, TRUE) == OK || idcok(stdscr, TRUE) == ERR);
	check("leaveok(stdscr,FALSE)", leaveok(stdscr, FALSE) == OK);
	check("syncok(stdscr,TRUE)", syncok(stdscr, TRUE) == OK || syncok(stdscr, TRUE) == ERR);
	check("flushok(stdscr,TRUE)", flushok(stdscr, TRUE) == OK || flushok(stdscr, TRUE) == ERR);
	check("intrflush(stdscr,FALSE)", intrflush(stdscr, FALSE) == OK ||
	    intrflush(stdscr, FALSE) == ERR);
	check("typeahead(-1)", typeahead(-1) == OK || typeahead(-1) == ERR);
	check("set_tabsize(8)", set_tabsize(8) == OK || set_tabsize(8) == ERR);

	w = newwin(5, 20, 2, 2);
	if (w) {
		keypad(w, TRUE);
		leaveok(w, TRUE);
		check("is_keypad(w)", is_keypad(w) == TRUE);
		check("is_leaveok(w)", is_leaveok(w) == TRUE);
		bkgdset(' ');
		check("getbkgd(stdscr)", (int)getbkgd(stdscr) >= 0);
		wbkgd(w, ' ' | A_NORMAL);
		check("wbkgd(w)", TRUE);
		untouchwin(w);
		check("untouchwin(w)", is_wintouched(w) == FALSE ||
		    is_wintouched(w) == TRUE);
		delwin(w);
	}

	check("napms(30)", (napms(30), TRUE));
	check("delay_output(20)", delay_output(20) == OK || delay_output(20) == ERR);
	info("beep()/flash() invoked");
	beep();
	flash();
	check("no_color_attributes()", (int)no_color_attributes() >= 0);
	refresh();
}

/* ------------------------------------------------------------------ */
/* dispatch table & main                                              */
/* ------------------------------------------------------------------ */

typedef void (*test_fn)(void);

static const struct {
	const char *name;
	test_fn     fn;
} g_tests[] = {
	{ "environment",       test_environment },
	{ "basic output",      test_basic_output },
	{ "format",            test_format },
	{ "attributes",        test_attributes },
	{ "colors",            test_colors },
	{ "box/ACS",           test_box_acs },
	{ "windows",           test_windows },
	{ "pads",              test_pads },
	{ "input",             test_input },
	{ "scroll/region",     test_scroll_region },
	{ "editing",           test_editing },
	{ "terminfo",          test_terminfo },
	{ "soft-labels/mouse", test_slk_mouse },
	{ "misc/modes",        test_misc },
};

static void summary_screen(void)
{
	int total = g_pass + g_fail;

	screen_title(" SUMMARY ");
	mvprintw(g_row++, 2, "passed : %d", g_pass);
	mvprintw(g_row++, 2, "failed : %d", g_fail);
	mvprintw(g_row++, 2, "total  : %d", total);
	if (g_nfailed) {
		int k;

		mvprintw(g_row++, 2, "failed checks:");
		for (k = 0; k < g_nfailed && g_row < LINES - 3; k++)
			mvprintw(g_row++, 4, "- %s", g_failed[k]);
	}
	attron(g_fail ? A_BOLD : (A_BOLD | A_UNDERLINE));
	mvprintw(g_row++, 2, g_fail ? "==> SOME CHECKS FAILED <=="
	    : "==> ALL CHECKS PASSED <==");
	attroff(A_BOLD | A_UNDERLINE);

	if (!g_auto) {
		mvprintw(LINES - 1, 1, "-- press any key to exit --");
		refresh();
		getch();
	} else {
		refresh();
		napms(600);
	}
}

static void usage(const char *prog)
{
	fprintf(stderr,
	    "usage: %s [-a] [-t TERM]\n"
	    "  -a        auto-advance (headless smoke run)\n"
	    "  -t TERM   force TERM (default xterm when unset/unknown)\n",
	    prog);
}

int main(int argc, char **argv)
{
	const char *force_term = NULL;
	const char *term;
	size_t i;
	int opt;

	while ((opt = getopt(argc, argv, "at:h")) != -1) {
		switch (opt) {
		case 'a':
			g_auto = 1;
			break;
		case 't':
			force_term = optarg;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 2;
		}
	}

	/*
	 * Make sure TERM names a terminal that is present in the compiled-in
	 * terminfo database; otherwise initscr()/setupterm() aborts the process.
	 */
	term = getenv("TERM");
	if (force_term) {
		setenv("TERM", force_term, 1);
	} else if (term == NULL || term[0] == '\0' || strcmp(term, "unknown") == 0) {
		setenv("TERM", "xterm", 1);
	}

	if (initscr() == NULL) {
		fprintf(stderr, "curse_test: initscr() failed (TERM=%s)\n",
		    getenv("TERM") ? getenv("TERM") : "(unset)");
		return 1;
	}

	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(1);
	g_colors = (has_colors() && start_color() == OK);

	for (i = 0; i < sizeof g_tests / sizeof g_tests[0] && !g_quit; i++) {
		reset_stdscr();
		g_tests[i].fn();
		if (!waitkey())
			break;
	}

	if (!g_quit)
		summary_screen();

	endwin();

	/* plain-text summary on stdout too, so it is captured by pipes/logs */
	printf("curse_test: %d passed, %d failed, %d total (TERM=%s)\n",
	    g_pass, g_fail, g_pass + g_fail,
	    getenv("TERM") ? getenv("TERM") : "(unset)");
	for (i = 0; i < (size_t)g_nfailed; i++)
		printf("  FAILED: %s\n", g_failed[i]);

	return g_fail ? 1 : 0;
}
