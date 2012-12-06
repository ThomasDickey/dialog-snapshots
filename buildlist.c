/*
 *  $Id: buildlist.c,v 1.1 2012/12/04 10:37:07 tom Exp $
 *
 *  treeview.c -- implements the treeview dialog
 *
 *  Copyright 2012	Thomas E. Dickey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 2.1
 *  as published by the Free Software Foundation.
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

/*
 * TODO
 * result in Xdialog is tags separated by "|"
 */
int
dialog_buildlist(const char *title,
		const char *file,
		int height,
		int width,
		int list_height,
		int item_no,
		char **items)
{
    int result = DLG_EXIT_ERROR;

    return (result);
}
