/*
 *  $Id: msgbox.c,v 1.38 2004/11/18 21:57:00 tom Exp $
 *
 *  msgbox.c -- implements the message box and info box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *     and: Thomas E. Dickey
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

/*
 * Display the message in a scrollable window.  Actually the way it works is
 * that we create a "tall" window of the proper width, let the text wrap within
 * that, and copy a slice of the result to the dialog.
 *
 * It works for ncurses.  Other curses implementations show only blanks (Tru64)
 * or garbage (NetBSD).
 */
static int
show_message(WINDOW *dialog,
	     const char *prompt,
	     int offset,
	     int page,
	     int width,
	     int pauseopt)
{
#ifdef NCURSES_VERSION
    if (pauseopt) {
	int wide = width - (2 * MARGIN);
	int high = LINES;
	int y, x;
	int len;
	int percent;
	WINDOW *dummy;
	char buffer[5];

#if defined(NCURSES_VERSION_PATCH) && NCURSES_VERSION_PATCH >= 20040417
	/*
	 * If we're not limited by the screensize, allow text to possibly be
	 * one character per line.
	 */
	if ((len = strlen(prompt)) > high)
	    high = len;
#endif
	dummy = newwin(high, width, 0, 0);
	wbkgdset(dummy, dialog_attr | ' ');
	wattrset(dummy, dialog_attr);
	werase(dummy);
	dlg_print_autowrap(dummy, prompt, high, wide);
	getyx(dummy, y, x);

	copywin(dummy,		/* srcwin */
		dialog,		/* dstwin */
		offset + MARGIN,	/* sminrow */
		MARGIN,		/* smincol */
		MARGIN,		/* dminrow */
		MARGIN,		/* dmincol */
		page,		/* dmaxrow */
		wide,		/* dmaxcol */
		FALSE);

	delwin(dummy);

	/* if the text is incomplete, or we have scrolled, show the percentage */
	if (y > 0 && wide > 4) {
	    percent = ((page + offset) * 100.0 / y);
	    if (percent < 0)
		percent = 0;
	    if (percent > 100)
		percent = 100;
	    if (offset != 0 || percent != 100) {
		(void) wattrset(dialog, position_indicator_attr);
		(void) wmove(dialog, MARGIN + page, wide - 4);
		(void) sprintf(buffer, "%d%%", percent);
		(void) waddstr(dialog, buffer);
		if ((len = strlen(buffer)) < 4) {
		    wattrset(dialog, border_attr);
		    whline(dialog, ACS_HLINE, 4 - len);
		}
	    }
	}
	return (y - page);
    }
#endif
    (void) offset;
    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, page + 1 + (3 * MARGIN), width);
    return 0;
}

/*
 * Display a message box. Program will pause and display an "OK" button
 * if the parameter 'pauseopt' is non-zero.
 */
int
dialog_msgbox(const char *title, const char *cprompt, int height, int width,
	      int pauseopt)
{
    int x, y, last = 0, page;
    int key = 0, fkey;
    WINDOW *dialog = 0;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_ok_label();
    int offset = 0;
    bool show = TRUE;

#ifdef KEY_RESIZE
    int req_high = height;
    int req_wide = width;
  restart:
#endif

    dlg_tab_correct_str(prompt);
    dlg_auto_size(title, prompt, &height, &width,
		  (pauseopt == 1 ? 2 : 0),
		  (pauseopt == 1 ? 12 : 0));
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

#ifdef KEY_RESIZE
    if (dialog != 0) {
	(void) wresize(dialog, height, width);
	(void) mvwin(dialog, y, x);
	(void) refresh();
    } else
#endif
	dialog = dlg_new_window(height, width, y, x);
    page = height - (1 + 3 * MARGIN);

    dlg_mouse_setbase(x, y);

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);

    if (pauseopt) {
	bool done = FALSE;

	dlg_draw_bottom_box(dialog);
	mouse_mkbutton(height - 2, width / 2 - 4, 6, '\n');
	dlg_draw_buttons(dialog, height - 2, 0, buttons, FALSE, FALSE, width);

	while (!done) {
	    if (show) {
		getyx(dialog, y, x);
		last = show_message(dialog, prompt, offset, page, width, pauseopt);
		wmove(dialog, y, x);
		wrefresh(dialog);
		show = FALSE;
	    }
	    key = dlg_mouse_wgetch(dialog, &fkey);
	    if (!fkey) {
		fkey = TRUE;
		switch (key) {
		case ESC:
		    done = TRUE;
		    continue;
		case ' ':
		case '\n':
		case '\r':
		    key = KEY_ENTER;
		    break;
		case 'F':
		case 'f':
		    key = KEY_NPAGE;
		    break;
		case 'B':
		case 'b':
		    key = KEY_PPAGE;
		    break;
		case 'g':
		    key = KEY_HOME;
		    break;
		case 'G':
		    key = KEY_END;
		    break;
		case 'K':
		case 'k':
		    key = KEY_UP;
		    break;
		case 'J':
		case 'j':
		    key = KEY_DOWN;
		    break;
		default:
		    if (dlg_char_to_button(key, buttons) == 0) {
			key = KEY_ENTER;
		    } else {
			fkey = FALSE;
		    }
		    break;
		}
	    }
	    if (fkey) {
		switch (key) {
#ifdef KEY_RESIZE
		case KEY_RESIZE:
		    dlg_clear();
		    height = req_high;
		    width = req_wide;
		    goto restart;
#endif
		case KEY_ENTER:
		    done = TRUE;
		    break;
		case KEY_HOME:
		    if (offset > 0) {
			offset = 0;
			show = TRUE;
		    }
		    break;
		case KEY_END:
		    if (offset < last) {
			offset = last;
			show = TRUE;
		    }
		    break;
		case KEY_UP:
		    if (offset > 0) {
			--offset;
			show = TRUE;
		    }
		    break;
		case KEY_DOWN:
		    if (offset < last) {
			++offset;
			show = TRUE;
		    }
		    break;
		case KEY_PPAGE:
		    if (offset > 0) {
			offset -= page;
			if (offset < 0)
			    offset = 0;
			show = TRUE;
		    }
		    break;
		case KEY_NPAGE:
		    if (offset < last) {
			offset += page;
			if (offset > last)
			    offset = last;
			show = TRUE;
		    }
		    break;
		default:
		    if (key >= M_EVENT)
			done = TRUE;
		    else
			beep();
		    break;
		}
	    } else {
		beep();
	    }
	}
    } else {
	show_message(dialog, prompt, offset, page, width, pauseopt);
	key = '\n';
	wrefresh(dialog);
    }

    dlg_del_window(dialog);
    dlg_mouse_free_regions();
    free(prompt);
    return (key == ESC) ? DLG_EXIT_ESC : DLG_EXIT_OK;
}
