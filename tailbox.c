/*
 *  $Id: tailbox.c,v 1.22 2000/10/28 01:03:37 tom Exp $
 *
 *  tailbox.c -- implements the tail box
 *
 *  AUTHOR: Pasquale De Marco (demarco_p@abramo.it)
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

static void last_lines(int n);
static void print_page(WINDOW *win, int height, int width);
static void print_last_page(WINDOW *win, int height, int width);
static void print_line(WINDOW *win, int row, int width);
static char *get_line(void);

static int hscroll = 0, page_length, old_hscroll = 0, end_reached;
static FILE *fd;

int can_quit = 0;

static RETSIGTYPE
signal_tailbg(int sig)
{				/* __sighandler_t */
    if (sig == 15)
	can_quit = 1;
}

/*
 * Display text from a file in a dialog box, like in a "tail -f".
 */
int
dialog_tailbox(const char *title, const char *file, int height, int width)
{
    int x, y, cur_x, cur_y, key = 0;
    WINDOW *dialog, *text;
    int ch;
    const char **buttons = dlg_exit_label();

    auto_sizefile(title, file, &height, &width, 2, 12);
    print_size(height, width);
    ctl_size(height, width);

    /* Open input file for reading */
    if ((fd = fopen(file, "rb")) == NULL)
	exiterr("Can't open input file in dialog_tailbox().");

    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

    dialog = new_window(height, width, y, x);

    mouse_setbase(x, y);

    /* Create window for text region, used for scrolling text */
    text = sub_window(dialog, height - 4, width - 2, y + 1, x + 1);
    (void) keypad(text, TRUE);

    draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    draw_bottom_box(dialog);
    draw_title(dialog, title);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, FALSE, FALSE, width);

    (void) wmove(dialog, height - 4, 2);

    (void) wnoutrefresh(dialog);
    getyx(dialog, cur_y, cur_x);	/* Save cursor position */

    /* Print last page of text */
    attr_clear(text, height - 4, width - 2, dialog_attr);
    print_last_page(text, height - 5, width - 2);
    (void) wmove(dialog, cur_y, cur_x);		/* Restore cursor position */
    wrefresh_lock(dialog);

    /* nodelay(dialog, TRUE);  with this it takes much more cpu time */
    wtimeout(dialog, WTIMEOUT_VAL);

    while (key != ESC) {
	key = dlg_getc(dialog);
	if (dlg_char_to_button(key, buttons) == 0) {
	    key = '\n';
	}
	switch (key) {
	case ERR:
	    ch = getc(fd);
	    (void) ungetc(ch, fd);
	    if ((ch != EOF) || (hscroll != old_hscroll)) {
		old_hscroll = hscroll;
		print_last_page(text, height - 5, width - 2);
		(void) wmove(dialog, cur_y, cur_x);	/* Restore cursor position */
		wrefresh_lock(dialog);
	    }
	    ctl_idlemsg(dialog);
	    break;
	case '\n':
	    (void) delwin(dialog);
	    (void) fclose(fd);
	    return 0;
	case '0':		/* Beginning of line */
	case 'H':		/* Scroll left */
	case 'h':
	case KEY_LEFT:
	    if (hscroll > 0) {
		if (key == '0')
		    hscroll = 0;
		else
		    hscroll--;
	    }
	    break;
	case 'L':		/* Scroll right */
	case 'l':
	case KEY_RIGHT:
	    if (hscroll < MAX_LEN)
		hscroll++;
	    break;
	case ESC:
	    break;
	}
    }

    (void) delwin(dialog);
    (void) fclose(fd);
    return -1;			/* ESC pressed */
}
/* End of dialog_tailbox() */

/*
 * Display text from a file in a dialog box in background, like in a "tail -f &".
 */
void
dialog_tailboxbg(const char *title, const char *file, int height, int width, int cant_kill)
{
    int x, y;
    WINDOW *dialog, *text;
    int ch;

    auto_sizefile(title, file, &height, &width, 1, 0);
    print_size(height, width);
    ctl_size(height, width);

    /* Open input file for reading */
    if ((fd = fopen(file, "rb")) == NULL)
	exiterr("Can't open input file in dialog_tailboxbg().");

    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

    dialog = new_window(height, width, y, x);

    /* Create window for text region, used for scrolling text */
    text = sub_window(dialog, height - 2, width - 2, y + 1, x + 1);
    (void) keypad(text, FALSE);

    draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

    draw_title(dialog, title);

    (void) wmove(dialog, height - 2, 2);

    (void) wnoutrefresh(dialog);

    /* Print last page of text */
    attr_clear(text, height - 2, width - 2, dialog_attr);
    print_last_page(text, height - 3, width - 2);
    wrefresh_lock_tailbg(dialog);

    if (cant_kill)
	(void) signal(15, &signal_tailbg);

    while (!can_quit) {
	ch = getc(fd);
	(void) ungetc(ch, fd);
	if ((ch != EOF) || (hscroll != old_hscroll)) {
	    old_hscroll = hscroll;
	    print_last_page(text, height - 2 - 1, width - 2);
	    wrefresh_lock_tailbg(dialog);
	}
	(void) napms(1000);
    }
}

/* End of dialog_tailboxbg() */

/*
 * Go back 'n' lines in text file. BUF_SIZE has to be in 'size_t' range.
 */
static void
last_lines(int n)
{
    int i = 0;
    static char *ptr, buf[BUF_SIZE + 1];
    size_t size_to_read;
    long fpos;

    if (fseek(fd, 0, SEEK_END) == -1)
	exiterr("Error moving file pointer in last_lines().");

    if ((fpos = ftell(fd)) >= BUF_SIZE) {
	if (fseek(fd, -BUF_SIZE, SEEK_END) == -1)
	    exiterr("Error moving file pointer in last_lines().");

	size_to_read = (size_t) BUF_SIZE;
    } else {
	if (fseek(fd, 0, SEEK_SET) == -1)
	    exiterr("Error moving file pointer in last_lines().");

	size_to_read = ((size_t) fpos);
    }
    (void) fread(buf, size_to_read, 1, fd);
    if (ferror(fd))
	exiterr("Error reading file in last_lines().");

    ptr = buf + size_to_read;	/* here ptr points to (last char of buf)+1 = eof */

    while (i++ <= n)
	while (*(--ptr) != '\n')
	    if (ptr <= buf) {
		if (size_to_read == BUF_SIZE)	/* buffer full */
		    exiterr("Error reading file in last_lines(): Line too long.");
		else {
		    ptr = buf - 1;
		    i = n + 1;
		    break;
		}
	    }
    ptr++;

    if (fseek(fd, fpos - (buf + size_to_read - ptr), SEEK_SET) == -1)
	exiterr("Error moving file pointer in last_lines().");
}
/* End of last_lines() */

/*
 * Print a new page of text. Called by dialog_tailbox().
 */
static void
print_page(WINDOW *win, int height, int width)
{
    int i, passed_end = 0;

    page_length = 0;
    for (i = 0; i < height; i++) {
	print_line(win, i, width);
	if (!passed_end)
	    page_length++;
	if (end_reached && !passed_end)
	    passed_end = 1;
    }
    (void) wnoutrefresh(win);
}

/* End of print_page() */

static void
print_last_page(WINDOW *win, int height, int width)
{
    last_lines(height);
    print_page(win, height, width);
}

/*
 * Print a new line of text. Called by dialog_tailbox() and print_page().
 */
static void
print_line(WINDOW *win, int row, int width)
{
    int i, y, x;
    char *line;

    line = get_line();
    line += MIN((int) strlen(line), hscroll);	/* Scroll horizontally */
    (void) wmove(win, row, 0);	/* move cursor to correct line */
    (void) waddch(win, ' ');
#ifdef NCURSES_VERSION
    (void) waddnstr(win, line, MIN((int) strlen(line), width - 2));
#else
    line[MIN((int) strlen(line), width - 2)] = '\0';
    waddstr(win, line);
#endif

    getyx(win, y, x);
    /* Clear 'residue' of previous line */
    for (i = 0; i < width - x; i++)
	(void) waddch(win, ' ');
}

/* End of print_line() */

/*
 * Return current line of text. Called by dialog_tailbox() and print_line().
 * 'page' should point to start of current line before calling, and will be
 * updated to point to start of next line.
 */
static char *
get_line(void)
{
    int i = 0, j, tmpint, ch;
    static char line[MAX_LEN + 1];

    end_reached = 0;

    do {
	if (((ch = getc(fd)) == EOF) && !feof(fd))
	    exiterr("Error moving file pointer in get_line().");
	else if ((i < MAX_LEN) && !feof(fd) && (ch != '\n')) {
	    if ((ch == TAB) && (dialog_vars.tab_correct)) {
		tmpint = dialog_vars.tab_len - (i % dialog_vars.tab_len);
		for (j = 0; j < tmpint; j++) {
		    if (i < MAX_LEN)
			line[i++] = ' ';
		}
	    } else {
		line[i++] = ch;
	    }
	}
    } while (!feof(fd) && (ch != '\n'));

    line[i++] = '\0';

    if (feof(fd))
	end_reached = 1;

    return line;
}
/* End of get_line() */
