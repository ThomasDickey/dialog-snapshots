/*
 * $Id: mousewget.c,v 1.16 2004/12/20 00:48:20 tom Exp $
 *
 * mousewget.c -- mouse/wgetch support for dialog
 *
 * Copyright 2000-2002,2003   Thomas E. Dickey
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ********/

#include "dialog.h"

static int
mouse_wgetch(WINDOW *win, int *fkey, bool ignore_errs)
{
    int key;

#if USE_MOUSE

    do {

	key = dlg_getc(win, fkey);
	if (fkey && (key == KEY_MOUSE)) {
	    MEVENT event;
	    mseRegion *p;

	    if (getmouse(&event) != ERR) {
		if ((p = dlg_mouse_region(event.y, event.x)) != 0) {
		    key = M_EVENT + p->code;
		} else if ((p = dlg_mouse_bigregion(event.y, event.x)) != 0) {
		    int x = event.x - p->x;
		    int y = event.y - p->y;
		    int row = (p->X - p->x) / p->step_x;

		    key = -(p->code);
		    switch (p->mode) {
		    case 1:	/* index by lines */
			key += y;
			break;
		    case 2:	/* index by columns */
			key += (x / p->step_x);
			break;
		    default:
		    case 3:	/* index by cells */
			key += (x / p->step_x) + (y * row);
			break;
		    }
		} else {
		    (void) beep();
		    key = ERR;
		}
	    } else {
		(void) beep();
		key = ERR;
	    }
	}

    } while (ignore_errs && (key == ERR));

#else

    do {
	key = dlg_getc(win, fkey);
    } while (ignore_errs && (key == ERR));

#endif

    return key;
}

int
dlg_mouse_wgetch(WINDOW *win, int *fkey)
{
    return mouse_wgetch(win, fkey, TRUE);
}

int
dlg_mouse_wgetch_nowait(WINDOW *win, int *fkey)
{
    return mouse_wgetch(win, fkey, FALSE);
}
