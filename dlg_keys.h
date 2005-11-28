/*
 *  $Id: dlg_keys.h,v 1.8 2005/11/27 23:39:17 tom Exp $
 *
 *  dlg_keys.h -- runtime binding support for dialog
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

#ifndef DLG_KEYS_H_included
#define DLG_KEYS_H_included 1

#include <dialog.h>

typedef struct {
    int is_function_key;
    int	curses_key;
    int dialog_key;
} DLG_KEYS_BINDING;

#define DLG_KEYS_DATA(dialog, curses)  { curses >= KEY_MIN, curses, dialog }

#define END_KEYS_BINDING { -1, 0, 0 }

/*
 * Define dialog's internal function-keys past the range used by curses.
 */
typedef enum {
    DLGK_MIN = KEY_MAX + 1,
    /* moving from screen to screen (pages) */
    DLGK_PAGE_FIRST,
    DLGK_PAGE_LAST,
    DLGK_PAGE_NEXT,
    DLGK_PAGE_PREV,
    /* moving within a list */
    DLGK_ITEM_FIRST,
    DLGK_ITEM_LAST,
    DLGK_ITEM_NEXT,
    DLGK_ITEM_PREV,
    /* moving from field to field (or buttons) */
    DLGK_FIELD_FIRST,
    DLGK_FIELD_LAST,
    DLGK_FIELD_NEXT,
    DLGK_FIELD_PREV,
    /* moving within a grid */
    DLGK_GRID_UP,
    DLGK_GRID_DOWN,
    DLGK_GRID_LEFT,
    DLGK_GRID_RIGHT,
    /* delete */
    DLGK_DELETE_LEFT,
    DLGK_DELETE_RIGHT,
    DLGK_DELETE_ALL,
    /* special */
    DLGK_ENTER,
    DLGK_BEGIN,
    DLGK_FINAL,
    DLGK_SELECT
} DLG_KEYS_ENUM;

#define DLGK_MOUSE(code) ((code) + M_EVENT)

#define INPUTSTR_BINDINGS \
	DLG_KEYS_DATA( DLGK_BEGIN,	   KEY_HOME ), \
	DLG_KEYS_DATA( DLGK_DELETE_ALL,    21 ), \
	DLG_KEYS_DATA( DLGK_DELETE_LEFT,   CHR_BACKSPACE ), \
	DLG_KEYS_DATA( DLGK_DELETE_LEFT,   KEY_BACKSPACE ), \
	DLG_KEYS_DATA( DLGK_DELETE_RIGHT,  CHR_DELETE ), \
	DLG_KEYS_DATA( DLGK_DELETE_RIGHT,  KEY_DC ), \
	DLG_KEYS_DATA( DLGK_FINAL,	   KEY_END ), \
	DLG_KEYS_DATA( DLGK_GRID_LEFT,	   KEY_LEFT ), \
	DLG_KEYS_DATA( DLGK_GRID_RIGHT,	   KEY_RIGHT )

extern int dlg_lookup_key(WINDOW * /*win */, int /* curses_key */, int * /*dialog_key */);
extern void dlg_register_window(WINDOW * /* win */, const char * /* name */, DLG_KEYS_BINDING * /* binding */);
extern void dlg_unregister_window(WINDOW * /* win */);

#endif /* DLG_KEYS_H_included */
