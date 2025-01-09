/*
 *  $Id: help.c,v 1.5 2025/01/09 22:33:20 tom Exp $
 *
 *  help.c -- implements the help dialog
 *
 *  Copyright 2011-2022,2025	Thomas E. Dickey
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

#include <dlg_internals.h>

/*
 * Display a help-file as a textbox widget.
 */
int
dialog_helpfile(const char *title,
		const char *file,
		int height,
		int width)
{
    int result = DLG_EXIT_ERROR;
    DIALOG_VARS save;

    if (!dialog_vars.in_helpfile && file != NULL && *file != '\0') {
	dlg_save_vars(&save);

	dialog_vars.no_label = NULL;
	dialog_vars.ok_label = NULL;
	dialog_vars.help_button = FALSE;
	dialog_vars.extra_button = FALSE;
	dialog_vars.nook = FALSE;

	dialog_vars.in_helpfile = TRUE;

	result = dialog_textbox(title, file, height, width);

	dlg_restore_vars(&save);
    }
    return (result);
}
