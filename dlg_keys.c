/*
 *  $Id: dlg_keys.c,v 1.2 2005/11/27 18:59:32 tom Exp $
 *
 *  dlg_keys.c -- runtime binding support for dialog
 *
 *  Copyright 2005	Thomas E. Dickey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to
 *	Free Software Foundation, Inc.
 *	51 Franklin St., Fifth Floor
 *	Boston, MA 02110, USA.
 */

#include <dialog.h>
#include <dlg_keys.h>

#define LIST_BINDINGS struct _list_bindings

LIST_BINDINGS {
    LIST_BINDINGS *link;
    WINDOW *win;
    const char *name;
    DLG_KEYS_BINDING *binding;
};

static LIST_BINDINGS *all_bindings;

/*
 * For a given named widget's window, associate a binding table.
 */
void
dlg_register_window(WINDOW *win, const char *name, DLG_KEYS_BINDING * binding)
{
    LIST_BINDINGS *p;

    for (p = all_bindings; p != 0; p = p->link) {
	if (p->win == win && !strcmp(p->name, name)) {
	    p->binding = binding;
	    return;
	}
    }
    if ((p = (LIST_BINDINGS *) calloc(1, sizeof(*p))) != 0) {
	p->link = all_bindings;
	p->win = win;
	p->name = name;
	p->binding = binding;
	all_bindings = p;
    }
}

/*
 * Remove the bindings for a given window.
 */
void
dlg_unregister_window(WINDOW *win)
{
    LIST_BINDINGS *p, *q;

    for (p = all_bindings, q = 0; p != 0; p = p->link) {
	if (p->win == win) {
	    if (q != 0) {
		q->link = p->link;
	    } else {
		all_bindings = p->link;
	    }
	    free(p);
	    break;
	}
	q = p;
    }
}

/*
 * Call this after wgetch(), using the same window pointer and passing
 * the curses-key.
 *
 * If there is no binding associated with the widget, it simply returns
 * the given curses-key.
 *
 * Parameters:
 *	win is the window on which the wgetch() was done.
 *	curses_key is the value returned by wgetch().
 *	fkey in/out (on input, it is true if curses_key is a function key,
 *		and on output, it is true if the result is a function key).
 */
int
dlg_lookup_key(WINDOW *win, int curses_key, int *fkey)
{
    LIST_BINDINGS *p;
    int n;

    /*
     * Ignore mouse clicks, since they are already encoded properly.
     */
#ifdef KEY_MOUSE
    if (*fkey != 0 && curses_key == KEY_MOUSE) {
	;
    } else
#endif
	/*
	 * Ignore resize events, since they are already encoded properly.
	 */
#ifdef KEY_RESIZE
    if (*fkey != 0 && curses_key == KEY_RESIZE) {
	;
    } else
#endif
    if (*fkey == 0 || curses_key < KEY_MAX) {
	for (p = all_bindings; p != 0; p = p->link) {
	    if (p->win == win) {
		int function_key = (*fkey != 0);
		for (n = 0; p->binding[n].is_function_key >= 0; ++n) {
		    if (p->binding[n].curses_key == curses_key
			&& p->binding[n].is_function_key == function_key) {
			*fkey = p->binding[n].dialog_key;
			return *fkey;
		    }
		}
	    }
	}
    }
    return curses_key;
}
