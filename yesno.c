/*
 *  $Id: yesno.c,v 1.32 2004/09/19 23:09:34 tom Exp $
 *
 *  yesno.c -- implements the yes/no box
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
 * Display a dialog box with two buttons - Yes and No
 */
int
dialog_yesno(const char *title, const char *cprompt, int height, int width)
{
    int x, y;
    int key = 0, fkey;
    int button = dlg_defaultno_button();
    WINDOW *dialog = 0;
    int result = DLG_EXIT_UNKNOWN;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_yes_labels();

#ifdef KEY_RESIZE
    int req_high = height;
    int req_wide = width;
  restart:
#endif

    dlg_tab_correct_str(prompt);
    dlg_auto_size(title, prompt, &height, &width, 2, 25);
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

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_bottom_box(dialog);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, height, width);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);

    while (result == DLG_EXIT_UNKNOWN) {
	key = dlg_mouse_wgetch(dialog, &fkey);
	if ((result = dlg_char_to_button(key, buttons)) >= 0) {
	    continue;
	}
	/* handle non-functionkeys */
	if (!fkey) {
	    switch (key) {
	    case ESC:
		result = DLG_EXIT_ESC;
		break;
	    case TAB:
		key = KEY_RIGHT;
		fkey = TRUE;
		break;
	    case ' ':
	    case '\n':
	    case '\r':
		key = KEY_ENTER;
		fkey = TRUE;
		break;
	    }
	}
	/* handle functionkeys */
	if (fkey) {
	    switch (key) {
	    case KEY_BTAB:
	    case KEY_UP:
	    case KEY_DOWN:
	    case KEY_LEFT:
	    case KEY_RIGHT:
		button = !button;
		dlg_draw_buttons(dialog,
				 height - 2, 0,
				 buttons, button,
				 FALSE, width);
		break;
	    case M_EVENT + 0:
	    case M_EVENT + 1:
		button = (key == (M_EVENT + 1));
		/* FALLTHRU */
	    case KEY_ENTER:
		result = button ? DLG_EXIT_CANCEL : DLG_EXIT_OK;
		break;
#ifdef KEY_RESIZE
	    case KEY_RESIZE:
		dlg_clear();
		height = req_high;
		width = req_wide;
		goto restart;
#endif
	    default:
		beep();
		break;
	    }
	} else {
	    beep();
	}
    }

    dlg_del_window(dialog);
    dlg_mouse_free_regions();
    free(prompt);
    return result;
}
