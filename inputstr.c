/*
 * $Id: inputstr.c,v 1.7 2002/06/15 00:24:45 tom Exp $
 *
 *  inputstr.c -- functions for input/display of a string
 *
 *  AUTHOR: Thomas Dickey
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

bool
dlg_edit_string(char *string, int *offset, int key, bool force)
{
    int i;
    int len = strlen(string);
    int max_len = MAX_LEN;
    bool edit = TRUE;

    if (dialog_vars.max_input != 0 && dialog_vars.max_input < MAX_LEN)
	max_len = dialog_vars.max_input;
    switch (key) {
    case 0:			/* special case for loop entry */
	edit = force;
	break;
    case KEY_LEFT:
	if (*offset)
	    *offset -= 1;
	break;
    case KEY_RIGHT:
	if (*offset < len)
	    *offset += 1;
	break;
    case KEY_HOME:
	if (*offset)
	    *offset = 0;
	break;
    case KEY_END:
	if (*offset < len)
	    *offset = len;
	break;
    case 8:
    case KEY_BACKSPACE:
	if (*offset) {
	    *offset -= 1;
	    len--;
	    for (i = *offset; i <= len; i++)
		string[i] = string[i + 1];
	}
	break;
    case 127:
    case KEY_DC:
	if (len) {
	    if (--len == 0) {
		string[*offset = 0] = '\0';
	    } else {
		if (*offset > len)
		    *offset = len;
		for (i = *offset; i <= len; i++)
		    string[i] = string[i + 1];
	    }
	}
	break;
    case 21:			/* ^U */
	string[len = *offset = 0] = '\0';
	break;
    default:
	if (key < 0x100 && isprint(key)) {
	    if (len < max_len) {
		for (i = ++len; i > *offset; i--)
		    string[i] = string[i - 1];
		string[*offset] = key;
		*offset += 1;
	    } else
		(void) flash();	/* Alarm user about overflow */
	    break;
	}
	edit = 0;
	break;
    }
    return edit;
}

/*
 * Returns the offset in the string-editor where we will display the cursor
 */
int
dlg_edit_offset(int offset, int x_last)
{
    int scrollamt = (offset + 1 - x_last);

    if (scrollamt < 0)
	scrollamt = 0;
    return offset - scrollamt;
}

void
dlg_show_string(WINDOW *win, char *string, int offset, chtype attr,
		int y_base, int x_base, int x_last, bool hidden, bool force)
{
    if (hidden) {
	if (force) {
	    (void) wmove(win, y_base, x_base);
	    wrefresh(win);
	}
    } else {
	int i;
	int input_x = dlg_edit_offset(offset, x_last);
	int len = strlen(string);
	int scrollamt = (offset + 1 - x_last);

	if (scrollamt < 0)
	    scrollamt = 0;

	wattrset(win, attr);
	(void) wmove(win, y_base, x_base);
	for (i = 0; i < x_last; i++)
	    (void) waddch(win,
			  (i + scrollamt) < len
			  ? CharOf(string[scrollamt + i])
			  : ' ');
	(void) wmove(win, y_base, x_base + input_x);
	wrefresh(win);
    }
}
