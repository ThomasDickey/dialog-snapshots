/*
 *  $Id: checklist.c,v 1.83 2004/12/20 20:42:58 tom Exp $
 *
 *  checklist.c -- implements the checklist box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *     Stuart Herbert - S.Herbert@sheffield.ac.uk: radiolist extension
 *     Alessandro Rubini - rubini@ipvvis.unipv.it: merged the two
 *     Thomas E. Dickey - rewrote...
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

static int list_width, check_x, item_x, checkflag;

#define LLEN(n) ((n) * CHECKBOX_TAGS)
#define ItemData(i)    &items[LLEN(i)]
#define ItemName(i)    items[LLEN(i)]
#define ItemText(i)    items[LLEN(i) + 1]
#define ItemStatus(i)  items[LLEN(i) + 2]
#define ItemHelp(i)    items[LLEN(i) + 3]

static void
print_arrows(WINDOW *win,
	     int box_x,
	     int box_y,
	     int scrollamt,
	     int choice,
	     int item_no,
	     int list_height)
{
    dlg_draw_arrows2(win, scrollamt,
		     scrollamt + choice < item_no - 1,
		     box_x + check_x + 5,
		     box_y,
		     box_y + list_height + 1,
		     menubox_attr,
		     menubox_border_attr);
}

/*
 * Print list item
 */
static void
print_item(WINDOW *win, char **items, int status,
	   int choice, int selected)
{
    chtype save = getattrs(win);
    int i;
    chtype attr = A_NORMAL;
    const int *indx;
    int limit;

    /* Clear 'residue' of last item */
    wattrset(win, menubox_attr);
    (void) wmove(win, choice, 0);
    for (i = 0; i < list_width; i++)
	(void) waddch(win, ' ');

    (void) wmove(win, choice, check_x);
    wattrset(win, selected ? check_selected_attr : check_attr);
    (void) wprintw(win,
		   (checkflag == FLAG_CHECK) ? "[%c]" : "(%c)",
		   status ? 'X' : ' ');
    wattrset(win, menubox_attr);
    (void) waddch(win, ' ');

    if (strlen(ItemName(0)) != 0) {

	indx = dlg_index_wchars(ItemName(0));
	limit = dlg_count_wchars(ItemName(0));

	wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
	(void) waddnstr(win, ItemName(0), indx[1]);

	if ((int) strlen(ItemName(0)) > indx[1]) {
	    limit = dlg_limit_columns(ItemName(0), (item_x - check_x - 6), 1);
	    if (limit > 1) {
		wattrset(win, selected ? tag_selected_attr : tag_attr);
		(void) waddnstr(win,
				ItemName(0) + indx[1],
				indx[limit] - indx[1]);
	    }
	}
    }

    if (strlen(ItemText(0)) != 0) {
	indx = dlg_index_wchars(ItemText(0));
	limit = dlg_limit_columns(ItemText(0), (getmaxx(win) - item_x - 1), 0);

	if (limit > 0) {
	    (void) wmove(win, choice, item_x);
	    wattrset(win, selected ? item_selected_attr : item_attr);
	    dlg_print_text(win, ItemText(0), indx[limit], &attr);
	}
    }

    if (selected) {
	dlg_item_help(ItemHelp(0));
    }
    wattrset(win, save);
}

/*
 * Display a dialog box with a list of options that can be turned on or off
 * The `flag' parameter is used to select between radiolist and checklist.
 */
int
dialog_checklist(const char *title, const char *cprompt, int height, int width,
		 int list_height, int item_no, char **items, int flag)
{
    bool show_status = FALSE;
    bool separate_output = ((flag == FLAG_CHECK)
			    && (dialog_vars.separate_output));
    int i, j, key2, found, x, y, cur_x, cur_y, box_x, box_y;
    int key = 0, fkey;
    int button = dialog_state.visit_items ? -1 : dlg_defaultno_button();
    int choice = dlg_default_item(items, CHECKBOX_TAGS);
    int scrollamt = 0;
    int max_choice, *status;
    int use_width, name_width, text_width;
    int result = DLG_EXIT_UNKNOWN;
    WINDOW *dialog, *list;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_ok_labels();

    dlg_tab_correct_str(prompt);
    if (list_height == 0) {
	use_width = dlg_calc_listw(item_no, items, CHECKBOX_TAGS) + 10;
	/* calculate height without items (4) */
	dlg_auto_size(title, prompt, &height, &width, 4, MAX(26, use_width));
	dlg_calc_listh(&height, &list_height, item_no);
    } else {
	dlg_auto_size(title, prompt, &height, &width, 4 + list_height, 26);
    }
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    checkflag = flag;

    /* Allocate space for storing item on/off status */
    status = malloc(sizeof(int) * item_no);
    assert_ptr(status, "dialog_checklist");

    /* Initializes status */
    for (i = 0; i < item_no; i++)
	status[i] = !dlg_strcmp(ItemStatus(i), "on");

    max_choice = MIN(list_height, item_no);

    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);

    dlg_mouse_setbase(x, y);

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_bottom_box(dialog);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, height, width);

    list_width = width - 6;
    getyx(dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - list_width) / 2 - 1;

    /* create new window for the list */
    list = dlg_sub_window(dialog, list_height, list_width,
			  y + box_y + 1, x + box_x + 1);

    /* draw a box around the list items */
    dlg_draw_box(dialog, box_y, box_x,
		 list_height + 2 * MARGIN,
		 list_width + 2 * MARGIN,
		 menubox_border_attr, menubox_attr);

    text_width = 0;
    name_width = 0;
    /* Find length of longest item to center checklist */
    for (i = 0; i < item_no; i++) {
	text_width = MAX(text_width, (int) strlen(ItemText(i)));
	name_width = MAX(name_width, (int) strlen(ItemName(i)));
    }

    /* If the name+text is wider than the list is allowed, then truncate
     * one or both of them.  If the name is no wider than 1/4 of the list,
     * leave it intact.
     */
    use_width = (list_width - 6);
    if (text_width + name_width > use_width) {
	int need = 0.25 * use_width;
	if (name_width > need) {
	    int want = use_width * ((double) name_width) / (text_width + name_width);
	    name_width = (want > need) ? want : need;
	}
	text_width = use_width - name_width;
    }

    check_x = (use_width - (text_width + name_width)) / 2;
    item_x = name_width + check_x + 6;

    /* Print the list */
    if (choice >= (max_choice + scrollamt)) {
	scrollamt = choice - max_choice + 1;
	choice = max_choice - 1;
    }
    for (i = 0; i < max_choice; i++)
	print_item(list,
		   ItemData(i + scrollamt),
		   status[i + scrollamt], i, i == choice);
    (void) wnoutrefresh(list);

    /* register the new window, along with its borders */
    dlg_mouse_mkbigregion(box_y + 1, box_x, list_height, list_width + 2,
			  KEY_MAX, 1, 1, 1 /* by lines */ );

    print_arrows(dialog,
		 box_x, box_y,
		 scrollamt, choice, item_no, list_height);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);

    while (result == DLG_EXIT_UNKNOWN) {
	if (button < 0)		/* --visit-items */
	    wmove(dialog, box_y + choice + 1, box_x + check_x + 2);

	key = dlg_mouse_wgetch(dialog, &fkey);

	if (fkey && (key >= (M_EVENT + KEY_MAX))) {
	    getyx(dialog, cur_y, cur_x);
	    i = (key - (M_EVENT + KEY_MAX));
	    if (scrollamt + i < max_choice) {
		/* De-highlight current item */
		print_item(list,
			   ItemData(scrollamt + choice),
			   status[scrollamt + choice], choice, FALSE);
		/* Highlight new item */
		choice = (key - (M_EVENT + KEY_MAX));
		print_item(list,
			   ItemData(scrollamt + choice),
			   status[scrollamt + choice], choice, TRUE);
		(void) wnoutrefresh(list);
		(void) wmove(dialog, cur_y, cur_x);

		key = ' ';	/* force the selected item to toggle */
	    } else {
		beep();
		continue;
	    }
	    fkey = FALSE;
	}

	/*
	 * A space toggles the item status.  We handle either a checklist
	 * (any number of items can be selected) or radio list (zero or one
	 * items can be selected).
	 */
	if (key == ' ') {
	    getyx(dialog, cur_y, cur_x);
	    if (flag == FLAG_CHECK) {	/* checklist? */
		status[scrollamt + choice] = !status[scrollamt + choice];
		print_item(list,
			   ItemData(scrollamt + choice),
			   status[scrollamt + choice], choice, TRUE);
	    } else {		/* radiolist */
		if (!status[scrollamt + choice]) {
		    for (i = 0; i < item_no; i++)
			status[i] = FALSE;
		    status[scrollamt + choice] = TRUE;
		    for (i = 0; i < max_choice; i++)
			print_item(list,
				   ItemData(scrollamt + i),
				   status[scrollamt + i], i, i == choice);
		}
	    }
	    (void) wnoutrefresh(list);
	    (void) wmove(dialog, cur_y, cur_x);
	    wrefresh(dialog);
	    continue;		/* wait for another key press */
	} else if (key == ESC) {
	    result = DLG_EXIT_ESC;
	    continue;
	}

	if (!fkey) {
	    fkey = TRUE;
	    switch (key) {
	    case '\n':
	    case '\r':
		key = KEY_ENTER;
		break;
	    case '-':
		key = KEY_UP;
		break;
	    case '+':
		key = KEY_DOWN;
		break;
	    case TAB:
		key = KEY_RIGHT;
		break;
	    default:
		fkey = FALSE;
		break;
	    }
	}

	/*
	 * Check if key pressed matches first character of any item tag in
	 * list.  If there is more than one match, we will cycle through
	 * each one as the same key is pressed repeatedly.
	 */
	found = FALSE;
	if (!fkey) {
	    if (button < 0 || !dialog_state.visit_items) {
		for (j = scrollamt + choice + 1; j < item_no; j++) {
		    if (dlg_match_char(dlg_last_getc(), ItemName(j))) {
			found = TRUE;
			i = j - scrollamt;
			break;
		    }
		}
		if (!found) {
		    for (j = 0; j <= scrollamt + choice; j++) {
			if (dlg_match_char(dlg_last_getc(), ItemName(j))) {
			    found = TRUE;
			    i = j - scrollamt;
			    break;
			}
		    }
		}
		if (found)
		    dlg_flush_getc();
	    } else if ((j = dlg_char_to_button(key, buttons)) >= 0) {
		button = j;
		ungetch('\n');
		continue;
	    }
	}

	/*
	 * A single digit (1-9) positions the selection to that line in the
	 * current screen.
	 */
	if (!found
	    && (key <= '9')
	    && (key > '0')
	    && (key - '1' < max_choice)) {
	    found = TRUE;
	    i = key - '1';
	}

	if (!found) {
	    if (fkey) {
		found = TRUE;
		switch (key) {
		case KEY_HOME:
		    i = -scrollamt;
		    break;
		case KEY_LL:
		case KEY_END:
		    i = item_no - 1 - scrollamt;
		    break;
		case M_EVENT + KEY_PPAGE:
		case KEY_PPAGE:
		    if (choice)
			i = 0;
		    else if (scrollamt != 0)
			i = -MIN(scrollamt, max_choice);
		    else
			continue;
		    break;
		case M_EVENT + KEY_NPAGE:
		case KEY_NPAGE:
		    i = MIN(choice + max_choice, item_no - scrollamt - 1);
		    break;
		case KEY_UP:
		    i = choice - 1;
		    if (choice == 0 && scrollamt == 0)
			continue;
		    break;
		case KEY_DOWN:
		    i = choice + 1;
		    if (scrollamt + choice >= item_no - 1)
			continue;
		    break;
		default:
		    found = FALSE;
		    break;
		}
	    }
	}

	if (found) {
	    if (i != choice) {
		getyx(dialog, cur_y, cur_x);
		if (i < 0 || i >= max_choice) {
#if defined(NCURSES_VERSION_MAJOR) && NCURSES_VERSION_MAJOR < 5
		    /*
		     * Using wscrl to assist ncurses scrolling is not needed
		     * in version 5.x
		     */
		    if (i == -1) {
			if (list_height > 1) {
			    /* De-highlight current first item */
			    print_item(list,
				       ItemData(scrollamt),
				       status[scrollamt], 0, FALSE);
			    scrollok(list, TRUE);
			    wscrl(list, -1);
			    scrollok(list, FALSE);
			}
			scrollamt--;
			print_item(list,
				   ItemData(scrollamt),
				   status[scrollamt], 0, TRUE);
		    } else if (i == max_choice) {
			if (list_height > 1) {
			    /* De-highlight current last item before scrolling up */
			    print_item(list,
				       ItemData(scrollamt + max_choice - 1),
				       status[scrollamt + max_choice - 1],
				       max_choice - 1, FALSE);
			    scrollok(list, TRUE);
			    wscrl(list, 1);
			    scrollok(list, FALSE);
			}
			scrollamt++;
			print_item(list,
				   ItemData(scrollamt + max_choice - 1),
				   status[scrollamt + max_choice - 1],
				   max_choice - 1, TRUE);
		    } else
#endif
		    {
			if (i < 0) {
			    scrollamt += i;
			    choice = 0;
			} else {
			    choice = max_choice - 1;
			    scrollamt += (i - max_choice + 1);
			}
			for (i = 0; i < max_choice; i++) {
			    print_item(list,
				       ItemData(scrollamt + i),
				       status[scrollamt + i], i, i == choice);
			}
		    }
		    (void) wnoutrefresh(list);
		    print_arrows(dialog,
				 box_x, box_y,
				 scrollamt, choice, item_no, list_height);
		} else {
		    /* De-highlight current item */
		    print_item(list,
			       ItemData(scrollamt + choice),
			       status[scrollamt + choice], choice, FALSE);
		    /* Highlight new item */
		    choice = i;
		    print_item(list,
			       ItemData(scrollamt + choice),
			       status[scrollamt + choice], choice, TRUE);
		    (void) wnoutrefresh(list);
		    (void) wmove(dialog, cur_y, cur_x);
		    wrefresh(dialog);
		}
	    }
	    continue;		/* wait for another key press */
	}

	if (fkey) {
	    switch (key) {
	    case KEY_ENTER:
		result = dlg_ok_buttoncode(button);
		break;
	    case KEY_BTAB:
	    case KEY_LEFT:
		button = dlg_prev_button(buttons, button);
		dlg_draw_buttons(dialog, height - 2, 0, buttons, button,
				 FALSE, width);
		break;
	    case KEY_RIGHT:
		button = dlg_next_button(buttons, button);
		dlg_draw_buttons(dialog, height - 2, 0, buttons, button,
				 FALSE, width);
		break;
	    default:
		if (key >= M_EVENT) {
		    if ((key2 = dlg_ok_buttoncode(key - M_EVENT)) >= 0) {
			result = key2;
			break;
		    }
		    beep();
		}
	    }
	} else {
	    beep();
	}
    }

    dlg_del_window(dialog);
    switch (result) {
    case DLG_EXIT_OK:		/* FALLTHRU */
    case DLG_EXIT_EXTRA:
	show_status = TRUE;
	break;
    case DLG_EXIT_HELP:
	dlg_add_result("HELP ");
	show_status = dialog_vars.help_status;
	if (USE_ITEM_HELP(ItemHelp(scrollamt + choice))) {
	    if (show_status) {
		if (separate_output) {
		    dlg_add_result(ItemHelp(scrollamt + choice));
		    dlg_add_result("\n");
		} else {
		    dlg_add_quoted(ItemHelp(scrollamt + choice));
		}
	    } else {
		dlg_add_result(ItemHelp(scrollamt + choice));
	    }
	    result = DLG_EXIT_ITEM_HELP;
	} else {
	    if (show_status) {
		if (separate_output) {
		    dlg_add_result(ItemName(scrollamt + choice));
		    dlg_add_result("\n");
		} else {
		    dlg_add_quoted(ItemName(scrollamt + choice));
		}
	    } else {
		dlg_add_result(ItemName(scrollamt + choice));
	    }
	}
	break;
    }

    if (show_status) {
	for (i = 0; i < item_no; i++) {
	    if (status[i]) {
		if (separate_output) {
		    dlg_add_result(ItemName(i));
		    dlg_add_result("\n");
		} else {
		    if (*(dialog_vars.input_result))
			dlg_add_result(" ");
		    if (flag == FLAG_CHECK) {
			dlg_add_quoted(ItemName(i));
		    } else {
			dlg_add_result(ItemName(i));
		    }
		}
	    }
	}
    }
    dlg_mouse_free_regions();
    free(status);
    free(prompt);
    return result;
}
