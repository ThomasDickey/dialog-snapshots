/*
 *  arrows.c -- draw arrows to indicate end-of-range for lists
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

void
dlg_draw_arrows(WINDOW *dialog, int top_arrow, int bottom_arrow, int x, int
    top, int bottom)
{
    int cur_x, cur_y;

    getyx(dialog, cur_y, cur_x);

    wmove(dialog, top, x);
    if (top_arrow) {
	wattrset(dialog, uarrow_attr);
	waddch(dialog, ACS_UARROW);
	waddstr(dialog, "(+)");
    } else {
	wattrset(dialog, menubox_attr);
	whline(dialog, ACS_HLINE, 4);
    }
    wmove(dialog, bottom, x);
    if (bottom_arrow) {
	wattrset(dialog, darrow_attr);
	waddch(dialog, ACS_DARROW);
	waddstr(dialog, "(+)");
    } else {
	wattrset(dialog, menubox_border_attr);
	whline(dialog, ACS_HLINE, 4);
    }
    wmove(dialog, cur_y, cur_x);
    wrefresh_lock(dialog);
}
