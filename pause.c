/*
 *  $Id: pause.c,v 1.5 2005/09/11 23:49:02 tom Exp $
 *
 *  pause.c -- implements the pause dialog
 *
 *  AUTHOR: Yura Kalinichenko
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

#define MY_TIMEOUT 50

/*
 * This is like gauge, but can be interrupted.
 *
 * A pause box displays a meter along the bottom of the box.  The meter
 * indicates how many seconds remain until the end of the pause.  The pause
 * exits when timeout is reached (status OK) or the user presses the Exit
 * button (status CANCEL).
 */
int
dialog_pause(const char *title,
	     const char *prompt,
	     int height,
	     int width,
	     int seconds)
{
#ifdef KEY_RESIZE
    int old_height = height;
    int old_width = width;
#endif

    int i, x, y, step;
    int seconds_orig;
    WINDOW *dialog;
    const char **buttons = dlg_exit_label();
    int key = 0, fkey;
    int status = DLG_EXIT_OK;

    curs_set(0);

    seconds_orig = (seconds > 0) ? seconds : 1;

#ifdef KEY_RESIZE
  retry:
    height = old_height;
    width = old_width;
#endif

    dlg_auto_size(title, prompt, &height, &width, 0, 0);
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    /* center dialog box on screen */
    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);
    dlg_mouse_setbase(x, y);
    nodelay(dialog, TRUE);

    do {
	(void) werase(dialog);
	dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

	dlg_draw_title(dialog, title);

	wattrset(dialog, dialog_attr);
	dlg_print_autowrap(dialog, prompt, height, width);

	dlg_draw_box(dialog,
		     height - 6, 2 + MARGIN,
		     2 + MARGIN, width - 2 * (2 + MARGIN),
		     dialog_attr,
		     border_attr);

	/*
	 * Clear the area for the progress bar by filling it with spaces
	 * in the title-attribute, and write the percentage with that
	 * attribute.
	 */
	(void) wmove(dialog, height - 5, 4);
	wattrset(dialog, title_attr);

	for (i = 0; i < (width - 2 * (3 + MARGIN)); i++)
	    (void) waddch(dialog, ' ');

	(void) wmove(dialog, height - 5, (width / 2) - 2);
	(void) wprintw(dialog, "%3d", seconds);

	/*
	 * Now draw a bar in reverse, relative to the background.
	 * The window attribute was useful for painting the background,
	 * but requires some tweaks to reverse it.
	 */
	x = (seconds * (width - 2 * (3 + MARGIN))) / seconds_orig;
	if ((title_attr & A_REVERSE) != 0) {
	    wattroff(dialog, A_REVERSE);
	} else {
	    wattrset(dialog, A_REVERSE);
	}
	(void) wmove(dialog, height - 5, 4);
	for (i = 0; i < x; i++) {
	    chtype ch = winch(dialog);
	    if (title_attr & A_REVERSE) {
		ch &= ~A_REVERSE;
	    }
	    (void) waddch(dialog, ch);
	}

	dlg_draw_bottom_box(dialog);
	mouse_mkbutton(height - 2, width / 2 - 4, 6, '\n');
	dlg_draw_buttons(dialog, height - 2, 0, buttons, FALSE, FALSE, width);
	(void) wrefresh(dialog);
	for (step = 0; step < 1000; step += MY_TIMEOUT) {
	    napms(MY_TIMEOUT);
	    key = dlg_mouse_wgetch_nowait(dialog, &fkey);
#ifdef KEY_RESIZE
	    if (key == KEY_RESIZE) {
		dlg_clear();	/* fill the background */
		dlg_del_window(dialog);		/* delete this window */
		refresh();	/* get it all onto the terminal */
		goto retry;
	    }
#endif
	    if (key != ERR) {
		status = DLG_EXIT_CANCEL;
		seconds = 0;
		break;
	    }
	}
    } while (seconds-- > 0);

    curs_set(1);
    dlg_del_window(dialog);
    return (status);
}
