/*
 *  yesno.c -- implements the yes/no box
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

/*
 * Display a dialog box with two buttons - Yes and No
 */
int
dialog_yesno (const char *title, const char *cprompt, int height, int width, int defaultno)
{
    int x, y, key = 0, button = defaultno;
    WINDOW *dialog = 0;
    char *prompt=strclone(cprompt);

#ifdef KEY_RESIZE
    int req_high = height;
    int req_wide = width;
restart:
#endif

    tab_correct_str(prompt);
    prompt=auto_size(title, prompt, &height, &width, 2, 25);
    print_size(height, width);
    ctl_size(height, width);
  
    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

#ifdef KEY_RESIZE
    if (dialog != 0) {
	wresize(dialog, height, width);
	mvwin(dialog, y, x);
	refresh();
    } else
#endif
    dialog = new_window (height, width, y, x);

    mouse_setbase (x, y);

    draw_box (dialog, 0, 0, height, width, dialog_attr, border_attr);
    draw_bottom_box (dialog);
    draw_title(dialog, title);

    wattrset (dialog, dialog_attr);
    print_autowrap (dialog, prompt, width, 1, 2);

    x = width / 2 - 10;
    y = height - 2;
    if (! defaultno)
    	print_button (dialog, "  No  ", y, x + 13, FALSE);
    print_button (dialog, " Yes ", y, x, !defaultno);
    if (defaultno)
    	print_button (dialog, "  No  ", y, x + 13, TRUE);
    wrefresh_lock (dialog);

#ifndef KEY_RESIZE
    wtimeout(dialog, WTIMEOUT_VAL);
#endif

    while (key != ESC) {
	key = mouse_wgetch (dialog);
	switch (key) {
	case 'Y':
	case 'y':
	    delwin (dialog);
	    return 0;
	case 'N':
	case 'n':
	    delwin (dialog);
	    return 1;

	case M_EVENT + 'y':	/* mouse enter... */
	case M_EVENT + 'n':	/* use the code for toggling */
	    button = (key == M_EVENT + 'y');

	case TAB:
	case KEY_UP:
	case KEY_DOWN:
	case KEY_LEFT:
	case KEY_RIGHT:
	    if (!button) {
		button = 1;	/* "No" button selected */
		print_button (dialog, " Yes ", y, x, FALSE);
		print_button (dialog, "  No  ", y, x + 13, TRUE);
	    } else {
		button = 0;	/* "Yes" button selected */
		print_button (dialog, "  No  ", y, x + 13, FALSE);
		print_button (dialog, " Yes ", y, x, TRUE);
	    }
	    wrefresh_lock (dialog);
	    break;
	case ' ':
	case '\n':
	    delwin (dialog);
	    return button;
	case ESC:
	    break;
#ifdef KEY_RESIZE
	case KEY_RESIZE:
	    attr_clear (stdscr, LINES, COLS, screen_attr);
	    height = req_high;
	    width  = req_wide;
	    goto restart;
#endif
	}
    }

    delwin (dialog);
    return -1;			/* ESC pressed */
}
