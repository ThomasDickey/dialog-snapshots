/*
 *  $Id: inputbox.c,v 1.43 2004/09/19 22:09:50 tom Exp $
 *
 *  inputbox.c -- implements the input box
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

#define sTEXT -1

/*
 * Display a dialog box for entering a string
 */
int
dialog_inputbox(const char *title, const char *cprompt, int height, int width,
		const char *init, const int password)
{
    int x, y, box_y, box_x, box_width;
    int show_buttons = TRUE;
    int col_offset = 0;
    int chr_offset = 0;
    int key = 0, fkey = 0, code;
    int result = DLG_EXIT_UNKNOWN;
    int state = dialog_vars.defaultno ? dlg_defaultno_button() : sTEXT;
    int first = (state == sTEXT);
    char *input = dialog_vars.input_result;
    WINDOW *dialog;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_ok_labels();

    dlg_does_output();

    dlg_tab_correct_str(prompt);
    if (init != NULL) {
	dlg_auto_size(title, prompt, &height, &width, 5,
		      MIN(MAX((int) strlen(init) + 7, 26),
			  SCOLS - (dialog_vars.begin_set ?
				   dialog_vars.begin_x : 0)));
    } else {
	dlg_auto_size(title, prompt, &height, &width, 5, 26);
    }
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);

    dlg_mouse_setbase(x, y);

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_bottom_box(dialog);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, height, width);

    /* Draw the input field box */
    box_width = width - 6;
    getyx(dialog, y, x);
    box_y = y + 2;
    box_x = (width - box_width) / 2;
    dlg_mouse_mkregion(y + 1, box_x - 1, 3, box_width + 2, 'i');
    dlg_draw_box(dialog, y + 1, box_x - 1, 3, box_width + 2,
		 border_attr, dialog_attr);

    /* Set up the initial value */
    if (!init)
	input[0] = '\0';
    else
	strcpy((char *) input, init);

    while (result == DLG_EXIT_UNKNOWN) {
	int edit = 0;

	/*
	 * The last field drawn determines where the cursor is shown:
	 */
	if (show_buttons) {
	    show_buttons = FALSE;
	    col_offset = dlg_edit_offset(input, chr_offset, box_width);
	    (void) wmove(dialog, box_y, box_x + col_offset);
	    dlg_draw_buttons(dialog, height - 2, 0, buttons, state, FALSE, width);
	}

	if (!first)
	    key = dlg_mouse_wgetch(dialog, &fkey);

	/*
	 * Handle mouse clicks first, since we want to know if this is a button,
	 * or something that dlg_edit_string() should handle.
	 */
	if (fkey
	    && key >= M_EVENT
	    && (code = dlg_ok_buttoncode(key - M_EVENT)) >= 0) {
	    result = code;
	    continue;
	}

	if (state == sTEXT) {	/* Input box selected */
	    edit = dlg_edit_string(input, &chr_offset, key, fkey, first);

	    if (edit) {
		dlg_show_string(dialog, input, chr_offset, inputbox_attr,
				box_y, box_x, box_width, password, first);
		first = FALSE;
		continue;
	    }
	}

	/* handle non-functionkeys */
	if (!fkey) {

	    if ((code = dlg_char_to_button(key, buttons)) >= 0) {
		dlg_del_window(dialog);
		result = code;
		continue;
	    }

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
	    case M_EVENT + 'i':	/* mouse enter events */
		state = 0;
		/* FALLTHRU */
	    case KEY_BTAB:
	    case KEY_UP:
	    case KEY_LEFT:
		show_buttons = TRUE;
		state = dlg_prev_ok_buttonindex(state, sTEXT);
		break;
	    case KEY_DOWN:
	    case KEY_RIGHT:
		show_buttons = TRUE;
		state = dlg_next_ok_buttonindex(state, sTEXT);
		break;
	    case KEY_ENTER:
		dlg_del_window(dialog);
		result = (state >= 0) ? dlg_ok_buttoncode(state) : DLG_EXIT_OK;
		break;
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
