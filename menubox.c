/*
 *  $Id: menubox.c,v 1.79 2004/12/20 20:42:58 tom Exp $
 *
 *  menubox.c -- implements the menu box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
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

static int menu_width, tag_x, item_x;

typedef enum {
    Unselected = 0,
    Selected,
    Editing
} Mode;

#define INPUT_ROWS     3	/* rows per inputmenu entry */

#define LLEN(n) ((n) * MENUBOX_TAGS)
#define ItemData(i)    &items[LLEN(i)]
#define ItemName(i)    items[LLEN(i)]
#define ItemText(i)    items[LLEN(i) + 1]
#define ItemHelp(i)    items[LLEN(i) + 2]

#define RowHeight(i) (dialog_vars.input_menu ? ((i) * INPUT_ROWS) : ((i) * 1))
#define ItemToRow(i) (dialog_vars.input_menu ? ((i) * INPUT_ROWS + 1) : (i))
#define RowToItem(i) (dialog_vars.input_menu ? ((i) / INPUT_ROWS + 0) : (i))

static void
print_arrows(WINDOW *win,
	     int box_x,
	     int box_y,
	     int scrollamt,
	     int max_choice,
	     int item_no,
	     int menu_height)
{
    dlg_draw_arrows(win, scrollamt,
		    scrollamt + max_choice < item_no,
		    box_x + tag_x + 1,
		    box_y,
		    box_y + menu_height + 1);
}

/*
 * Print the tag of a menu-item
 */
static void
print_tag(WINDOW *win,
	  char **items,
	  int choice,
	  Mode selected)
{
    int my_x = item_x;
    int my_y = ItemToRow(choice);
    int tag_width = (my_x - tag_x - GUTTER);
    const int *cols;
    const int *indx;
    int limit;
    unsigned prefix;

    cols = dlg_index_columns(ItemName(0));
    indx = dlg_index_wchars(ItemName(0));
    limit = dlg_count_wchars(ItemName(0));
    prefix = indx[1] - indx[0];

    /* highlight first char of the tag to be special */
    (void) wmove(win, my_y, tag_x);
    wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
    if (strlen(ItemName(0)) != 0)
	(void) waddnstr(win, ItemName(0), prefix);
    /* print rest of the string */
    wattrset(win, selected ? tag_selected_attr : tag_attr);
    if (strlen(ItemName(0)) > prefix) {
	limit = dlg_limit_columns(ItemName(0), tag_width, 1);
	if (limit > 0)
	    (void) waddnstr(win, ItemName(0) + indx[1], indx[limit] - indx[1]);
    }
}

/*
 * Print menu item
 */
static void
print_item(WINDOW *win,
	   char **items,
	   int choice,
	   Mode selected)
{
    chtype save = getattrs(win);
    int n;
    int my_width = menu_width;
    int my_x = item_x;
    int my_y = ItemToRow(choice);
    chtype attr = A_NORMAL;
    chtype textchar;
    chtype bordchar;

    if (items == 0)
	return;

    switch (selected) {
    default:
    case Unselected:
	textchar = item_attr;
	bordchar = item_attr;
	break;
    case Selected:
	textchar = item_selected_attr;
	bordchar = item_selected_attr;
	break;
    case Editing:
	textchar = inputbox_attr;
	bordchar = dialog_attr;
	break;
    }

    /* Clear 'residue' of last item and mark current current item */
    if (dialog_vars.input_menu) {
	wattrset(win, (selected != Unselected) ? item_selected_attr : item_attr);
	for (n = my_y - 1; n < my_y + INPUT_ROWS - 1; n++) {
	    wmove(win, n, 0);
	    wprintw(win, "%*s", my_width, " ");
	}
    } else {
	wattrset(win, menubox_attr);
	wmove(win, my_y, 0);
	wprintw(win, "%*s", my_width, " ");
    }

    print_tag(win, items, choice, selected);

    /* Draw the input field box (only for inputmenu) */
    (void) wmove(win, my_y, my_x);
    if (dialog_vars.input_menu) {
	my_width -= 1;
	dlg_draw_box(win, my_y - 1, my_x, INPUT_ROWS, my_width - my_x - tag_x,
		     bordchar,
		     bordchar);
	my_width -= 1;
	++my_x;
    }

    /* print actual item */
    wmove(win, my_y, my_x);
    wattrset(win, textchar);
    dlg_print_text(win, ItemText(0), my_width - my_x, &attr);

    if (selected) {
	dlg_item_help(ItemHelp(0));
    }
    wattrset(win, save);
}

/*
 * Allow the user to edit the text of a menu entry.
 */
static int
input_menu_edit(WINDOW *win, char **items, int choice, char **resultp)
{
    chtype save = getattrs(win);
    char *result;
    int offset = 0;
    int key = 0, fkey = 0;
    int first = TRUE;
    /* see above */
    int y = ItemToRow(choice);
    int code = TRUE;

    result = malloc(dialog_vars.max_input);
    assert_ptr(result, "input_menu_edit");

    /* original item is used to initialize the input string. */
    result[0] = '\0';
    strcpy(result, ItemText(0));

    print_item(win, items, choice, Editing);

    /* taken out of inputbox.c - but somewhat modified */
    for (;;) {
	if (!first)
	    key = dlg_mouse_wgetch(win, &fkey);
	if (dlg_edit_string(result, &offset, key, fkey, first)) {
	    /* 
	     * menu_width - 2 ..... it's the actual number of maximal
	     *                      possible characters could be written
	     *                      to the screen.
	     *
	     * item_x - tag_x - 2 . same as "name_width"
	     *                      ( see in dialog_menu() )
	     */
	    dlg_show_string(win, result, offset, inputbox_attr,
			    y, item_x + 1, menu_width - item_x - 3,
			    FALSE, first);
	    first = FALSE;
	} else if (key == ESC || key == TAB) {
	    code = FALSE;
	    break;
	} else {
	    break;
	}
    }
    print_item(win, items, choice, Selected);
    wattrset(win, save);

    *resultp = result;
    return code;
}

static int
handle_button(int code, char **items, int choice)
{
    switch (code) {
    case DLG_EXIT_OK:		/* FALLTHRU */
    case DLG_EXIT_EXTRA:
	dlg_add_result(ItemName(choice));
	break;
    case DLG_EXIT_HELP:
	dlg_add_result("HELP ");
	if (USE_ITEM_HELP(ItemHelp(choice))) {
	    dlg_add_result(ItemHelp(choice));
	    code = DLG_EXIT_ITEM_HELP;
	} else {
	    dlg_add_result(ItemName(choice));
	}
	break;
    }
    return code;
}

/*
 * Display a menu for choosing among a number of options
 */
int
dialog_menu(const char *title, const char *cprompt, int height, int width,
	    int menu_height, int item_no, char **items)
{
    int i, j, x, y, cur_x, cur_y, box_x, box_y;
    int key = 0, fkey;
    int button = dialog_state.visit_items ? -1 : dlg_defaultno_button();
    int choice = dlg_default_item(items, MENUBOX_TAGS);
    int result = DLG_EXIT_UNKNOWN;
    int scrollamt = 0;
    int max_choice, min_width;
    int found;
    int use_width, name_width, text_width;
    WINDOW *dialog, *menu;
    char *prompt = dlg_strclone(cprompt);
    const char **buttons = dlg_ok_labels();

    dlg_tab_correct_str(prompt);
    if (menu_height == 0) {
	min_width = dlg_calc_listw(item_no, items, MENUBOX_TAGS) + 10;
	/* calculate height without items (4) */
	dlg_auto_size(title, prompt, &height, &width, 4, MAX(26, min_width));
	dlg_calc_listh(&height, &menu_height, item_no);
    } else {
	dlg_auto_size(title, prompt, &height, &width, 4 + menu_height, 26);
    }
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    /* Find out maximal number of displayable items at once. */
    max_choice = MIN(menu_height,
		     RowHeight(item_no));
    if (dialog_vars.input_menu)
	max_choice /= INPUT_ROWS;

    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);

    dlg_mouse_setbase(x, y);

    dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    dlg_draw_bottom_box(dialog);
    dlg_draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    dlg_print_autowrap(dialog, prompt, height, width);

    menu_width = width - 6;
    getyx(dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - menu_width) / 2 - 1;

    /* create new window for the menu */
    menu = dlg_sub_window(dialog, menu_height, menu_width,
			  y + box_y + 1,
			  x + box_x + 1);

    /* draw a box around the menu items */
    dlg_draw_box(dialog, box_y, box_x, menu_height + 2, menu_width + 2,
		 menubox_border_attr, menubox_attr);

    name_width = 0;
    text_width = 0;

    /* Find length of longest item to center menu  *
     * only if --menu was given, using --inputmenu *
     * won't be centered.                         */
    for (i = 0; i < item_no; i++) {
	name_width = MAX(name_width, dlg_count_columns(ItemName(i)));
	text_width = MAX(text_width, dlg_count_columns(ItemText(i)));
    }

    /* If the name+text is wider than the list is allowed, then truncate
     * one or both of them.  If the name is no wider than 30% of the list,
     * leave it intact.
     *
     * FIXME: the gutter width and name/list ratio should be configurable.
     */
    use_width = (menu_width - GUTTER);
    if (text_width + name_width > use_width) {
	int need = (int) (0.30 * use_width);
	if (name_width > need) {
	    int want = (int) (use_width
			      * ((double) name_width)
			      / (text_width + name_width));
	    name_width = (want > need) ? want : need;
	}
	text_width = use_width - name_width;
    }

    tag_x = (dialog_vars.input_menu
	     ? 0
	     : (use_width - text_width - name_width) / 2);
    item_x = name_width + tag_x + GUTTER;

    if (choice - scrollamt >= max_choice) {
	scrollamt = choice - (max_choice - 1);
	choice = max_choice - 1;
    }

    /* Print the menu */
    for (i = 0; i < max_choice; i++) {
	print_item(menu,
		   ItemData(i + scrollamt),
		   i,
		   (i == choice) ? Selected : Unselected);
    }
    (void) wnoutrefresh(menu);

    /* register the new window, along with its borders */
    dlg_mouse_mkbigregion(box_y + 1, box_x, menu_height + 2, menu_width + 2,
			  KEY_MAX, 1, 1, 1 /* by lines */ );

    print_arrows(dialog, box_x, box_y, scrollamt, max_choice, item_no, menu_height);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);

    while (result == DLG_EXIT_UNKNOWN) {
	if (button < 0)		/* --visit-items */
	    wmove(dialog, box_y + ItemToRow(choice) + 1, box_x + tag_x + 1);

	key = dlg_mouse_wgetch(dialog, &fkey);

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
	    case ' ':
	    case TAB:
		key = KEY_RIGHT;
		break;
	    case ESC:
		result = DLG_EXIT_ESC;
		continue;
	    default:
		fkey = FALSE;
		break;
	    }
	}

	found = FALSE;
	if (fkey) {
	    /*
	     * Allow a mouse-click on a box to switch selection to that box.
	     * Handling a button click is a little more complicated, since we
	     * push a KEY_ENTER back onto the input stream so we'll put the
	     * cursor at the right place before handling the "keypress".
	     */
	    if (key >= (M_EVENT + KEY_MAX)) {
		key -= (M_EVENT + KEY_MAX);
		i = RowToItem(key);
		if (scrollamt + i < max_choice) {
		    found = TRUE;
		} else {
		    beep();
		    continue;
		}
	    } else if (key >= M_EVENT
		       && dlg_ok_buttoncode(key - M_EVENT) >= 0) {
		button = (key - M_EVENT);
		ungetch('\n');
		continue;
	    }
	} else {
	    /*
	     * Check if key pressed matches first character of any item tag in
	     * list.  If there is more than one match, we will cycle through
	     * each one as the same key is pressed repeatedly.
	     */
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
	}

	if (!found && fkey) {
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
			if (menu_height > 1) {
			    /* De-highlight current first item */
			    print_item(menu, ItemData(scrollamt), 0, Unselected);
			    scrollok(menu, TRUE);
			    wscrl(menu, -RowHeight(1));
			    scrollok(menu, FALSE);
			}
			scrollamt--;
			print_item(menu, ItemData(scrollamt), 0, Selected);
		    } else if (i == max_choice) {
			if (menu_height > 1) {
			    /* De-highlight current last item before scrolling up */
			    print_item(menu,
				       ItemData(scrollamt + max_choice - 1),
				       max_choice - 1,
				       Unselected);
			    scrollok(menu, TRUE);
			    wscrl(menu, RowHeight(1));
			    scrollok(menu, FALSE);
			}
			scrollamt++;
			print_item(menu,
				   ItemData(scrollamt + max_choice - 1),
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
			    print_item(menu,
				       ItemData(scrollamt + i),
				       i,
				       (i == choice) ? Selected : Unselected);
			}
		    }
		    /* Clean bottom lines */
		    if (dialog_vars.input_menu) {
			int spare_lines, x_count;
			spare_lines = menu_height % INPUT_ROWS;
			wattrset(menu, menubox_attr);
			for (; spare_lines; spare_lines--) {
			    wmove(menu, menu_height - spare_lines, 0);
			    for (x_count = 0; x_count < menu_width;
				 x_count++) {
				waddch(menu, ' ');
			    }
			}
		    }
		    (void) wnoutrefresh(menu);
		    print_arrows(dialog,
				 box_x, box_y,
				 scrollamt, max_choice, item_no, menu_height);
		} else {
		    /* De-highlight current item */
		    print_item(menu,
			       ItemData(scrollamt + choice),
			       choice,
			       Unselected);
		    /* Highlight new item */
		    choice = i;
		    print_item(menu,
			       ItemData(scrollamt + choice),
			       choice,
			       Selected);
		    (void) wnoutrefresh(menu);
		    (void) wmove(dialog, cur_y, cur_x);
		    wrefresh(dialog);
		}
	    }
	    continue;		/* wait for another key press */
	}

	if (fkey) {
	    switch (key) {
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
	    case KEY_ENTER:
		result = handle_button(dlg_ok_buttoncode(button),
				       items,
				       scrollamt + choice);

		if (dialog_vars.input_menu && result == DLG_EXIT_EXTRA) {
		    char *tmp;

		    if (input_menu_edit(menu,
					ItemData(scrollamt + choice),
					choice,
					&tmp)) {
			dialog_vars.input_result[0] = '\0';
			dlg_add_result("RENAMED ");
			dlg_add_result(ItemName(scrollamt + choice));
			dlg_add_result(" ");
			dlg_add_result(tmp);
		    } else {
			result = DLG_EXIT_UNKNOWN;
			print_item(menu,
				   ItemData(scrollamt + choice),
				   choice,
				   Selected);
			(void) wnoutrefresh(menu);
		    }
		    free(tmp);

		    dlg_draw_buttons(dialog, height - 2, 0,
				     buttons, button, FALSE, width);
		}
		break;
	    default:
		flash();
		break;
	    }
	}
    }

    dlg_mouse_free_regions();
    dlg_del_window(dialog);
    free(prompt);
    return result;
}
