/*
 *  $Id: formbox.c,v 1.32 2005/02/07 00:10:37 tom Exp $
 *
 *  formbox.c -- implements the form (i.e, some pairs label/editbox)
 *
 *  AUTHOR: Valery Reznic (valery_reznic@users.sourceforge.net)
 *     and: Thomas E. Dickey
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

#define LLEN(n) ((n) * FORMBOX_TAGS)

#define ItemName(i)     items[LLEN(i) + 0]
#define ItemNameY(i)    items[LLEN(i) + 1]
#define ItemNameX(i)    items[LLEN(i) + 2]
#define ItemText(i)     items[LLEN(i) + 3]
#define ItemTextY(i)    items[LLEN(i) + 4]
#define ItemTextX(i)    items[LLEN(i) + 5]
#define ItemTextFLen(i) items[LLEN(i) + 6]
#define ItemTextILen(i) items[LLEN(i) + 7]
#define ItemHelp(i)     (dialog_vars.item_help ? items[LLEN(i) + 8] : "")

typedef struct {
    char *name;			/* the field label */
    int name_len;		/* ...its length */
    int name_y;			/* ...its y-ordinate */
    int name_x;			/* ...its x-ordinate */
    char *text;			/* the field contents */
    int text_len;		/* ...its length on the screen */
    int text_y;			/* ...its y-ordinate */
    int text_x;			/* ...its x-ordinate */
    int text_flen;		/* ...its length on screen, or 0 if no input allowed */
    int text_ilen;		/* ...its limit on amount to be entered */
    char *help;			/* help-message, if any */
} FORM_ELT;

static bool
in_window(WINDOW *win, int scrollamt, int y)
{
    return (y >= scrollamt && y - scrollamt < getmaxy(win));
}

static bool
ok_move(WINDOW *win, int scrollamt, int y, int x)
{
    return in_window(win, scrollamt, y)
	&& (wmove(win, y - scrollamt, x) != ERR);
}

static void
move_past(WINDOW *win, int y, int x)
{
    if (wmove(win, y, x) == ERR)
	wmove(win, y, getmaxx(win) - 1);
}

/*
 * Print form item
 */
static int
print_item(WINDOW *win, FORM_ELT * elt, int scrollamt, bool choice)
{
    int count = 0;
    int len;

    if (ok_move(win, scrollamt, elt->name_y, elt->name_x)) {
	len = elt->name_len;
	len = MIN(len, getmaxx(win) - elt->name_x);
	if (len > 0) {
	    dlg_show_string(win,
			    elt->name,
			    0,
			    menubox_attr,
			    elt->name_y,
			    elt->name_x,
			    len,
			    FALSE,
			    FALSE);
	    move_past(win, elt->name_y, elt->name_x + len);
	    count = 1;
	}
    }
    if (elt->text_len && ok_move(win, scrollamt, elt->text_y, elt->text_x)) {
	len = elt->text_len;
	len = MIN(len, getmaxx(win) - elt->text_x);
	if (len > 0) {
	    dlg_show_string(win,
			    elt->text,
			    0,
			    choice ? form_active_text_attr : form_text_attr,
			    elt->text_y,
			    elt->text_x,
			    len,
			    FALSE,
			    FALSE);
	    move_past(win, elt->text_y, elt->text_x + len);
	    count = 1;
	}
    }
    return count;
}

/*
 * Print the entire form.
 */
static void
print_form(WINDOW *win, FORM_ELT * elt, int total, int scrollamt, int choice)
{
    int n;
    int count = 0;

    for (n = 0; n < total; ++n) {
	count += print_item(win, elt + n, scrollamt, n == choice);
    }
    if (count) {
	wbkgdset(win, menubox_attr | ' ');
	wclrtobot(win);
	(void) wnoutrefresh(win);
    }
}

static int
set_choice(FORM_ELT elt[], int choice, int item_no)
{
    int i;
    if (elt[choice].text_flen > 0)
	return choice;
    for (i = 0; i < item_no; i++) {
	if (elt[i].text_flen > 0)
	    return i;
    }
    dlg_exiterr("No field has flen > 0\n");
    return -1;			/* Dummy, to make compiler happy */
}

/*
 * Find the last y-value in the form.
 */
static int
form_limit(FORM_ELT elt[])
{
    int n;
    int limit = 0;
    for (n = 0; elt[n].name != 0; ++n) {
	if (limit < elt[n].name_y)
	    limit = elt[n].name_y;
	if (limit < elt[n].text_y)
	    limit = elt[n].text_y;
    }
    return limit;
}

/*
 * Tab to the next field.
 */
static bool
tab_next(WINDOW *win,
	 FORM_ELT elt[],
	 int item_no,
	 int stepsize,
	 int *choice,
	 int *scrollamt)
{
    int old_choice = *choice;
    int old_scroll = *scrollamt;
    bool wrapped = FALSE;

    do {
	*choice += stepsize;
	if (*choice < 0) {
	    *choice = item_no - 1;
	    wrapped = TRUE;
	}
	if (*choice >= item_no) {
	    *choice = 0;
	    wrapped = TRUE;
	}
	if (elt[*choice].text_flen > 0) {
	    int lo = MIN(elt[*choice].name_y, elt[*choice].text_y);
	    int hi = MAX(elt[*choice].name_y, elt[*choice].text_y);

	    if (old_choice == *choice)
		break;
	    print_item(win, elt + old_choice, *scrollamt, FALSE);

	    if (*scrollamt < lo + 1 - getmaxy(win))
		*scrollamt = lo + 1 - getmaxy(win);
	    if (*scrollamt > hi)
		*scrollamt = hi;
	    /*
	     * If we have to scroll to show a wrap-around, it does get
	     * confusing.  Just give up rather than scroll.  Tab'ing to the
	     * next field in a multi-column form is a different matter.  Scroll
	     * for that.
	     */
	    if (*scrollamt != old_scroll) {
		if (wrapped) {
		    beep();
		    *scrollamt = old_scroll;
		    *choice = old_choice;
		} else {
		    scrollok(win, TRUE);
		    wscrl(win, *scrollamt - old_scroll);
		    scrollok(win, FALSE);
		}
	    }
	    break;
	}
    } while (*choice != old_choice);

    return (old_choice != *choice) || (old_scroll != *scrollamt);
}

/*
 * Scroll to the next page, putting the choice at the first editable field
 * in that page.  Note that fields are not necessarily in top-to-bottom order,
 * nor is there necessarily a field on each row of the window.
 */
static bool
scroll_next(WINDOW *win, FORM_ELT elt[], int stepsize, int *choice, int *scrollamt)
{
    int old_choice = *choice;
    int old_scroll = *scrollamt;
    int old_row = MIN(elt[old_choice].text_y, elt[old_choice].name_y);
    int target = old_scroll + stepsize;
    int n;

    if (stepsize < 0) {
	if (old_row != old_scroll)
	    target = old_scroll;
	else
	    target = old_scroll + stepsize;
	if (target < 0)
	    target = 0;
    } else {
	int limit = form_limit(elt);
	if (target > limit)
	    target = limit;
    }

    for (n = 0; elt[n].name != 0; ++n) {
	if (elt[n].text_flen > 0) {
	    int new_row = MIN(elt[n].text_y, elt[n].name_y);
	    if (abs(new_row - target) < abs(old_row - target)) {
		old_row = new_row;
		*choice = n;
	    }
	}
    }

    if (old_choice != *choice)
	print_item(win, elt + old_choice, *scrollamt, FALSE);

    *scrollamt = *choice;
    if (*scrollamt != old_scroll) {
	scrollok(win, TRUE);
	wscrl(win, *scrollamt - old_scroll);
	scrollok(win, FALSE);
    }
    return (old_choice != *choice) || (old_scroll != *scrollamt);
}

static FORM_ELT *
init_fe(char **items,
	int item_no,
	int *min_height,
	int *min_width)
{
    FORM_ELT *elt;
    int i;
    int min_w = 0;
    int min_h = 0;

    /*
     * Note that elt[item_no].name is null, since we allocate an extra item.
     */
    elt = (FORM_ELT *) calloc(item_no + 1, sizeof(FORM_ELT));
    assert_ptr(elt, "dialog_form");

    for (i = 0; i < item_no; ++i) {
	int name_y = atoi(ItemNameY(i));
	int name_x = atoi(ItemNameX(i));
	int name_len = strlen(ItemName(i));
	int text_y = atoi(ItemTextY(i));
	int text_x = atoi(ItemTextX(i));
	int text_flen = atoi(ItemTextFLen(i));
	int text_ilen = atoi(ItemTextILen(i));
	int text_len = (text_flen > 0
			? text_flen
			: (text_flen < 0
			   ? -text_flen
			   : (int) strlen(ItemText(i))));

	/*
	 * Special value '0' for text_flen: no input allowed
	 * Special value '0' for text_ilen: 'be as text_flen'
	 */
	if (text_ilen == 0)
	    text_ilen = text_len;

	min_h = MAX(min_h, name_y);
	min_h = MAX(min_h, text_y);
	min_w = MAX(min_w, name_x + name_len);
	min_w = MAX(min_w, text_x + text_len);

	elt[i].name = ItemName(i);
	elt[i].name_len = strlen(elt[i].name);
	elt[i].name_y = name_y - 1;
	elt[i].name_x = name_x - 1;
	elt[i].text = ItemText(i);
	elt[i].text_len = text_len;
	elt[i].text_y = text_y - 1;
	elt[i].text_x = text_x - 1;
	elt[i].text_flen = text_flen;
	elt[i].text_ilen = text_ilen;
	elt[i].help = ItemHelp(i);

	if (text_flen > 0) {
	    int max_len = MAX_LEN;
	    if (dialog_vars.max_input != 0 && dialog_vars.max_input < MAX_LEN)
		max_len = dialog_vars.max_input;
	    elt[i].text = malloc(max_len + 1);
	    assert_ptr(elt[i].text, "dialog_form");
	    sprintf(elt[i].text, "%.*s", text_ilen, ItemText(i));
	}
    }

    *min_height = min_h;
    *min_width = min_w;

    return elt;
}

/*
 * Display a form for fulfill a number of fields
 */
int
dialog_form(const char *title, const char *cprompt, int height, int width,
	    int form_height, int item_no, char **items)
{
#define sTEXT -1
    bool show_status = FALSE;
    int form_width;
    int first = TRUE;
    int chr_offset = 0;
    int state = dialog_vars.defaultno ? dlg_defaultno_button() : sTEXT;
    const int password = 0;
    int i, x, y, cur_x, cur_y, box_x, box_y;
    int code;
    int key = 0;
    int fkey;
    int choice = dlg_default_item(items, FORMBOX_TAGS);
    int new_choice, new_scroll;
    int scrollamt = 0;
    int result = DLG_EXIT_UNKNOWN;
    int min_width = 0, min_height = 0;
    bool was_autosize = (height == 0 || width == 0);
    bool show_buttons = FALSE;
    bool scroll_changed = FALSE;
    bool field_changed = FALSE;
    WINDOW *dialog, *form;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_ok_labels();
    FORM_ELT *elt, *current;

    elt = init_fe(items, item_no, &min_height, &min_width);
    dlg_tab_correct_str(prompt);
    dlg_auto_size(title, prompt, &height, &width,
		  1 + 3 * MARGIN,
		  MAX(26, 2 + min_width));

    if (form_height == 0)
	form_height = min_height;

    if (was_autosize) {
	form_height = MIN(SLINES - height, form_height);
	height += form_height;
    } else {
	int thigh = 0;
	int twide = 0;
	dlg_auto_size(title, prompt, &thigh, &twide, 0, width);
	thigh = SLINES - (height - (thigh + 1 + 3 * MARGIN));
	form_height = MIN(thigh, form_height);
    }

    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);

    dlg_mouse_setbase(x, y);

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_bottom_box(dialog);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, height, width);

    form_width = width - 6;
    getyx(dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - form_width) / 2 - 1;

    /* create new window for the form */
    form = dlg_sub_window(dialog, form_height, form_width, y + box_y + 1,
			  x + box_x + 1);

    /* draw a box around the form items */
    dlg_draw_box(dialog, box_y, box_x, form_height + 2, form_width + 2,
		 menubox_border_attr, menubox_attr);

    /* register the new window, along with its borders */
    dlg_mouse_mkbigregion(getbegy(form) - getbegy(dialog),
			  getbegx(form) - getbegx(dialog),
			  getmaxy(form),
			  getmaxx(form),
			  KEY_MAX, 1, 1, 3 /* by cells */ );

    show_buttons = TRUE;
    scroll_changed = TRUE;
    field_changed = TRUE;

    choice = set_choice(elt, choice, item_no);
    current = &elt[choice];

    while (result == DLG_EXIT_UNKNOWN) {
	int edit = FALSE;

	if (scroll_changed) {
	    print_form(form, elt, item_no, scrollamt, choice);
	    dlg_draw_arrows(dialog, scrollamt,
			    scrollamt + form_height < item_no,
			    box_x + 1,
			    box_y,
			    box_y + form_height + 1);
	    scroll_changed = FALSE;
	}

	if (show_buttons) {
	    dlg_item_help("");
	    dlg_draw_buttons(dialog, height - 2, 0, buttons,
			     ((state < 0)
			      ? 1000	/* no such button, not highlighted */
			      : state),
			     FALSE, width);
	    show_buttons = FALSE;
	}

	if (field_changed || state == sTEXT) {
	    if (field_changed)
		chr_offset = 0;
	    current = &elt[choice];
	    dialog_vars.max_input = current->text_ilen;
	    dlg_item_help(current->help);
	    dlg_show_string(form, current->text, chr_offset,
			    form_active_text_attr,
			    current->text_y - scrollamt,
			    current->text_x,
			    current->text_len, password, first);
	    field_changed = FALSE;
	}

	key = dlg_mouse_wgetch(dialog, &fkey);

	/* handle non-functionkeys */
	if (!fkey) {
	    if (state != sTEXT) {
		code = dlg_char_to_button(key, buttons);
		if (code >= 0) {
		    dlg_del_window(dialog);
		    result = code;
		    continue;
		}
	    }

	    fkey = TRUE;
	    switch (key) {
	    case ESC:
		result = DLG_EXIT_ESC;
		continue;
	    case CHR_NEXT:
		key = KEY_NEXT;
		break;
	    case CHR_PREVIOUS:
		key = KEY_PREVIOUS;
		break;
	    case ' ':
		if (state == sTEXT) {
		    fkey = FALSE;
		    break;
		}
		/* FALLTHRU */
	    case '\n':
	    case '\r':
		key = KEY_ENTER;
		break;
	    case TAB:
		state = dlg_next_ok_buttonindex(state, sTEXT);
		show_buttons = TRUE;
		continue;
	    default:
		fkey = FALSE;
		break;
	    }
	}

	/* handle functionkeys */
	if (fkey) {
	    bool do_scroll = FALSE;
	    bool do_tab = FALSE;
	    int move_by = 0;

	    switch (key) {
	    case M_EVENT + KEY_PPAGE:
	    case KEY_PPAGE:
		do_scroll = TRUE;
		move_by = -form_height;
		break;

	    case M_EVENT + KEY_NPAGE:
	    case KEY_NPAGE:
		do_scroll = TRUE;
		move_by = form_height;
		break;

	    case KEY_ENTER:
		dlg_del_window(dialog);
		result = (state >= 0) ? dlg_ok_buttoncode(state) : DLG_EXIT_OK;
		continue;

	    case KEY_LEFT:
		if (state == sTEXT)
		    break;
		/* FALLTHRU */
	    case KEY_UP:
	    case KEY_PREVIOUS:
		if (state == sTEXT) {
		    do_tab = TRUE;
		    move_by = -1;
		    break;
		} else {
		    state = dlg_prev_ok_buttonindex(state, 0);
		    show_buttons = TRUE;
		    continue;
		}

	    case KEY_BTAB:
		state = dlg_prev_ok_buttonindex(state, sTEXT);
		show_buttons = TRUE;
		continue;

	    case KEY_RIGHT:
		if (state == sTEXT)
		    break;
		/* FALLTHRU */

	    case KEY_DOWN:
	    case KEY_NEXT:
		if (state == sTEXT) {
		    do_tab = TRUE;
		    move_by = 1;
		    break;
		} else {
		    state = dlg_next_ok_buttonindex(state, 0);
		    show_buttons = TRUE;
		    continue;
		}

	    default:
#if USE_MOUSE
		if (key >= M_EVENT) {
		    if (key >= M_EVENT + KEY_MAX) {
			int cell = key - (M_EVENT + KEY_MAX);
			int row = (cell / getmaxx(form)) + scrollamt;
			int col = (cell % getmaxx(form));
			int n;

			for (n = 0; n < item_no; ++n) {
			    if (elt[n].name_y == row
				&& elt[n].name_x <= col
				&& (elt[n].name_x + elt[n].name_len > col
				    || (elt[n].name_y == elt[n].text_y
					&& elt[n].text_x > col))) {
				field_changed = TRUE;
				break;
			    }
			    if (elt[n].text_y == row
				&& elt[n].text_x <= col
				&& elt[n].text_x + elt[n].text_ilen > col) {
				field_changed = TRUE;
				break;
			    }
			}
			if (field_changed) {
			    print_item(form, elt + choice, scrollamt, FALSE);
			    choice = n;
			    continue;
			}
			beep();
		    } else if ((code = dlg_ok_buttoncode(key - M_EVENT)) >= 0) {
			result = code;
		    }
		    continue;
		}
#endif
		break;
	    }

	    new_scroll = scrollamt;
	    new_choice = choice;
	    if (do_scroll) {
		if (scroll_next(form, elt, move_by, &new_choice, &new_scroll)) {
		    if (choice != new_choice) {
			choice = new_choice;
			field_changed = TRUE;
		    }
		    if (scrollamt != new_scroll) {
			scrollamt = new_scroll;
			scroll_changed = TRUE;
		    }
		}
		continue;
	    }
	    if (do_tab) {
		if (tab_next(form, elt, item_no, move_by, &new_choice, &new_scroll)) {
		    if (choice != new_choice) {
			choice = new_choice;
			field_changed = TRUE;
		    }
		    if (scrollamt != new_scroll) {
			scrollamt = new_scroll;
			scroll_changed = TRUE;
		    }
		}
		continue;
	    }
	}

	if (state == sTEXT) {	/* Input box selected */
	    edit = dlg_edit_string(current->text, &chr_offset, key, fkey, first);
	    if (edit) {
		dlg_show_string(form, current->text, chr_offset,
				form_active_text_attr,
				current->text_y - scrollamt,
				current->text_x,
				current->text_len, password, first);
		continue;
	    }
	}

    }

    switch (result) {
    case DLG_EXIT_OK:		/* FALLTHRU */
    case DLG_EXIT_EXTRA:
	show_status = TRUE;
	break;
    case DLG_EXIT_HELP:
	dlg_add_result("HELP ");
	show_status = dialog_vars.help_status;
	if (USE_ITEM_HELP(ItemHelp(scrollamt + choice))) {
	    dlg_add_result(ItemHelp(scrollamt + choice));
	    result = DLG_EXIT_ITEM_HELP;
	} else {
	    dlg_add_result(ItemName(scrollamt + choice));
	}
	if (show_status)
	    dlg_add_result("\n");
	break;
    }
    if (show_status) {
	for (i = 0; i < item_no; i++) {
	    if (elt[i].text_flen > 0) {
		dlg_add_result(elt[i].text);
		dlg_add_result("\n");
		free(elt[i].text);
	    }
	}
    }

    free(elt);

    dlg_mouse_free_regions();
    dlg_del_window(dialog);
    free(prompt);
    return result;
}
