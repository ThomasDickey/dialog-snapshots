/*
 * $Id: inputstr.c,v 1.40 2004/12/23 00:46:15 tom Exp $
 *
 *  inputstr.c -- functions for input/display of a string
 *
 * Copyright 2000-2003,2004	Thomas E. Dickey
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
#include <errno.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

typedef struct _cache {
    struct _cache *next;
#ifdef USE_WIDE_CURSES
    struct _cache *cache_at;	/* unique: associate caches by CACHE */
    const char *string_at;	/* unique: associate caches by char* */
#endif
    unsigned s_len;		/* strlen(string) - we add 1 for EOS */
    unsigned i_len;		/* length(list) - we add 1 for EOS */
    char *string;		/* a copy of the last-processed string */
    int *list;			/* indices into the string */
} CACHE;

#ifdef USE_WIDE_CURSES
#define SAME_CACHE(c,s,l) (c->string != 0 && memcmp(c->string,s,l) == 0)

static CACHE *cache_list;

#ifdef USE_WIDE_CURSES
static int
have_locale(void)
{
    static int result = -1;
    if (result < 0) {
	char *test = setlocale(LC_ALL, 0);
	if (test == 0 || *test == 0) {
	    result = FALSE;
	} else if (strcmp(test, "C") && strcmp(test, "POSIX")) {
	    result = TRUE;
	} else {
	    result = FALSE;
	}
    }
    return result;
}
#endif

static void
make_cache(CACHE * cache, const char *string)
{
    CACHE *p;

    p = (CACHE *) calloc(1, sizeof(CACHE));
    assert_ptr(p, "load_cache");
    p->next = cache_list;
    cache_list = p;

    p->cache_at = cache;
    p->string_at = string;

    *cache = *p;
}

static void
load_cache(CACHE * cache, const char *string)
{
    CACHE *p;

    for (p = cache_list; p != 0; p = p->next) {
	if (p->cache_at == cache
	    && p->string_at == string) {
	    *cache = *p;
	    return;
	}
    }
    make_cache(cache, string);
}

static void
save_cache(CACHE * cache, const char *string)
{
    CACHE *p;

    for (p = cache_list; p != 0; p = p->next) {
	if (p->cache_at == cache
	    && p->string_at == string) {
	    CACHE *q = p->next;
	    *p = *cache;
	    p->next = q;
	    return;
	}
    }
}
#else
#define SAME_CACHE(c,s,l) (c->string != 0)
#define load_cache(cache, string)	/* nothing */
#define save_cache(cache, string)	/* nothing */
#endif /* USE_WIDE_CURSES */

/*
 * If the given string has not changed, we do not need to update the index.
 * If we need to update the index, allocate enough memory for it.
 */
static bool
same_cache2(CACHE * cache, const char *string, unsigned i_len)
{
    unsigned need;
    unsigned s_len = strlen(string);

    if (cache->s_len != 0
	&& cache->s_len >= s_len
	&& cache->list != 0
	&& SAME_CACHE(cache, string, s_len)) {
	return TRUE;
    }

    need = sizeof(int *) * (i_len + 1);
    if (cache->list == 0) {
	cache->list = malloc(need);
    } else if (cache->i_len < i_len) {
	cache->list = realloc(cache->list, need);
    }
    cache->i_len = i_len;

    if (cache->s_len >= s_len && cache->string != 0) {
	strcpy(cache->string, string);
    } else {
	if (cache->string != 0)
	    free(cache->string);
	cache->string = dlg_strclone(string);
    }
    cache->s_len = s_len;

    return FALSE;
}

#ifdef USE_WIDE_CURSES
/*
 * Like same_cache2(), but we are only concerned about caching a copy of the
 * string and its associated length.
 */
static bool
same_cache1(CACHE * cache, const char *string, unsigned i_len)
{
    unsigned s_len = strlen(string);

    if (cache->s_len == s_len
	&& SAME_CACHE(cache, string, s_len)) {
	return TRUE;
    }

    if (cache->s_len >= s_len && cache->string != 0) {
	strcpy(cache->string, string);
    } else {
	if (cache->string != 0)
	    free(cache->string);
	cache->string = dlg_strclone(string);
    }
    cache->s_len = s_len;
    cache->i_len = i_len;

    return FALSE;
}
#endif /* USE_WIDE_CURSES */

/*
 * Counts the number of bytes that make up complete wide-characters, up to byte
 * 'len'.  If there is no locale set, simply return the original length.
 */
#ifdef USE_WIDE_CURSES
static int
dlg_count_wcbytes(const char *string, size_t len)
{
    int result;

    if (have_locale()) {
	static CACHE cache;

	load_cache(&cache, string);
	if (!same_cache1(&cache, string, len)) {
	    while (len != 0) {
		int part = 0;
		int code = 0;
		const char *src = cache.string;
		mbstate_t state;
		int save = cache.string[len];

		cache.string[len] = '\0';
		memset(&state, 0, sizeof(state));
		code = mbsrtowcs((wchar_t *) 0, &src, len, &state);
		cache.string[len] = save;
		if (code >= 0) {
		    break;
		}
		++part;
		--len;
	    }
	    cache.i_len = len;
	}
	save_cache(&cache, string);
	result = cache.i_len;
    } else {
	result = len;
    }
    return result;
}
#endif /* USE_WIDE_CURSES */

/*
 * Counts the number of wide-characters in the string.
 */
int
dlg_count_wchars(const char *string)
{
    int result;

#ifdef USE_WIDE_CURSES
    if (have_locale()) {
	static CACHE cache;
	size_t len = strlen(string);

	load_cache(&cache, string);
	if (!same_cache1(&cache, string, len)) {
	    const char *src = cache.string;
	    mbstate_t state;
	    int part = dlg_count_wcbytes(cache.string, len);
	    int save = cache.string[part];
	    int code;
	    wchar_t *temp = calloc(sizeof(wchar_t), len + 1);

	    cache.string[part] = '\0';
	    memset(&state, 0, sizeof(state));
	    code = mbsrtowcs(temp, &src, part, &state);
	    cache.i_len = (code >= 0) ? wcslen(temp) : 0;
	    cache.string[part] = save;
	    free(temp);
	}
	save_cache(&cache, string);
	result = cache.i_len;
    } else
#endif /* USE_WIDE_CURSES */
    {
	result = strlen(string);
    }
    return result;
}

/*
 * Build an index of the wide-characters in the string, so we can easily tell
 * which byte-offset begins a given wide-character.
 */
const int *
dlg_index_wchars(const char *string)
{
    static CACHE cache;
    unsigned len = dlg_count_wchars(string);
    unsigned inx;

    load_cache(&cache, string);
    if (!same_cache2(&cache, string, len)) {
	const char *current = string;

	cache.list[0] = 0;
	for (inx = 1; inx <= len; ++inx) {
#ifdef USE_WIDE_CURSES
	    if (have_locale()) {
		mbstate_t state;
		int width;
		memset(&state, 0, sizeof(state));
		width = mbrlen(current, strlen(current), &state);
		if (width <= 0)
		    width = 1;	/* FIXME: what if we have a control-char? */
		current += width;
		cache.list[inx] = cache.list[inx - 1] + width;
	    } else
#endif /* USE_WIDE_CURSES */
	    {
		(void) current;
		cache.list[inx] = inx;
	    }
	}
    }
    save_cache(&cache, string);
    return cache.list;
}

/*
 * Given the character-offset to find in the list, return the corresponding
 * array index.
 */
static int
dlg_find_index(const int *list, int limit, int to_find)
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
const int *
dlg_index_columns(const char *string)
{
    static CACHE cache;
    unsigned len = dlg_count_wchars(string);
    unsigned inx;

    load_cache(&cache, string);
    if (!same_cache2(&cache, string, len)) {
	cache.list[0] = 0;
#ifdef USE_WIDE_CURSES
	if (have_locale()) {
	    size_t num_bytes = strlen(string);
	    const int *inx_wchars = dlg_index_wchars(string);
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
		cache.list[inx + 1] = result;
		if (inx > 0)
		    cache.list[inx + 1] += cache.list[inx];
	    }
	} else
#endif /* USE_WIDE_CURSES */
	{
	    for (inx = 0; inx < len; ++inx) {
		cache.list[inx + 1] = (isprint(UCH(string[inx]))
				       ? 1
				       : strlen(unctrl(UCH(string[inx]))));
		if (string[inx] == '\n')
		    cache.list[inx + 1] = 1;
		if (inx != 0)
		    cache.list[inx + 1] += cache.list[inx];
	    }
	}
    }
    save_cache(&cache, string);
    return cache.list;
}

/*
 * Returns the number of columns used for a string.  That happens to be the
 * end-value of the cols[] array.
 */
int
dlg_count_columns(const char *string)
{
    int result = 0;
    int limit = dlg_count_wchars(string);
    if (limit > 0) {
	const int *cols = dlg_index_columns(string);
	result = cols[limit];
    } else {
	result = strlen(string);
    }
    return result;
}

/*
 * Given a column limit, count the number of wide characters that can fit
 * into that limit.  The offset is used to skip over a leading character
 * that was already written.
 */
int
dlg_limit_columns(const char *string, int limit, int offset)
{
    const int *cols = dlg_index_columns(string);
    int result = dlg_count_wchars(string);

    while (result > 0 && (cols[result] - cols[offset]) > limit)
	--result;
    return result;
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
    int limit = dlg_count_wchars(string);
    const int *indx = dlg_index_wchars(string);
    int offset = dlg_find_index(indx, limit, *chr_offset);
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
		if (gap > 0) {
		    for (i = *chr_offset;
			 (string[i] = string[i + gap]) != '\0';
			 i++) {
			;
		    }
		}
	    }
	    break;
	case KEY_DC:
	    if (limit) {
		if (--limit == 0) {
		    string[*chr_offset = 0] = '\0';
		} else {
		    int gap = ((offset <= limit)
			       ? (indx[offset + 1] - indx[offset])
			       : 0);
		    if (gap > 0) {
			for (i = indx[offset];
			     (string[i] = string[i + gap]) != '\0';
			     i++) {
			    ;
			}
		    } else if (offset > 0) {
			string[indx[offset - 1]] = '\0';
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
compute_edit_offset(const char *string,
		    int chr_offset,
		    int x_last,
		    int *p_dpy_column,
		    int *p_scroll_amt)
{
    const int *cols = dlg_index_columns(string);
    const int *indx = dlg_index_wchars(string);
    int limit = dlg_count_wchars(string);
    int offset = dlg_find_index(indx, limit, chr_offset);
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
		const char *string,	/* string to display (may be multibyte) */
		int chr_offset,	/* character (not bytes) offset */
		chtype attr,	/* window-attributes */
		int y_base,	/* beginning row on screen */
		int x_base,	/* beginning column on screen */
		int x_last,	/* number of columns on screen */
		bool hidden,	/* if true, do not echo */
		bool force)	/* if true, force repaint */
{
    x_last = MIN(x_last + x_base, getmaxx(win)) - x_base;

    if (hidden && !dialog_vars.insecure) {
	if (force) {
	    (void) wmove(win, y_base, x_base);
	    wrefresh(win);
	}
    } else {
	const int *cols = dlg_index_columns(string);
	const int *indx = dlg_index_wchars(string);
	int limit = dlg_count_wchars(string);

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
		    if (hidden && dialog_vars.insecure)
			waddch(win, '*');
		    else
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

#ifdef NO_LEAKS
void
_dlg_inputstr_leaks(void)
{
    while (cache_list != 0) {
	CACHE *next = cache_list->next;
	if (cache_list->string != 0)
	    free(cache_list->string);
	if (cache_list->list != 0)
	    free(cache_list->list);
	free(cache_list);
	cache_list = next;
    }
}
#endif
