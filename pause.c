/*
 *  $Id: pause.c,v 1.3 2004/12/20 23:42:34 tom Exp $
 *
 *  pause.c -- implements the pause dialog
 *
 *  AUTHOR: Yura Kalinichenko
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
 * Display a gauge, or progress meter.  Starts at percent% and reads stdin.  If
 * stdin is not XXX, then it is interpreted as a percentage, and the display is
 * updated accordingly.  Otherwise the next line is the percentage, and
 * subsequent lines up to another XXX are used for the new prompt.  Note that
 * the size of the window never changes, so the prompt can not get any larger
 * than the height and width specified.
 */
int
dialog_pause(const char *title,
	     const char *prompt,
	     int height,
	     int width,
	     int seconds)
{
    int i, x, y;
    int seconds_orig;
    WINDOW *dialog;
    const char **buttons = dlg_exit_label();
    int key = 0, fkey;
    int status = DLG_EXIT_OK;

    seconds_orig = (seconds > 0) ? seconds : 1;
    dlg_auto_size(title, prompt, &height, &width, 0, 0);
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    /* center dialog box on screen */
    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);
    dlg_mouse_setbase(x, y);
    nodelay(dialog, TRUE);

    curs_set(0);
    do {
	(void) werase(dialog);
	dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

	dlg_draw_title(dialog, title);

	wattrset(dialog, dialog_attr);
	dlg_print_autowrap(dialog, prompt, height, width - (2 * MARGIN));

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
	napms(1000);
	key = dlg_mouse_wgetch_nowait(dialog, &fkey);
	if (key != ERR) {
	    status = DLG_EXIT_CANCEL;
	    break;
	}
    } while (seconds-- > 0);

    curs_set(1);
    dlg_del_window(dialog);
    return (status);
}
