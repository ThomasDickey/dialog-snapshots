/*
 *  $Id: util.c,v 1.106 2003/08/31 00:50:27 tom Exp $
 *
 *  util.c
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *  and     Thomas E. Dickey
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dialog.h"

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef NCURSES_VERSION
#if defined(HAVE_NCURSESW_TERM_H)
#include <ncursesw/term.h>
#elif defined(HAVE_NCURSES_TERM_H)
#include <ncurses/term.h>
#else
#include <term.h>
#endif
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef S_IRUSR
#define S_IRUSR 0400
#endif

#ifndef S_IWUSR
#define S_IWUSR 0200
#endif

#ifndef DIALOG_TMPDIR
#define DIALOG_TMPDIR NULL
#endif

#define LOCK_PERMITS (S_IRUSR | S_IWUSR)
#define LOCK_TIMEOUT 10		/* timeout for locking, in seconds */

/* globals */
FILE *pipe_fp;			/* copy of stdin, for reading a pipe */
DIALOG_STATE dialog_state;
DIALOG_VARS dialog_vars;
int defaultno = FALSE;
int screen_initialized = 0;
static FILE *my_output = 0;	/* prefer to stdout, to support --stdout */

#ifdef HAVE_COLOR
/* use colors by default? */
bool use_colors = USE_COLORS;
/* shadow dialog boxes by default?
   Note that 'use_shadow' implies 'use_colors' */
bool use_shadow = USE_SHADOW;
#endif

#define concat(a,b) a##b

#ifdef HAVE_RC_FILE
#define RC_DATA(name,comment) , #name "_color", comment " color"
#else
#define RC_DATA(name,comment)	/*nothing */
#endif

#ifdef HAVE_COLOR
#include <colors.h>
#define COLOR_DATA(upr) , concat(upr,_FG), concat(upr,_BG), concat(upr,_HL)
#else
#define COLOR_DATA(upr)		/*nothing */
#endif

#define DATA(atr,upr,lwr,cmt) { atr COLOR_DATA(upr) RC_DATA(lwr,cmt) }

/*
 * Table of color and attribute values, default is for mono display.
 */
/* *INDENT-OFF* */
DIALOG_COLORS color_table[] =
{
    DATA(A_NORMAL,	SCREEN,			screen, "Screen"),
    DATA(A_NORMAL,	SHADOW,			shadow, "Shadow"),
    DATA(A_REVERSE,	DIALOG,			dialog, "Dialog box"),
    DATA(A_REVERSE,	TITLE,			title, "Dialog box title"),
    DATA(A_REVERSE,	BORDER,			border, "Dialog box border"),
    DATA(A_BOLD,	BUTTON_ACTIVE,		button_active, "Active button"),
    DATA(A_DIM,		BUTTON_INACTIVE,	button_inactive, "Inactive button"),
    DATA(A_UNDERLINE,	BUTTON_KEY_ACTIVE,	button_key_active, "Active button key"),
    DATA(A_UNDERLINE,	BUTTON_KEY_INACTIVE,	button_key_inactive, "Inactive button key"),
    DATA(A_NORMAL,	BUTTON_LABEL_ACTIVE,	button_label_active, "Active button label"),
    DATA(A_NORMAL,	BUTTON_LABEL_INACTIVE,	button_label_inactive, "Inactive button label"),
    DATA(A_REVERSE,	INPUTBOX,		inputbox, "Input box"),
    DATA(A_REVERSE,	INPUTBOX_BORDER,	inputbox_border, "Input box border"),
    DATA(A_REVERSE,	SEARCHBOX,		searchbox, "Search box"),
    DATA(A_REVERSE,	SEARCHBOX_TITLE,	searchbox_title, "Search box title"),
    DATA(A_REVERSE,	SEARCHBOX_BORDER,	searchbox_border, "Search box border"),
    DATA(A_REVERSE,	POSITION_INDICATOR,	position_indicator, "File position indicator"),
    DATA(A_REVERSE,	MENUBOX,		menubox, "Menu box"),
    DATA(A_REVERSE,	MENUBOX_BORDER,		menubox_border, "Menu box border"),
    DATA(A_REVERSE,	ITEM,			item, "Item"),
    DATA(A_NORMAL,	ITEM_SELECTED,		item_selected, "Selected item"),
    DATA(A_REVERSE,	TAG,			tag, "Tag"),
    DATA(A_REVERSE,	TAG_SELECTED,		tag_selected, "Selected tag"),
    DATA(A_NORMAL,	TAG_KEY,		tag_key, "Tag key"),
    DATA(A_BOLD,	TAG_KEY_SELECTED,	tag_key_selected, "Selected tag key"),
    DATA(A_REVERSE,	CHECK,			check, "Check box"),
    DATA(A_REVERSE,	CHECK_SELECTED,		check_selected, "Selected check box"),
    DATA(A_REVERSE,	UARROW,			uarrow, "Up arrow"),
    DATA(A_REVERSE,	DARROW,			darrow, "Down arrow"),
    DATA(A_NORMAL,	ITEMHELP,		itemhelp, "Item help-text"),
    DATA(A_BOLD,	FORM_ACTIVE_TEXT,	form_active_text, "Active form text"),
    DATA(A_REVERSE,	FORM_TEXT,		form_text, "Form text"),
};
/* *INDENT-ON* */

/*
 * Display background title if it exists ...
 */
void
put_backtitle(void)
{
    int i;

    if (dialog_vars.backtitle != NULL) {
	chtype attr = A_NORMAL;

	wattrset(stdscr, screen_attr);
	(void) wmove(stdscr, 0, 1);
	dlg_print_text(stdscr, dialog_vars.backtitle, COLS - 2, &attr);
	for (i = 0; i < COLS - (int) strlen(dialog_vars.backtitle); i++)
	    (void) waddch(stdscr, ' ');
	(void) wmove(stdscr, 1, 1);
	for (i = 0; i < COLS - 2; i++)
	    (void) waddch(stdscr, ACS_HLINE);
    }

    (void) wnoutrefresh(stdscr);
}

/*
 * Set window to attribute 'attr'.  There are more efficient ways to do this,
 * but will not work on older/buggy ncurses versions.
 */
void
attr_clear(WINDOW *win, int height, int width, chtype attr)
{
    int i, j;

    wattrset(win, attr);
    for (i = 0; i < height; i++) {
	(void) wmove(win, i, 0);
	for (j = 0; j < width; j++)
	    (void) waddch(win, ' ');
    }
    (void) touchwin(win);
}

void
dialog_clear(void)
{
    attr_clear(stdscr, LINES, COLS, screen_attr);
}

#define isprivate(s) ((s) != 0 && strstr(s, "\033[?") != 0)

#define TTY_DEVICE "/dev/tty"

static int
open_terminal(char **result, int mode)
{
    const char *device = TTY_DEVICE;
    if (!isatty(fileno(stderr))
	|| (device = ttyname(fileno(stderr))) == 0) {
	if (!isatty(fileno(stdout))
	    || (device = ttyname(fileno(stdout))) == 0) {
	    if (!isatty(fileno(stdin))
		|| (device = ttyname(fileno(stdin))) == 0) {
		device = TTY_DEVICE;
	    }
	}
    }
    *result = strclone(device);
    return open(device, mode);
}

/*
 * Do some initialization for dialog
 */
void
init_dialog(void)
{
    int fd1, fd2;
    char *device = 0;

#ifdef HAVE_RC_FILE
    if (parse_rc() == -1)	/* Read the configuration file */
	exiterr("init_dialog: parse_rc");
#endif

    /*
     * Some widgets (such as gauge) may read from the standard input.
     * That would get in the way of curses' normal reading stdin for getch.
     * If we're not reading from a tty, see if we can open /dev/tty.
     */
    pipe_fp = stdin;
    if (!isatty(fileno(stdin))) {
	if ((fd1 = open_terminal(&device, O_RDONLY)) >= 0
	    && (fd2 = dup(fileno(stdin))) >= 0) {
	    pipe_fp = fdopen(fd2, "r");
	    *stdin = *freopen(device, "r", stdin);
	    if (fileno(stdin) != 0)	/* some functions may read fd #0 */
		(void) dup2(fileno(stdin), 0);
	}
	free(device);
    }

    /*
     * If stdout is not a tty, assume dialog is called with the --stdout
     * option.  We cannot test the option from this point, but it is a fairly
     * safe assumption.  The curses library normally writes its output to
     * stdout, leaving stderr free for scripting.  Scripts are simpler when
     * stdout is redirected.  However, we have to find a way to write curses'
     * output.  The newterm function is useful; it allows us to specify where
     * the output goes.  The simplest solution uses stderr for the output.  If
     * stderr cannot be used, then we try to reopen our terminal.  Reopening
     * the terminal is a last resort since several configurations do not allow
     * this to work properly:
     *
     * a) some getty implementations (and possibly broken tty drivers, e.g., on
     *    HPUX 10 and 11) cause stdin to act as if it is still in cooked mode
     *    even though results from ioctl's state that it is successfully
     *    altered to raw mode.  Broken is the proper term.
     *
     * b) the user may not have permissions on the device, e.g., if one su's
     *    from the login user to another non-privileged user.
     *
     * If stdout is a tty, none of this need apply, so we use initscr.
     */
    if (!isatty(fileno(stdout))) {
	if (!isatty(fileno(stderr))
	    || newterm(NULL, stderr, stdin) != 0) {
	    if ((fd1 = open_terminal(&device, O_WRONLY)) >= 0
		&& (my_output = fdopen(fd1, "w")) != 0) {
		if (newterm(NULL, my_output, stdin) == 0) {
		    exiterr("cannot initialize curses");
		}
		free(device);
	    }
	}
    } else {
	my_output = stdout;
	(void) initscr();
    }
#ifdef NCURSES_VERSION
    /*
     * Cancel xterm's alternate-screen mode.
     */
    if ((my_output != stdout || isatty(fileno(my_output)))
	&& key_mouse != 0	/* xterm and kindred */
	&& isprivate(enter_ca_mode)
	&& isprivate(exit_ca_mode)) {
	/*
	 * initscr() or newterm() already did putp(enter_ca_mode) as a side
	 * effect of initializing the screen.  It would be nice to not even
	 * do that, but we do not really have access to the correct copy of
	 * the terminfo description until those functions have been invoked.
	 */
	(void) putp(exit_ca_mode);
	(void) putp(clear_screen);
	/*
	 * Prevent ncurses from switching "back" to the normal screen when
	 * exiting from dialog.  That would move the cursor to the original
	 * location saved in xterm.  Normally curses sets the cursor position
	 * to the first line after the display, but the alternate screen
	 * switching is done after that point.
	 *
	 * Cancelling the strings altogether also works around the buggy
	 * implementation of alternate-screen in rxvt, etc., which clear
	 * more of the display than they should.
	 */
	enter_ca_mode = 0;
	exit_ca_mode = 0;
    }
#endif
    (void) keypad(stdscr, TRUE);
    (void) cbreak();
    (void) noecho();
    mouse_open();
    screen_initialized = 1;

#ifdef HAVE_COLOR
    if (use_colors || use_shadow)	/* Set up colors */
	color_setup();
#endif

    /* Set screen to screen attribute */
    dialog_clear();
}

#ifdef HAVE_COLOR
static int defined_colors = 0;
/*
 * Setup for color display
 */
void
color_setup(void)
{
    unsigned i;

    if (has_colors()) {		/* Terminal supports color? */
	(void) start_color();

	for (i = 0; i < sizeof(color_table) / sizeof(color_table[0]); i++) {

	    /* Initialize color pairs */
	    (void) init_pair(i + 1, color_table[i].fg, color_table[i].bg);

	    /* Setup color attributes */
	    color_table[i].atr = C_ATTR(color_table[i].hilite, i + 1);
	}
	defined_colors = i + 1;
    }
}

int
dlg_color_count(void)
{
    return sizeof(color_table) / sizeof(color_table[0]);
}

/*
 * Reuse color pairs (they are limited), returning a COLOR_PAIR() value if we
 * have (or can) define a pair with the given color as foreground on the
 * window's defined background.
 */
static chtype
define_color(WINDOW *win, int foreground)
{
    chtype result = 0;
    chtype attrs = getattrs(win);
    int pair;
    short fg, bg, background;
    bool found = FALSE;

    if ((pair = PAIR_NUMBER(attrs)) != 0
	&& pair_content(pair, &fg, &bg) != ERR
	&& bg != foreground) {
	background = bg;
    } else {
	background = COLOR_BLACK;
    }

    for (pair = 0; pair < defined_colors; ++pair) {
	if (pair_content(pair, &fg, &bg) != ERR
	    && fg == foreground
	    && bg == background) {
	    result = COLOR_PAIR(pair);
	    found = TRUE;
	    break;
	}
    }
    if (!found && (defined_colors + 1) < COLOR_PAIRS) {
	pair = defined_colors++;
	(void) init_pair(pair, foreground, background);
	result = COLOR_PAIR(pair);
    }
    return result;
}
#endif

/*
 * End using dialog functions.
 */
void
end_dialog(void)
{
    if (screen_initialized) {
	screen_initialized = 0;
	mouse_close();
	(void) endwin();
	(void) fflush(stdout);
    }
}

#define isOurEscape(p) (((p)[0] == '\\') && ((p)[1] == 'Z') && ((p)[2] != 0))

static int
centered(int width, const char *string)
{
    int len = strlen(string);
    int left;
    int hide = 0;
    int n;

    if (dialog_vars.colors) {
	for (n = 0; n < len; ++n) {
	    if (isOurEscape(string + n)) {
		hide += 3;
	    }
	}
    }
    left = (width - (len - hide)) / 2 - 1;
    if (left < 0)
	left = 0;
    return left;
}

/*
 * Print up to 'len' bytes from 'text', optionally rendering our escape
 * sequence for attributes and color.
 */
void
dlg_print_text(WINDOW *win, const char *txt, int len, chtype *attr)
{
    while (len > 0 && (*txt != '\0')) {
	if (dialog_vars.colors) {
	    while (isOurEscape(txt)) {
		int code;

		txt += 2;
		switch (code = CharOf(*txt)) {
#ifdef HAVE_COLOR
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		    *attr &= ~A_COLOR;
		    *attr |= define_color(win, code - '0');
		    break;
#endif
		case 'B':
		    *attr &= ~A_BOLD;
		    break;
		case 'b':
		    *attr = A_BOLD;
		    break;
		case 'R':
		    *attr &= ~A_REVERSE;
		    break;
		case 'r':
		    *attr = A_REVERSE;
		    break;
		case 'U':
		    *attr &= ~A_UNDERLINE;
		    break;
		case 'u':
		    *attr = A_UNDERLINE;
		    break;
		case 'n':
		    *attr = A_NORMAL;
		    break;
		}
		++txt;
	    }
	    if (*txt == '\n')
		break;
	}
	(void) waddch(win, CharOf(*txt++) | *attr);
	--len;
    }
}

/*
 * Print one line of the prompt in the window within the limits of the
 * specified right margin.  The line will end on a word boundary and a pointer
 * to the start of the next line is returned, or a NULL pointer if the end of
 * *prompt is reached.
 */
static const char *
print_line(WINDOW *win, chtype *attr, const char *prompt, int lm, int rm, int *x)
{
    const char *wrap_ptr = prompt;
    const char *test_ptr = prompt;
    const int *cols = dlg_index_columns(prompt);
    const int *indx = dlg_index_wchars(prompt);
    int cur_x = lm;
    int hidden = 0;
    int limit = dlg_count_wchars(prompt);
    int n;

    *x = 1;

    /*
     * Set *test_ptr to the end of the line or the right margin (rm), whichever
     * is less, and set *wrap_ptr to the end of the last word in the line.
     */
    for (n = 0; n < limit; ++n) {
	if (*test_ptr == '\n' || *test_ptr == '\0' || cur_x >= (rm + hidden))
	    break;
	if (*test_ptr == ' ' && n != 0 && prompt[indx[n - 1]] != ' ') {
	    wrap_ptr = prompt + indx[n];
	    *x = cur_x;
	} else if (isOurEscape(test_ptr)) {
	    hidden += 3;
	    n += 2;
	}
	cur_x = lm + cols[n + 1];
	if (cur_x > (rm + hidden))
	    break;
	test_ptr = prompt + indx[n + 1];
    }

    /*
     * If the line doesn't reach the right margin in the middle of a word, then
     * we don't have to wrap it at the end of the previous word.
     */
    if (*test_ptr == '\n' || *test_ptr == ' ' || *test_ptr == '\0') {
	int i = 0;
	while (&wrap_ptr[++i] < test_ptr) {
	    ;
	}
	while (wrap_ptr[i - 1] == ' ') {
	    i--;
	}
	wrap_ptr += i;
	*x += i;
    }

    /*
     * If the line has no spaces, then wrap it anyway at the right margin
     */
    else if (*x == 1 && cur_x >= rm) {
	*x = rm;
	wrap_ptr = test_ptr;
    }

    /*
     * Print the line if we have a window pointer.  Otherwise this routine
     * is just being called for sizing the window.
     */
    if (win) {
	dlg_print_text(win, prompt, (wrap_ptr - prompt - hidden), attr);
    }

    /* *x tells the calling function how long the line was */
    if (*x == 1)
	*x = rm;

    /* Find the start of the next line and return a pointer to it */
    test_ptr = wrap_ptr;
    while (*test_ptr == ' ')
	test_ptr++;
    if (*test_ptr == '\n')
	test_ptr++;
    return (test_ptr);
}

static void
justify_text(WINDOW *win,
	     const char *prompt,
	     int limit_y,
	     int limit_x,
	     int *high, int *wide)
{
    chtype attr = A_NORMAL;
    int x = (2 * MARGIN);
    int y = 1;
    int max_x = 2;
    int lm = (2 * MARGIN);	/* left margin */
    int rm = limit_x;		/* right margin */
    int bm = limit_y;		/* bottom margin */

    if (win) {
	rm -= (2 * MARGIN);
	bm -= (2 * MARGIN);
    }
    if (prompt == 0)
	prompt = "";

    while (y <= bm && *prompt) {
	x = lm;

	if (*prompt == '\n') {
	    while (*prompt == '\n' && y < bm) {
		if (*(prompt + 1) != '\0') {
		    ++y;
		    if (win != 0)
			(void) wmove(win, y, lm);
		}
		prompt++;
	    }
	} else if (win != 0)
	    (void) wmove(win, y, lm);

	if (*prompt)
	    prompt = print_line(win, &attr, prompt, lm, rm, &x);
	if (*prompt) {
	    ++y;
	    if (win != 0)
		(void) wmove(win, y, lm);
	}
	max_x = MAX(max_x, x);
    }

    /* Set the final height and width for the calling function */
    if (high != 0)
	*high = y;
    if (wide != 0)
	*wide = max_x;
}

/*
 * Print a string of text in a window, automatically wrap around to the
 * next line if the string is too long to fit on one line. Note that the
 * string may contain embedded newlines.
 */
void
print_autowrap(WINDOW *win, const char *prompt, int height, int width)
{
    justify_text(win, prompt,
		 height,
		 width,
		 (int *) 0, (int *) 0);
}

/*
 * Calculate the window size for preformatted text.  This will calculate box
 * dimensions that are at or close to the specified aspect ratio for the prompt
 * string with all spaces and newlines preserved and additional newlines added
 * as necessary.
 */
static void
auto_size_preformatted(const char *prompt, int *height, int *width)
{
    int high, wide;
    float car;			/* Calculated Aspect Ratio */
    float diff;
    int max_y = SLINES - 1;
    int max_x = SCOLS - 2;
    int max_width = max_x;
    int ar = dialog_vars.aspect_ratio;

    /* Get the initial dimensions */
    justify_text((WINDOW *) 0, prompt, max_y, max_x, &high, &wide);
    car = (float) (wide / high);

    /*
     * If the aspect ratio is greater than it should be, then decrease the
     * width proportionately.
     */
    if (car > ar) {
	diff = car / (float) ar;
	max_x = wide / diff + 4;
	justify_text((WINDOW *) 0, prompt, max_y, max_x, &high, &wide);
	car = (float) wide / high;
    }

    /*
     * If the aspect ratio is too small after decreasing the width, then
     * incrementally increase the width until the aspect ratio is equal to or
     * greater than the specified aspect ratio.
     */
    while (car < ar && max_x < max_width) {
	max_x += 4;
	justify_text((WINDOW *) 0, prompt, max_y, max_x, &high, &wide);
	car = (float) (wide / high);
    }

    *height = high;
    *width = wide;
}

/*
 * if (height or width == -1) Maximize()
 * if (height or width == 0), justify and return actual limits.
 */
static void
real_auto_size(const char *title,
	       const char *prompt,
	       int *height, int *width,
	       int boxlines, int mincols)
{
    int x = (dialog_vars.begin_set ? dialog_vars.begin_x : 2);
    int y = (dialog_vars.begin_set ? dialog_vars.begin_y : 1);
    int title_length = title ? strlen(title) : 0;
    int nc = 4;
    int high;
    int wide;
    int save_high = *height;
    int save_wide = *width;

    if (prompt == 0) {
	if (*height == 0)
	    *height = -1;
	if (*width == 0)
	    *width = -1;
    }

    if (*height > 0) {
	high = *height;
    } else {
	high = SLINES - y;
    }

    if (*width > 0) {
	wide = *width;
    } else if (prompt != 0) {
	wide = MAX(title_length, mincols);
	if (strchr(prompt, '\n') == 0) {
	    double val = dialog_vars.aspect_ratio * strlen(prompt);
	    int tmp = sqrt(val);
	    wide = MAX(wide, tmp);
	    justify_text((WINDOW *) 0, prompt, high, wide, height, width);
	} else {
	    auto_size_preformatted(prompt, height, width);
	}
    } else {
	wide = SCOLS - x;
	justify_text((WINDOW *) 0, prompt, high, wide, height, width);
    }

    if (*width < title_length) {
	justify_text((WINDOW *) 0, prompt, high, title_length, height, width);
	*width = title_length;
    }

    if (*width < mincols && save_wide == 0)
	*width = mincols;
    if (prompt != 0) {
	*width += nc;
	*height += boxlines + 2;
    }
    if (save_high > 0)
	*height = save_high;
    if (save_wide > 0)
	*width = save_wide;
}

/* End of real_auto_size() */

void
auto_size(const char *title,
	  const char *prompt,
	  int *height,
	  int *width,
	  int boxlines,
	  int mincols)
{
    real_auto_size(title, prompt, height, width, boxlines, mincols);

    if (*width > SCOLS) {
	(*height)++;
	*width = SCOLS;
    }

    if (*height > SLINES)
	*height = SLINES;
}

/*
 * if (height or width == -1) Maximize()
 * if (height or width == 0)
 *    height=MIN(SLINES, num.lines in fd+n);
 *    width=MIN(SCOLS, MAX(longer line+n, mincols));
 */
void
auto_sizefile(const char *title, const char *file, int *height, int *width, int
	      boxlines, int mincols)
{
    int count = 0, len = title ? strlen(title) : 0, nc = 4, numlines = 2;
    long offset;
    char ch;
    FILE *fd;

    /* Open input file for reading */
    if ((fd = fopen(file, "rb")) == NULL)
	exiterr("auto_sizefile: Cannot open input file %s", file);

    if ((*height == -1) || (*width == -1)) {
	*height = SLINES - (dialog_vars.begin_set ? dialog_vars.begin_y : 0);
	*width = SCOLS - (dialog_vars.begin_set ? dialog_vars.begin_x : 0);
    }
    if ((*height != 0) && (*width != 0)) {
	(void) fclose(fd);
	return;
    }

    while (!feof(fd)) {
	offset = 0;
	while (((ch = getc(fd)) != '\n') && !feof(fd))
	    if ((ch == TAB) && (dialog_vars.tab_correct))
		offset += dialog_vars.tab_len - (offset % dialog_vars.tab_len);
	    else
		offset++;

	if (offset > len)
	    len = offset;

	count++;
    }

    /* now 'count' has the number of lines of fd and 'len' the max lenght */

    *height = MIN(SLINES, count + numlines + boxlines);
    *width = MIN(SCOLS, MAX((len + nc), mincols));
    /* here width and height can be maximized if > SCOLS|SLINES because
       textbox-like widgets don't put all <file> on the screen.
       Msgbox-like widget instead have to put all <text> correctly. */

    (void) fclose(fd);
}

/* End of auto_sizefile() */

/*
 * Draw a rectangular box with line drawing characters.
 *
 * borderchar is used to color the upper/left edges.
 *
 * boxchar is used to color the right/lower edges.  It also is fill-color used
 * for the box contents.
 *
 * Normally, if you are drawing a scrollable box, use menubox_border_attr for
 * boxchar, and menubox_attr for borderchar since the scroll-arrows are drawn
 * with menubox_attr at the top, and menubox_border_attr at the bottom.  That
 * also (given the default color choices) produces a recessed effect.
 *
 * If you want a raised effect (and are not going to use the scroll-arrows),
 * reverse this choice.
 */
void
draw_box(WINDOW *win, int y, int x, int height, int width,
	 chtype boxchar, chtype borderchar)
{
    int i, j;
    chtype save = getattrs(win);

    wattrset(win, 0);
    for (i = 0; i < height; i++) {
	(void) wmove(win, y + i, x);
	for (j = 0; j < width; j++)
	    if (!i && !j)
		(void) waddch(win, borderchar | ACS_ULCORNER);
	    else if (i == height - 1 && !j)
		(void) waddch(win, borderchar | ACS_LLCORNER);
	    else if (!i && j == width - 1)
		(void) waddch(win, boxchar | ACS_URCORNER);
	    else if (i == height - 1 && j == width - 1)
		(void) waddch(win, boxchar | ACS_LRCORNER);
	    else if (!i)
		(void) waddch(win, borderchar | ACS_HLINE);
	    else if (i == height - 1)
		(void) waddch(win, boxchar | ACS_HLINE);
	    else if (!j)
		(void) waddch(win, borderchar | ACS_VLINE);
	    else if (j == width - 1)
		(void) waddch(win, boxchar | ACS_VLINE);
	    else
		(void) waddch(win, boxchar | ' ');
    }
    wattrset(win, save);
}

#ifdef HAVE_COLOR
/*
 * Draw shadows along the right and bottom edge to give a more 3D look
 * to the boxes
 */
void
draw_shadow(WINDOW *win, int y, int x, int height, int width)
{
    int i, j;

    if (has_colors()) {		/* Whether terminal supports color? */
	wattrset(win, shadow_attr);
	for (i = y + height; i < y + height + SHADOW_ROWS; ++i) {
	    (void) wmove(win, i, x + SHADOW_COLS);
	    for (j = 0; j < width; ++j)
		(void) waddch(win, CharOf(winch(win)));
	}
	for (i = y + SHADOW_ROWS; i < y + height + SHADOW_ROWS; i++) {
	    (void) wmove(win, i, x + width);
	    for (j = 0; j < SHADOW_COLS; ++j)
		(void) waddch(win, CharOf(winch(win)));
	}
	(void) wnoutrefresh(win);
    }
}
#endif

/*
 * Allow shell scripts to remap the exit codes so they can distinguish ESC
 * from ERROR.
 */
void
dlg_exit(int code)
{
    /* *INDENT-OFF* */
    static const struct {
	int code;
	const char *name;
    } table[] = {
	{ DLG_EXIT_CANCEL, "DIALOG_CANCEL" },
	{ DLG_EXIT_ERROR,  "DIALOG_ERROR" },
	{ DLG_EXIT_ESC,	   "DIALOG_ESC" },
	{ DLG_EXIT_EXTRA,  "DIALOG_EXTRA" },
	{ DLG_EXIT_HELP,   "DIALOG_HELP" },
	{ DLG_EXIT_OK,	   "DIALOG_OK" },
    };
    /* *INDENT-ON* */

    unsigned n;
    char *name;
    char *temp;
    long value;

    for (n = 0; n < sizeof(table) / sizeof(table[0]); n++) {
	if (table[n].code == code) {
	    if ((name = getenv(table[n].name)) != 0) {
		value = strtol(name, &temp, 0);
		if (temp != 0 && temp != name && *temp == '\0')
		    code = value;
	    }
	    break;
	}
    }
    exit(code);
}

/* exiterr quit program killing all tailbg */
void
exiterr(const char *fmt,...)
{
    int retval;
    va_list ap;

    end_dialog();

    (void) fputc('\n', stderr);
    va_start(ap, fmt);
    (void) vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void) fputc('\n', stderr);

    killall_bg(&retval);

    (void) fflush(stderr);
    (void) fflush(stdout);
    dlg_exit(DLG_EXIT_ERROR);
}

void
beeping(void)
{
    if (dialog_vars.beep_signal) {
	(void) beep();
	dialog_vars.beep_signal = 0;
    }
}

void
print_size(int height, int width)
{
    if (dialog_vars.print_siz)
	fprintf(dialog_vars.output, "Size: %d, %d\n", height, width);
}

void
ctl_size(int height, int width)
{
    if (dialog_vars.size_err) {
	if ((width > COLS) || (height > LINES)) {
	    exiterr("Window too big. (height, width) = (%d, %d). Max allowed (%d, %d).",
		    height, width, LINES, COLS);
	}
#ifdef HAVE_COLOR
	else if ((use_shadow) && ((width > SCOLS || height > SLINES))) {
	    exiterr("Window+Shadow too big. (height, width) = (%d, %d). Max allowed (%d, %d).",
		    height, width, SLINES, SCOLS);
	}
#endif
    }
}

void
tab_correct_str(char *prompt)
{
    char *ptr;

    if (!dialog_vars.tab_correct)
	return;

    while ((ptr = strchr(prompt, TAB)) != NULL) {
	*ptr = ' ';
	prompt = ptr;
    }
}

void
calc_listh(int *height, int *list_height, int item_no)
{
    /* calculate new height and list_height */
    int rows = SLINES - (dialog_vars.begin_set ? dialog_vars.begin_y : 0);
    if (rows - (*height) > 0) {
	if (rows - (*height) > item_no)
	    *list_height = item_no;
	else
	    *list_height = rows - (*height);
    }
    (*height) += (*list_height);
}

int
calc_listw(int item_no, char **items, int group)
{
    int n, i, len1 = 0, len2 = 0;
    for (i = 0; i < (item_no * group); i += group) {
	if ((n = strlen(items[i])) > len1)
	    len1 = n;
	if ((n = strlen(items[i + 1])) > len2)
	    len2 = n;
    }
    return len1 + len2;
}

char *
strclone(const char *cprompt)
{
    char *prompt = (char *) malloc(strlen(cprompt) + 1);
    assert_ptr(prompt, "strclone");
    strcpy(prompt, cprompt);
    return prompt;
}

int
box_x_ordinate(int width)
{
    int x;

    if (dialog_vars.begin_set == 1) {
	x = dialog_vars.begin_x;
    } else {
	/* center dialog box on screen unless --begin-set */
	x = (SCOLS - width) / 2;
    }
    return x;
}

int
box_y_ordinate(int height)
{
    int y;

    if (dialog_vars.begin_set == 1) {
	y = dialog_vars.begin_y;
    } else {
	/* center dialog box on screen unless --begin-set */
	y = (SLINES - height) / 2;
    }
    return y;
}

void
draw_title(WINDOW *win, const char *title)
{
    if (title != NULL) {
	chtype attr = A_NORMAL;
	chtype save = getattrs(win);
	int x = centered(getmaxx(win), title);

	wattrset(win, title_attr);
	wmove(win, 0, x);
	dlg_print_text(win, title, getmaxx(win) - x, &attr);
	wattrset(win, save);
    }
}

void
draw_bottom_box(WINDOW *win)
{
    int width = getmaxx(win);
    int height = getmaxy(win);
    int i;

    wattrset(win, border_attr);
    (void) wmove(win, height - 3, 0);
    (void) waddch(win, ACS_LTEE);
    for (i = 0; i < width - 2; i++)
	(void) waddch(win, ACS_HLINE);
    wattrset(win, dialog_attr);
    (void) waddch(win, ACS_RTEE);
    (void) wmove(win, height - 2, 1);
    for (i = 0; i < width - 2; i++)
	(void) waddch(win, ' ');
}

/*
 * Remove a window, repainting everything else.  This would be simpler if we
 * used the panel library, but that is not _always_ available.
 */
void
del_window(WINDOW *win)
{
    DIALOG_WINDOWS *p, *q, *r;

    /* Leave the main window untouched if there are no background windows.
     * We do this so the current window will not be cleared on exit, allowing
     * things like the infobox demo to run without flicker.
     */
    if (dialog_state.getc_callbacks != 0) {
	touchwin(stdscr);
	wnoutrefresh(stdscr);
    }

    for (p = dialog_state.all_windows, r = 0; p != 0; r = p, p = q) {
	q = p->next;
	if (p->normal == win) {
	    if (p->shadow != 0)
		delwin(p->shadow);
	    delwin(p->normal);
	    if (r == 0) {
		dialog_state.all_windows = q;
	    } else {
		r->next = q;
	    }
	    free(p);
	} else {
	    if (p->shadow != 0) {
		touchwin(p->shadow);
		wnoutrefresh(p->shadow);
	    }
	    touchwin(p->normal);
	    wnoutrefresh(p->normal);
	}
    }
    doupdate();
}

/*
 * Create a window, optionally with a shadow.
 */
WINDOW *
new_window(int height, int width, int y, int x)
{
    WINDOW *win;
    DIALOG_WINDOWS *p = (DIALOG_WINDOWS *) calloc(1, sizeof(DIALOG_WINDOWS));

#ifdef HAVE_COLOR
    if (use_shadow) {
	if ((win = newwin(height, width, y + 1, x + 2)) != 0) {
	    draw_shadow(win, 0, 0, height, width);
	}
	p->shadow = win;
    }
#endif
    if ((win = newwin(height, width, y, x)) == 0) {
	exiterr("Can't make new window at (%d,%d), size (%d,%d).\n",
		y, x, height, width);
    }
    p->next = dialog_state.all_windows;
    p->normal = win;
    dialog_state.all_windows = p;

    (void) keypad(win, TRUE);
    return win;
}

WINDOW *
sub_window(WINDOW *parent, int height, int width, int y, int x)
{
    WINDOW *win;

    if ((win = subwin(parent, height, width, y, x)) == 0) {
	exiterr("Can't make sub-window at (%d,%d), size (%d,%d).\n",
		y, x, height, width);
    }

    (void) keypad(win, TRUE);
    return win;
}

int
dlg_default_item(char **items, int llen)
{
    if (dialog_vars.default_item != 0) {
	int count = 0;
	while (*items != 0) {
	    if (!strcmp(dialog_vars.default_item, *items))
		return count;
	    items += llen;
	    count++;
	}
    }
    return 0;
}

/*
 * Draw the string for item_help
 */
void
dlg_item_help(char *txt)
{
    if (USE_ITEM_HELP(txt)) {
	chtype attr = A_NORMAL;

	wattrset(stdscr, itemhelp_attr);
	(void) wmove(stdscr, LINES - 1, 0);
	(void) wclrtoeol(stdscr);
	(void) addch(' ');
	dlg_print_text(stdscr, txt, COLS - 2, &attr);
	(void) wnoutrefresh(stdscr);
    }
}

#ifndef HAVE_STRCASECMP
int
dlg_strcmp(const char *a, const char *b)
{
    int ac, bc, cmp;

    for (;;) {
	ac = UCH(*a++);
	bc = UCH(*b++);
	if (isalpha(ac) && islower(ac))
	    ac = _toupper(ac);
	if (isalpha(bc) && islower(bc))
	    bc = _toupper(bc);
	cmp = ac - bc;
	if (ac == 0 || bc == 0 || cmp != 0)
	    break;
    }
    return cmp;
}
#endif

/*
 * Returns true if 'dst' points to a blank which follows another blank which
 * is not a leading blank on a line.
 */
static bool
trim_blank(char *base, char *dst)
{
    int count = 0;

    while (dst-- != base) {
	if (*dst == '\n') {
	    return FALSE;
	} else if (*dst != ' ') {
	    return (count > 1);
	} else {
	    count++;
	}
    }
    return FALSE;
}

/*
 * Change embedded "\n" substrings to '\n' characters and tabs to single
 * spaces.  If there are no "\n"s, it will strip all extra spaces, for
 * justification.  If it has "\n"'s, it will preserve extra spaces.  If cr_wrap
 * is set, it will preserve '\n's.
 */
void
dlg_trim_string(char *s)
{
    char *base = s;
    char *p1;
    char *p = s;
    int has_newlines = (strstr(s, "\\n") != 0);

    while (*p != '\0') {
	if (*p == '\t' && !dialog_vars.nocollapse)
	    *p = ' ';

	if (has_newlines) {	/* If prompt contains "\n" strings */
	    if (*p == '\\' && *(p + 1) == 'n') {
		*s++ = '\n';
		p += 2;
		p1 = p;
		/*
		 * Handle end of lines intelligently.  If '\n' follows "\n"
		 * then ignore the '\n'.  This eliminates the need to escape
		 * the '\n' character (no need to use "\n\").
		 */
		while (*p1 == ' ')
		    p1++;
		if (*p1 == '\n')
		    p = p1 + 1;
	    } else if (*p == '\n') {
		if (dialog_vars.cr_wrap)
		    *s++ = *p++;
		else {
		    /* Replace the '\n' with a space if cr_wrap is not set */
		    if (!trim_blank(base, s))
			*s++ = ' ';
		    p++;
		}
	    } else		/* If *p != '\n' */
		*s++ = *p++;
	} else if (dialog_vars.trim_whitespace) {
	    if (*p == ' ') {
		if (*(s - 1) != ' ') {
		    *s++ = ' ';
		    p++;
		} else
		    p++;
	    } else if (*p == '\n') {
		if (dialog_vars.cr_wrap)
		    *s++ = *p++;
		else if (*(s - 1) != ' ') {
		    /* Strip '\n's if cr_wrap is not set. */
		    *s++ = ' ';
		    p++;
		} else
		    p++;
	    } else
		*s++ = *p++;
	} else {		/* If there are no "\n" strings */
	    if (*p == ' ' && !dialog_vars.nocollapse) {
		if (!trim_blank(base, s))
		    *s++ = *p;
		p++;
	    } else
		*s++ = *p++;
	}
    }

    *s = '\0';
}

void
dlg_set_focus(WINDOW *parent, WINDOW *win)
{
    if (win != 0) {
	(void) wmove(parent,
		     getpary(win) + getcury(win),
		     getparx(win) + getcurx(win));
	(void) wnoutrefresh(win);
	(void) doupdate();
    }
}

/*
 * Most of the time we can copy into a fixed buffer.  This handles the other
 * cases, e.g., checklist.c
 */
void
dlg_add_result(char *string)
{
    unsigned have = strlen(dialog_vars.input_result);
    unsigned want = strlen(string) + have;

    if (want >= MAX_LEN) {
	if (dialog_vars.input_length == 0) {
	    char *save = dialog_vars.input_result;
	    dialog_vars.input_length = want * 2;
	    dialog_vars.input_result = malloc(dialog_vars.input_length);
	    assert_ptr(dialog_vars.input_result, "dlg_add_result malloc");
	    dialog_vars.input_result[0] = 0;
	    if (save != 0)
		strcpy(dialog_vars.input_result, save);
	} else if (want >= dialog_vars.input_length) {
	    dialog_vars.input_length = want * 2;
	    dialog_vars.input_result = realloc(dialog_vars.input_result,
					       dialog_vars.input_length);
	    assert_ptr(dialog_vars.input_result, "dlg_add_result realloc");
	}
    }
    strcat(dialog_vars.input_result, string);
}

/*
 * Called each time a widget is invoked which may do output, increment a count.
 */
void
dlg_does_output(void)
{
    dialog_vars.output_count += 1;
}
