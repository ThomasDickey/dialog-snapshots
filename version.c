/*
 *  $Id: version.c,v 1.2 2003/11/26 16:42:38 tom Exp $
 *
 *  version.c -- dialog's version string
 *
 *  Copyright 2003	Thomas E. Dickey
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
#include <dialog.h>

#define quoted(a)	#a
#define concat(a,b)	a "-" quoted(b)
#define DLG_VERSION	concat(DIALOG_VERSION,DIALOG_PATCHDATE)

char *
dialog_version(void)
{
    return DLG_VERSION;
}
