/*
 * $Id: inputstr.c,v 1.15 2003/07/20 19:10:07 tom Exp $
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

/*
 * Counts the number of bytes that make up complete wide-characters, up to byte
 * 'len'.
 */
#ifdef HAVE_WGET_WCH
static int
count_wcbytes(char *string, size_t len)
{
    int part = 0;
    int result = 0;

    while (len != 0) {
	const char *src = string;
	mbstate_t state;
	int save = string[len];

	string[len] = '\0';
	memset(&state, 0, sizeof(state));
	result = mbsrtowcs((wchar_t *) 0, &src, len, &state);
	string[len] = save;
	if (result >= 0) {
	    break;
	}
	++part;
	--len;
    }
    return len;
}
#else
#define count_wcbytes(string,len) len
#endif

/*
 * Counts the number of wide-characters in the string, up to byte 'len'.
 */
#ifdef HAVE_WGET_WCH
static int
count_wchars(char *string, size_t len)
{
    const char *src = string;
    mbstate_t state;
    int part = count_wcbytes(string, len);
    int save = string[part];
    int result;
    wchar_t *temp = calloc(sizeof(wchar_t), len + 1);

    string[part] = '\0';
    memset(&state, 0, sizeof(state));
    result = mbsrtowcs(temp, &src, part, &state);
    string[part] = save;
    len = wcslen(temp);
    free(temp);

    return len;
}
#else
#define count_wchars(string,len) len
#endif

/*
 * Build an index of the wide-characters in the string, so we can easily tell
 * which byte-offset begins a given wide-character.
 */
static int *
index_wchars(char *string)
{
    static int *list;
    static unsigned have;
    unsigned len = count_wchars(string, strlen(string));
    unsigned inx;

    if (list == 0) {
	have = 80;
	list = malloc(sizeof(int *) * have);
    }
    if ((len + 2) > have) {
	have = (2 * len) + 3;
	list = realloc(list, sizeof(int *) * have);
    }
    list[0] = 0;
    for (inx = 1; inx <= len; ++inx) {
#ifdef HAVE_WGET_WCH
	mbstate_t state;
	int width;
	memset(&state, 0, sizeof(state));
	width = mbrlen(string, strlen(string), &state);
	string += width;
	list[inx] = list[inx - 1] + width;
#else
	list[inx] = inx;
#endif
    }
    return list;
}

/*
 * Given the character-offset to find in the list, return the corresponding
 * array index.
 */
static int
find_index(int *list, int limit, int to_find)
{
    int result;
    for (result = 0; result <= limit; ++result) {
	if (to_find == list[result]
	    || result == limit
	    || to_find < list[result + 1])
	    break;
    }
    return result;
}

/*
 * Build a list of the display-columns for the given string's characters.
 */
static int *
index_columns(char *string)
{
    static int *list;
    static unsigned have;
    size_t num_bytes = strlen(string);
    unsigned len = count_wchars(string, num_bytes);
    unsigned inx;

    if (list == 0) {
	have = 80;
	list = malloc(sizeof(int *) * have);
    }
    if ((len + 2) > have) {
	have = (2 * len) + 3;
	list = realloc(list, sizeof(int *) * have);
    }
    list[0] = 0;
#ifdef HAVE_WGET_WCH
    {
	int *inx_wchars = index_wchars(string);
	mbstate_t state;

	for (inx = 0; inx < len; ++inx) {
	    wchar_t temp;
	    int check;
	    int result;

	    memset(&state, 0, sizeof(state));
	    check = mbrtowc(&temp, string + inx_wchars[inx], num_bytes -
			    inx_wchars[inx], &state);
	    if (check <= 0)
		result = 1;
	    else
		result = wcwidth(temp);
	    if (result < 0) {
		cchar_t temp2;
		setcchar(&temp2, &temp, 0, 0, 0);
		result = wcslen(wunctrl(&temp2));
	    }
	    list[inx + 1] = result;
	    if (inx > 0)
		list[inx + 1] += list[inx];
	}
    }
#else
    for (inx = 0; inx < len; ++inx) {
	list[inx + 1] = (isprint(string[inx])
			 ? 1
			 : strlen(unctrl(string[inx])));
	if (inx > 0)
	    list[inx + 1] += list[inx];
    }
#endif
    return list;
}

/*
 * Updates the string and character-offset, given various editing characters
 * or literal characters which are inserted at the character-offset.
 */
bool
dlg_edit_string(char *string, int *chr_offset, int key, int fkey, bool force)
{
    int i;
    int len = strlen(string);
    int limit = count_wchars(string, strlen(string));
    int *indx = index_wchars(string);
    int offset = find_index(indx, limit, *chr_offset);
    int max_len = MAX_LEN;
    bool edit = TRUE;

    if (dialog_vars.max_input != 0 && dialog_vars.max_input < MAX_LEN)
	max_len = dialog_vars.max_input;

    /* transform editing characters into equivalent function-keys */
    if (!fkey) {
	fkey = TRUE;		/* assume we transform */
	switch (key) {
	case 0:
	    break;
	case CHR_BACKSPACE:
	    key = KEY_BACKSPACE;
	    break;
	case 21:		/* ^U */
	    key = KEY_DL;
	    break;
	case CHR_DELETE:
	    key = KEY_DC;
	    break;
	case '\n':
	case '\r':
	    key = KEY_ENTER;
	    break;
	case ESC:
	case TAB:
	    fkey = FALSE;	/* this is used for navigation */
	    break;
	default:
	    fkey = FALSE;	/* ...no, we did not transform */
	    break;
	}
    }

    if (fkey) {
	switch (key) {
	case 0:		/* special case for loop entry */
	    edit = force;
	    break;
	case KEY_LEFT:
	    if (*chr_offset)
		*chr_offset = indx[offset - 1];
	    break;
	case KEY_RIGHT:
	    if (offset < limit)
		*chr_offset = indx[offset + 1];
	    break;
	case KEY_HOME:
	    if (*chr_offset)
		*chr_offset = 0;
	    break;
	case KEY_END:
	    if (offset < limit)
		*chr_offset = indx[limit];
	    break;
	case KEY_BACKSPACE:
	    if (offset) {
		int gap = indx[offset] - indx[offset - 1];
		*chr_offset = indx[offset - 1];
		for (i = *chr_offset;
		     (string[i] = string[i + gap]) != '\0';
		     i++) {
		    ;
		}
	    }
	    break;
	case KEY_DC:
	    if (limit) {
		if (--limit == 0) {
		    string[*chr_offset = 0] = '\0';
		} else {
		    int gap = indx[offset + 1] - indx[offset];
		    for (i = indx[offset];
			 (string[i] = string[i + gap]) != '\0';
			 i++) {
			;
		    }
		    if (*chr_offset > indx[limit])
			*chr_offset = indx[limit];
		}
	    }
	    break;
	case KEY_DL:
	    string[*chr_offset = 0] = '\0';
	    break;
	case KEY_ENTER:
	    edit = 0;
	    break;
	default:
	    beep();
	    break;
	}
    } else {
	if (key == ESC || key == TAB) {
	    edit = 0;
	} else {
	    if (len < max_len) {
		for (i = ++len; i > *chr_offset; i--)
		    string[i] = string[i - 1];
		string[*chr_offset] = key;
		*chr_offset += 1;
	    } else {
		(void) beep();
	    }
	}
    }
    return edit;
}

static void
compute_edit_offset(char *string,
		    int chr_offset,
		    int x_last,
		    int *p_dpy_column,
		    int *p_scroll_amt)
{
    int *cols = index_columns(string);
    int *indx = index_wchars(string);
    int limit = count_wchars(string, strlen(string));
    int offset = find_index(indx, limit, chr_offset);
    int offset2;
    int dpy_column;
    int n;

    for (n = offset2 = 0; n <= offset; ++n) {
	if ((cols[offset] - cols[n]) < x_last
	    && (offset == limit || (cols[offset + 1] - cols[n]) < x_last)) {
	    offset2 = n;
	    break;
	}
    }

    dpy_column = cols[offset] - cols[offset2];

    if (p_dpy_column != 0)
	*p_dpy_column = dpy_column;
    if (p_scroll_amt != 0)
	*p_scroll_amt = offset2;
}

/*
 * Given the character-offset in the string, returns the display-offset where
 * we will position the cursor.
 */
int
dlg_edit_offset(char *string, int chr_offset, int x_last)
{
    int result;

    compute_edit_offset(string, chr_offset, x_last, &result, 0);

    return result;
}

/*
 * Displays the string, shifted as necessary, to fit within the box and show
 * the current character-offset.
 */
void
dlg_show_string(WINDOW *win,
		char *string,	/* string to display (may be multibyte) */
		int chr_offset,	/* character (not bytes) offset */
		chtype attr,	/* window-attributes */
		int y_base,	/* beginning row on screen */
		int x_base,	/* beginning column on screen */
		int x_last,	/* number of columns on screen */
		bool hidden,	/* if true, do not echo */
		bool force)	/* if true, force repaint */
{
    if (hidden) {
	if (force) {
	    (void) wmove(win, y_base, x_base);
	    wrefresh(win);
	}
    } else {
	int *cols = index_columns(string);
	int *indx = index_wchars(string);
	int limit = count_wchars(string, strlen(string));

	int i, j, k;
	int input_x;
	int scrollamt;

	compute_edit_offset(string, chr_offset, x_last, &input_x, &scrollamt);

	wattrset(win, attr);
	(void) wmove(win, y_base, x_base);
	for (i = scrollamt, k = 0; i < limit && k < x_last; ++i) {
	    int check = cols[i + 1] - cols[scrollamt];
	    if (check <= x_last) {
		for (j = indx[i]; j < indx[i + 1]; ++j) {
		    waddch(win, CharOf(string[j]));
		}
		k = check;
	    } else {
		break;
	    }
	}
	while (k++ < x_last)
	    waddch(win, ' ');
	(void) wmove(win, y_base, x_base + input_x);
	wrefresh(win);
    }
}
