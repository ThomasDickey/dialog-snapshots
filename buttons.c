/*
 *  buttons.c
 *
 *  AUTHOR: Thomas Dickey <dickey@clark.net>
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
 * Print a button
 */
void
print_button(WINDOW *win, const char *label, int y, int x, int selected)
{
    int i, temp;

    wmove(win, y, x);
    wattrset(win, selected
	? button_active_attr
	: button_inactive_attr);
    waddstr(win, "<");
    temp = strspn(label, " ");
    mouse_mkbutton(y, x, strlen(label) + 2, tolower(label[temp]));
    label += temp;
    wattrset(win, selected
	? button_label_active_attr
	: button_label_inactive_attr);
    for (i = 0; i < temp; i++)
	waddch(win, ' ');
    wattrset(win, selected
	? button_key_active_attr
	: button_key_inactive_attr);
    waddch(win, label[0]);
    wattrset(win, selected
	? button_label_active_attr
	: button_label_inactive_attr);
    wprintw(win, "%s", label + 1);
    wattrset(win, selected ? button_active_attr : button_inactive_attr);
    waddstr(win, ">");
    wmove(win, y, x + temp + 1);
}

/*
 * Print a list of buttons at the given position.
 */
void
dlg_draw_buttons(WINDOW *win, int y, int x, const char **labels, int selected, int
    vertical, int limit)
{
    int n;
    int step;
    int length = 0;
    int longest = 0;
    int first_y, first_x;
    int final_y, final_x;
    int found = FALSE;
    int gap;
    int margin;
    int count = 0;

    getyx(win, first_y, first_x);

    for (n = 0; labels[n] != 0; n++) {
	count++;
	if (vertical) {
	    length++;
	    longest = 1;
	} else {
	    int len = strlen(labels[n]);
	    if (len > longest)
		longest = len;
	    length += len;
	}
    }

    /*
     * FIXME:
     * If we can, make all of the buttons the same size.  This is only optional
     * for buttons laid out horizontally.
     */
    if ((gap = (limit - length) / (count + 3)) <= 0) {
	gap = (limit - length) / (count + 1);
	margin = gap;
    } else {
	margin = gap * 2;
    }
    step = gap + (length / count);
    if (vertical) {
	if (gap > 1)
	    y += (gap - 1);
    } else {
	x += margin;
    }

    final_x = 0;
    final_y = 0;
    for (n = 0; labels[n] != 0; n++) {
	print_button(win, labels[n], y, x,
	    (selected == n) || (n == 0 && selected < 0));
	if (!found) {
	    found = (selected == n);
	    getyx(win, final_y, final_x);
	}
	if (vertical) {
	    if ((y += step) > limit)
		break;
	} else {
	    if ((x += step) > limit)
		break;
	}
    }
    if (found)
	wmove(win, final_y, final_x);
    else
	wmove(win, first_y, first_x);
    wrefresh_lock(win);
}

const char **
dlg_ok_labels(void)
{
    static const char *labels[3];
    int n = 0;

    labels[n++] = LABEL_OK;
    if (!dialog_vars.nocancel)
	labels[n++] = LABEL_CANCEL;
    labels[n] = 0;
    return labels;
}

const char **
dlg_yes_labels(void)
{
    static const char *labels[3];
    int n = 0;

    labels[n++] = LABEL_YES;
    labels[n++] = LABEL_NO;
    labels[n] = 0;
    return labels;
}
