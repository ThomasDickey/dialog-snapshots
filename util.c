/*
 *  $Id: util.c,v 1.48 2000/10/18 01:02:18 tom Exp $
 *
 *  util.c
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
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

#ifdef NCURSES_VERSION
#include <term.h>
#endif

/* globals */
FILE *pipe_fp;			/* copy of stdin, for reading a pipe */
DIALOG_VARS dialog_vars;
char *lock_refresh;
char *lock_tailbg_exit;
char *lock_tailbg_refreshed;
int defaultno = FALSE;
int is_tailbg = FALSE;
int screen_initialized = 0;
pid_t tailbg_lastpid = 0;
pid_t tailbg_nokill_lastpid = 0;
pid_t tailbg_nokill_pids[MAX_TAILBG];
pid_t tailbg_pids[MAX_TAILBG];
static FILE *my_output;		/* prefer to stdout, to support --stdout */

#ifdef HAVE_COLOR
/* use colors by default? */
bool use_colors = USE_COLORS;
/* shadow dialog boxes by default?
   Note that 'use_shadow' implies 'use_colors' */
bool use_shadow = USE_SHADOW;
#endif

const char *dialog_result;

/* 
 * Attribute values, default is for mono display
 */
chtype attributes[] =
{
    A_NORMAL,			/* screen_attr */
    A_NORMAL,			/* shadow_attr */
    A_REVERSE,			/* dialog_attr */
    A_REVERSE,			/* title_attr */
    A_REVERSE,			/* border_attr */
    A_BOLD,			/* button_active_attr */
    A_DIM,			/* button_inactive_attr */
    A_UNDERLINE,		/* button_key_active_attr */
    A_UNDERLINE,		/* button_key_inactive_attr */
    A_NORMAL,			/* button_label_active_attr */
    A_NORMAL,			/* button_label_inactive_attr */
    A_REVERSE,			/* inputbox_attr */
    A_REVERSE,			/* inputbox_border_attr */
    A_REVERSE,			/* searchbox_attr */
    A_REVERSE,			/* searchbox_title_attr */
    A_REVERSE,			/* searchbox_border_attr */
    A_REVERSE,			/* position_indicator_attr */
    A_REVERSE,			/* menubox_attr */
    A_REVERSE,			/* menubox_border_attr */
    A_REVERSE,			/* item_attr */
    A_NORMAL,			/* item_selected_attr */
    A_REVERSE,			/* tag_attr */
    A_REVERSE,			/* tag_selected_attr */
    A_NORMAL,			/* tag_key_attr */
    A_BOLD,			/* tag_key_selected_attr */
    A_REVERSE,			/* check_attr */
    A_REVERSE,			/* check_selected_attr */
    A_REVERSE,			/* uarrow_attr */
    A_REVERSE,			/* darrow_attr */
    A_NORMAL			/* itemhelp_attr */
};

#ifdef HAVE_COLOR
#include "colors.h"

/*
 * Table of color values
 */
#define concat(a,b) a##b
#define DATA(name) { concat(name,_FG), concat(name,_BG), concat(name,_HL) }
int color_table[][3] =
{
    DATA(SCREEN),
    DATA(SHADOW),
    DATA(DIALOG),
    DATA(TITLE),
    DATA(BORDER),
    DATA(BUTTON_ACTIVE),
    DATA(BUTTON_INACTIVE),
    DATA(BUTTON_KEY_ACTIVE),
    DATA(BUTTON_KEY_INACTIVE),
    DATA(BUTTON_LABEL_ACTIVE),
    DATA(BUTTON_LABEL_INACTIVE),
    DATA(INPUTBOX),
    DATA(INPUTBOX_BORDER),
    DATA(SEARCHBOX),
    DATA(SEARCHBOX_TITLE),
    DATA(SEARCHBOX_BORDER),
    DATA(POSITION_INDICATOR),
    DATA(MENUBOX),
    DATA(MENUBOX_BORDER),
    DATA(ITEM),
    DATA(ITEM_SELECTED),
    DATA(TAG),
    DATA(TAG_SELECTED),
    DATA(TAG_KEY),
    DATA(TAG_KEY_SELECTED),
    DATA(CHECK),
    DATA(CHECK_SELECTED),
    DATA(UARROW),
    DATA(DARROW),
    DATA(ITEMHELP),
};				/* color_table */
#endif

/*
 * Display background title if it exists ...
 */
void
put_backtitle(void)
{
    int i;

    if (dialog_vars.backtitle != NULL) {
	wattrset(stdscr, screen_attr);
	wmove(stdscr, 0, 1);
	waddstr(stdscr, dialog_vars.backtitle);
	for (i = 0; i < COLS - (int) strlen(dialog_vars.backtitle); i++)
	    waddch(stdscr, ' ');
	wmove(stdscr, 1, 1);
	for (i = 0; i < COLS - 2; i++)
	    waddch(stdscr, ACS_HLINE);
    }

    wnoutrefresh(stdscr);
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
	wmove(win, i, 0);
	for (j = 0; j < width; j++)
	    waddch(win, ' ');
    }
    touchwin(win);
}

void
dialog_clear(void)
{
    attr_clear(stdscr, LINES, COLS, screen_attr);
}

#define isprivate(s) ((s) != 0 && strstr(s, "\033[?") != 0)

#ifdef NCURSES_VERSION
static int
my_putc(int ch)
{
    return fputc(ch, my_output);
}
#endif

static int
open_terminal(char **result, int mode)
{
    const char *device = "/dev/tty";
    if (!isatty(fileno(stderr))
	|| (device = ttyname(fileno(stderr))) == 0) {
	if (!isatty(fileno(stdout))
	    || (device = ttyname(fileno(stdout))) == 0) {
	    if (!isatty(fileno(stdin))
		|| (device = ttyname(fileno(stdin))) == 0) {
		device = "/dev/tty";
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
    char *name;

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
		dup2(fileno(stdin), 0);
	}
    }

    /*
     * Try to open the output directly to /dev/tty so we can support the
     * command-line option "--stdout".  Otherwise it will get lost in the
     * normal output to the screen.
     */
    if ((fd1 = open_terminal(&device, O_WRONLY)) >= 0
	&& (my_output = fdopen(fd1, "w")) != 0) {
	if (newterm(NULL, my_output, stdin) == 0) {
	    exiterr("cannot initialize curses");
	}
    } else {
	initscr();
    }
#ifdef NCURSES_VERSION
    /*
     * Cancel xterm's alternate-screen mode.
     */
    name = getenv("TERM");
    if (key_mouse != 0		/* xterm and kindred */
	&& isprivate(enter_ca_mode)
	&& isprivate(exit_ca_mode)) {
	tputs(exit_ca_mode, 0, my_putc);
	tputs(clear_screen, 0, my_putc);
    }
#endif
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
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
/*
 * Setup for color display
 */
void
color_setup(void)
{
    int i;

    if (has_colors()) {		/* Terminal supports color? */
	start_color();

	/* Initialize color pairs */
	for (i = 0; i < ATTRIBUTE_COUNT; i++)
	    init_pair(i + 1, color_table[i][0], color_table[i][1]);

	/* Setup color attributes */
	for (i = 0; i < ATTRIBUTE_COUNT; i++)
	    attributes[i] = C_ATTR(color_table[i][2], i + 1);
    }
}
#endif

/*
 * End using dialog functions.
 */
void
end_dialog(void)
{
    mouse_close();
    endwin();
}

static int
centered(int width, const char *string)
{
    int left = (width - strlen(string)) / 2 - 1;
    if (left < 0)
	left = 0;
    return left;
}

/*
 * Print a string of text in a window, automatically wrap around to the
 * next line if the string is too long to fit on one line. Note that the
 * string may contain "\n" to represent a newline character or the real
 * newline '\n', but in that case, auto wrap around will be disabled.
 */
void
print_autowrap(WINDOW *win, const char *prompt, int width, int y, int x)
{
    int first = 1, cur_x, cur_y;
    char tempstr[MAX_LEN + 1], *word, *tempptr1;

    strcpy(tempstr, prompt);
    if (strchr(tempstr, '\n') != NULL) {
	/* Prompt contains "\n" or '\n' */
	word = tempstr;
	cur_y = y;
	if (dialog_vars.cr_wrap)
	    cur_y++;
	wmove(win, cur_y, x);
	while ((tempptr1 = strchr(word, '\n')) != NULL) {
	    *tempptr1++ = 0;
	    waddstr(win, word);
	    word = tempptr1;
	    wmove(win, ++cur_y, x);
	}
	waddstr(win, word);
    } else if ((int) strlen(tempstr) <= width - x * 2) {
	/* If prompt is short */
	cur_y = y;
	if (dialog_vars.cr_wrap)
	    cur_y++;
	mvwprintw(win, cur_y, centered(width, tempstr), "%s", tempstr);
    } else {
	cur_x = x;
	cur_y = y;
	if (dialog_vars.cr_wrap)
	    cur_y++;
	/* Print prompt word by word, wrap around if necessary */
	while ((word = strtok(first ? tempstr : NULL, " ")) != NULL) {
	    if (first)		/* First iteration */
		first = 0;
	    if (cur_x + (int) strlen(word) > width - x) {
		/* wrap around to next line */
		cur_y++;
		cur_x = x;
	    }
	    wmove(win, cur_y, cur_x);
	    waddstr(win, word);
	    getyx(win, cur_y, cur_x);
	    cur_x++;
	}
    }
    if (dialog_vars.cr_wrap)
	wmove(win, cur_y + 1, x);
}

/*
 * if (height or width == -1) Maximize()
 * if (height or width == 0)
 *    if (there are \n in prompt)
 *       width=MAX(longer line+n, mincols)
 *       height=num.lines
 *    else
 *       calculate with aspect_ratio
 */
static char *
real_auto_size(const char *title, char *prompt, int *height, int *width, int
	       boxlines, int mincols)
{
    int count = 0;
    int len = title ? strlen(title) : 0;
    int i;
    int first;
    int nc = 4;
    int numlines = 3;
    int cur_x;
    int text_width;
    char *cr2, *ptr = prompt, *str, *word;

    if (prompt == 0) {
	if (*height == 0)
	    *height = -1;
	if (*width == 0)
	    *width = -1;
    }

    if (*height == -1)
	*height = SLINES - (dialog_vars.begin_set ? dialog_vars.begin_y : 0);
    if (*width == -1)
	*width = SCOLS - (dialog_vars.begin_set ? dialog_vars.begin_x : 0);

    if ((*height != 0) && (*width != 0))
	return prompt;

    while ((cr2 = strchr(ptr, '\n')) != NULL) {
	if ((cr2 - ptr) > len)
	    len = cr2 - ptr;
	ptr = cr2 + 1;
	count++;
    }

    if ((int) strlen(ptr) > len)
	len = strlen(ptr);

    /* now 'count' has the number of lines of text and 'len' the max length */

    if (count > 0) {		/* there are any '\n' */
	*height = count + numlines + boxlines + (dialog_vars.cr_wrap ? 2 : 0);
	*width = MAX((len + nc), mincols);
	return prompt;
    } else {			/* all chars in only a line */
	double val = dialog_vars.aspect_ratio * strlen(prompt);
	int tmp = sqrt(val);
	str = (char *) malloc(strlen(prompt) * 2);
	text_width = MAX(tmp, (mincols - nc));
	text_width = MAX(text_width, len);
	ptr = (char *) malloc(strlen(prompt) + 1);

	while (1) {
	    i = 0;

	    first = 1;
	    cur_x = 0;

	    *width = text_width + nc;

	    count = 0;
	    strcpy(ptr, prompt);
	    while (((word = strtok(first ? ptr : NULL, " ")) != NULL)
		   && ((int) strlen(word) <= text_width)) {
		if (first)
		    first = 0;

		if ((cur_x + (int) strlen(word)) > text_width) {
		    count++;
		    cur_x = 0;
		    str[i++] = '\n';
		} else if (cur_x > 0)
		    str[i++] = ' ';

		strcpy(str + i, word);
		i += strlen(word);

		cur_x += strlen(word) + 1;
	    }

	    if (word == NULL)
		break;
	    if ((int) strlen(word) > text_width)
		text_width = strlen(word);	/* strlen(word) is surely > (mincols-nc) */
	    else
		break;
	}
	str[i] = 0;

	*height = count + numlines + boxlines + (dialog_vars.cr_wrap ? 2 : 0);

	return str;
    }
}
/* End of auto_size() */

char *
auto_size(const char *title, char *prompt, int *height, int *width, int
	  boxlines, int mincols)
{
    char *s = real_auto_size(title, prompt, height, width, boxlines, mincols);

    if (*width > SCOLS) {
	(*height)++;
	*width = SCOLS;
    }

    if (*height > SLINES)
	*height = SLINES;

    return s;
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
	fclose(fd);
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

    fclose(fd);
}
/* End of auto_sizefile() */

/*
 * Draw a rectangular box with line drawing characters
 */
void
draw_box(WINDOW *win, int y, int x, int height, int width,
	 chtype boxchar, chtype borderchar)
{
    int i, j;
    chtype save = getattrs(win);

    wattrset(win, 0);
    for (i = 0; i < height; i++) {
	wmove(win, y + i, x);
	for (j = 0; j < width; j++)
	    if (!i && !j)
		waddch(win, borderchar | ACS_ULCORNER);
	    else if (i == height - 1 && !j)
		waddch(win, borderchar | ACS_LLCORNER);
	    else if (!i && j == width - 1)
		waddch(win, boxchar | ACS_URCORNER);
	    else if (i == height - 1 && j == width - 1)
		waddch(win, boxchar | ACS_LRCORNER);
	    else if (!i)
		waddch(win, borderchar | ACS_HLINE);
	    else if (i == height - 1)
		waddch(win, boxchar | ACS_HLINE);
	    else if (!j)
		waddch(win, borderchar | ACS_VLINE);
	    else if (j == width - 1)
		waddch(win, boxchar | ACS_VLINE);
	    else
		waddch(win, boxchar | ' ');
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
    int i;

    if (has_colors()) {		/* Whether terminal supports color? */
	wattrset(win, shadow_attr);
	wmove(win, y + height, x + 2);
	for (i = 0; i < width; i++)
	    waddch(win, CharOf(winch(win)));
	for (i = y + 1; i < y + height + 1; i++) {
	    wmove(win, i, x + width);
	    waddch(win, CharOf(winch(win)));
	    waddch(win, CharOf(winch(win)));
	}
	wnoutrefresh(win);
    }
}
#endif

/*
 * make_tmpfile()
 */
char *
make_lock_filename(const char *filename)
{
    char *file = (char *) malloc(30);
    strcpy(file, filename);
    if (mktemp(file) == NULL) {
	endwin();
	fprintf(stderr, "\nCan't make tempfile...\n");
	return NULL;
    }
    return file;
}
/* End of make_tmpfile() */

/*
 * Only a process at a time can call wrefresh()
 */

/* called by all normal widget */
void
wrefresh_lock(WINDOW *win)
{
    wrefresh_lock_sub(win);

    if (exist_lock(lock_tailbg_refreshed))
	remove_lock(lock_tailbg_refreshed);

}

/* If a tailboxbg was wrefreshed, wrefresh again the principal (normal) widget.
 *
 * The important thing here was to put cursor in the principal widget, 
 * but with touchwin(win) the principal widget is also rewritten. This 
 * is needed if a tailboxbg has overwritten this widget. 
 */
void
ctl_idlemsg(WINDOW *win)
{
    if (exist_lock(lock_tailbg_refreshed))
	wrefresh_lock(win);

    /* if a tailboxbg exited, quit */
    if (exist_lock(lock_tailbg_exit)) {
	remove_lock(lock_tailbg_exit);
	exiterr("ctl_idlemsg: quit");
    }
}

/* call by tailboxbg, the resident widget */
void
wrefresh_lock_tailbg(WINDOW *win)
{
    wrefresh_lock_sub(win);
    create_lock(lock_tailbg_refreshed);
    /* it's right also if more tailboxbg refresh */
    /* with --no-kill flag the lock_tailbg_refreshed file
       will not be removed :-/ :-| */
}

/* private function, used by wrefresh_lock() and wrefresh_lock_tailbg() */
void
wrefresh_lock_sub(WINDOW *win)
{
    if (lock_refresh) {
	while_exist_lock(lock_refresh);
	create_lock(lock_refresh);
    }
    wrefresh(win);
    beeping();
    wrefresh(win);
    remove_lock(lock_refresh);
}

/* End of wrefresh functions */

/*
 * To work with lock files, here are some functions
 */
int
exist_lock(char *filename)
{
    FILE *fil;
    if ((fil = fopen(filename, "r")) != NULL) {
	fclose(fil);
	return 1;
    }
    return 0;
}

void
while_exist_lock(char *filename)
{
    while (exist_lock(filename)) ;
}

void
create_lock(char *filename)
{
    FILE *fil = fopen(filename, "w");
    fclose(fil);
}

void
remove_lock(char *filename)
{
    remove(filename);
}

/* End of functions to work with lock files */

/* killall_bg: kill all tailboxbg, if any */
void
killall_bg(void)
{
    int i;
    for (i = 0; i < tailbg_lastpid; i++)
	kill(tailbg_pids[i], 15);
}

static int
is_nokill(int pid)
{				/* private func for quitall_bg() */
    int i;
    for (i = 0; i < tailbg_nokill_lastpid; i++)
	if (tailbg_nokill_pids[i] == pid)
	    return 1;
    return 0;
}
/* quitall_bg: kill all tailboxbg without --no-kill flag */
void
quitall_bg(void)
{
    int i;
    for (i = 0; i < tailbg_lastpid; i++)
	if (!is_nokill(tailbg_pids[i]))
	    kill(tailbg_pids[i], 15);
}

/* exiterr quit program killing all tailbg */
void
exiterr(const char *fmt,...)
{
    va_list ap;

    if (screen_initialized)
	endwin();

    fputc('\n', stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);

    if (is_tailbg)		/* this is a child process */
	create_lock(lock_tailbg_exit);
    else
	killall_bg();

    fflush(stderr);
    fflush(stdout);
    exit(-1);
}

void
beeping(void)
{
    if (dialog_vars.beep_signal) {
	beep();
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
	    exiterr("Window too big. (Height, width) = (%d, %d). Max allowed (%d, %d).",
		    height, width, LINES, COLS);
	}
#ifdef HAVE_COLOR
	else if ((use_shadow) && ((width > SCOLS || height > SLINES))) {
	    exiterr("Window+Shadow too big. (Height, width) = (%d, %d). Max allowed (%d, %d).",
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
	wattrset(win, title_attr);
	mvwprintw(win, 0, centered(getmaxx(win), title), " %s ", title);
    }
}

void
draw_bottom_box(WINDOW *win)
{
    int width = getmaxx(win);
    int height = getmaxy(win);
    int i;

    wattrset(win, border_attr);
    wmove(win, height - 3, 0);
    waddch(win, ACS_LTEE);
    for (i = 0; i < width - 2; i++)
	waddch(win, ACS_HLINE);
    wattrset(win, dialog_attr);
    waddch(win, ACS_RTEE);
    wmove(win, height - 2, 1);
    for (i = 0; i < width - 2; i++)
	waddch(win, ' ');
}

WINDOW *
new_window(int height, int width, int y, int x)
{
    WINDOW *win;

#ifdef HAVE_COLOR
    if (use_shadow)
	draw_shadow(stdscr, y, x, height, width);
#endif
    if ((win = newwin(height, width, y, x)) == 0) {
	exiterr("Can't make new window at (%d,%d), size (%d,%d).\n",
		y, x, height, width);
    }

    keypad(win, TRUE);
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

    keypad(win, TRUE);
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

int
dlg_getc(WINDOW *win)
{
    for (;;) {
	int ch = wgetch(win);
	switch (ch) {
	case CHR_REPAINT:
	    touchwin(win);
	    wrefresh(curscr);
	    break;
	default:
	    return ch;
	}
    }
}

/*
 * Draw the string for item_help
 */
void
dlg_item_help(char *txt)
{
    if (dialog_vars.item_help && txt) {
	wattrset(stdscr, itemhelp_attr);
	wmove(stdscr, LINES - 1, 0);
	wclrtoeol(stdscr);
	wprintw(stdscr, " %.*s", COLS - 2, txt);
	wnoutrefresh(stdscr);
    }
}

/*
 * Change embedded "\n" substring to '\n' character, tabs to spaces and
 * repeated spaces to a single space.  Leading/trailing blanks are discarded.
 */
void
dlg_trim_string(char *base)
{
    char *dst = base;
    char *src = dst;
    int ch, ch0 = ' ';

    while ((ch = *src++) != 0) {
	if (ch == '\\' && *src == 'n') {
	    ch = '\n';
	    src++;
	    base = dst + 1;	/* don't trim this one... */
	}
	if (ch == '\t')
	    ch = ' ';
	if (ch != ' ' || ch0 != ' ')
	    *dst++ = ch;
	ch0 = (ch == '\n') ? ' ' : ch;
    }
    if (ch0 == ' ') {
	while (dst - 1 >= base && isspace(dst[-1]))
	    *--dst = 0;
    }
    *dst = 0;
}
