/*
 * $Id: mousewget.c,v 1.13 2003/07/11 22:49:50 tom Exp $
 *
 * mousewget.c - mouse_wgetch support for dialog
 *
 * Copyright 2000-2001,2002   Thomas Dickey
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

int
mouse_wgetch(WINDOW *win, int *fkey)
{
    int key;

#if USE_MOUSE

    do {

	key = dlg_getc(win, fkey);
	if (fkey && (key == KEY_MOUSE)) {
	    MEVENT event;
	    mseRegion *p;

	    if (getmouse(&event) != ERR) {
		if ((p = mouse_region(event.y, event.x)) != 0) {
		    key = M_EVENT + p->code;
		} else if ((p = mouse_bigregion(event.y, event.x)) != 0) {
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

    } while (key == ERR);

#else

    do {
	key = dlg_getc(win, fkey);
    } while (key == ERR);

#endif

    return key;
}
