/*
 *  menubox.c -- implements the menu box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
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

#define LLEN(n) ((n) * 2)

/*
 * Print menu item
 */
static void
print_item(WINDOW *win, const char *tag, const char *item,
    int choice, int selected)
{
    int i;

    /* Clear 'residue' of last item */
    wattrset(win, menubox_attr);
    wmove(win, choice, 0);
    for (i = 0; i < menu_width; i++)
	waddch(win, ' ');
    wmove(win, choice, tag_x);
    wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
    waddch(win, tag[0]);
    wattrset(win, selected ? tag_selected_attr : tag_attr);
    waddstr(win, tag + 1);
    wmove(win, choice, item_x);
    wattrset(win, selected ? item_selected_attr : item_attr);
    waddstr(win, item);
}

/*
 * Display a menu for choosing among a number of options
 */
int
dialog_menu(const char *title, const char *cprompt, int height, int width,
    int menu_height, int item_no, char **items)
{
    int i, j, x, y, cur_x, cur_y, box_x, box_y;
    int key = 0, button = 0, choice = 0, scrollamt = 0, max_choice, min_width;
    int found;
    WINDOW *dialog, *menu;
    char *prompt = strclone(cprompt);
    const char **buttons = dlg_ok_labels();

    tab_correct_str(prompt);
    if (menu_height == 0) {
	min_width = calc_listw(item_no, items, 2) + 10;
	/* calculate height without items (4) */
	prompt = auto_size(title, prompt, &height, &width, 4, MAX(26, min_width));
	calc_listh(&height, &menu_height, item_no);
    } else
	prompt = auto_size(title, prompt, &height, &width, 4 + menu_height, 26);
    print_size(height, width);
    ctl_size(height, width);

    max_choice = MIN(menu_height, item_no);

    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

    dialog = new_window(height, width, y, x);

    mouse_setbase(x, y);

    draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    draw_bottom_box(dialog);
    draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    print_autowrap(dialog, prompt, width, 1, 2);

    menu_width = width - 6;
    getyx(dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - menu_width) / 2 - 1;

    /* create new window for the menu */
    menu = subwin(dialog, menu_height, menu_width, y + box_y + 1,
	x + box_x + 1);
    keypad(menu, TRUE);

    /* draw a box around the menu items */
    draw_box(dialog, box_y, box_x, menu_height + 2, menu_width + 2,
	menubox_border_attr, menubox_attr);

    tag_x = 0;
    item_x = 0;
    /* Find length of longest item in order to center menu */
    for (i = 0; i < item_no; i++) {
	tag_x = MAX(tag_x, (int) strlen(items[LLEN(i) + 1]));
	item_x = MAX(item_x, (int) strlen(items[LLEN(i)]));
    }
    tag_x = (menu_width - tag_x - item_x - 2) / 2;
    item_x = tag_x + item_x + 2;

    /* Print the menu */
    for (i = 0; i < max_choice; i++)
	print_item(menu, items[LLEN(i)], items[LLEN(i) + 1], i, i == choice);
    wnoutrefresh(menu);

    /* register the new window, along with its borders */
    mouse_mkbigregion(box_y, box_x, menu_height + 2, menu_width + 2,
	item_no, item_x,	/* the threshold */
	1 /* dirty mode */ );

    dlg_draw_arrows(dialog, scrollamt,
	scrollamt + max_choice < item_no,
	box_x + tag_x + 1,
	box_y,
	box_y + menu_height + 1);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);

    wtimeout(dialog, WTIMEOUT_VAL);

    while (key != ESC) {
	key = mouse_wgetch(dialog);
	/* Check if key pressed matches first character of any
	   item tag in menu */
	for (i = 0; i < max_choice; i++)
	    if (toupper(key) == toupper(items[LLEN(scrollamt + i)][0]))
		break;

	/*
	 * Check if key pressed matches first character of any item tag in
	 * list.  If there is more than one match, we will cycle through
	 * each one as the same key is pressed repeatedly.
	 */
	found = FALSE;
	for (j = scrollamt + choice + 1; j < item_no; j++) {
	    if (toupper(key) ==
		toupper(items[LLEN(j)][0])) {
		found = TRUE;
		i = j - scrollamt;
		break;
	    }
	}
	if (!found) {
	    for (j = 0; j <= scrollamt + choice; j++) {
		if (toupper(key) ==
		    toupper(items[LLEN(j)][0])) {
		    found = TRUE;
		    i = j - scrollamt;
		    break;
		}
	    }
	}

	/*
	 * A single digit (1-9) positions the selection to that line in the
	 * current screen.
	 */
	if (!found
	    && (key <= '9')
	    && (key > '0')
	    && (key - '1' <= max_choice)) {
	    found = TRUE;
	    i = key - '1';
	}

	if (!found) {
	    found = TRUE;
	    switch (key) {
	    case KEY_HOME:
		i = -scrollamt;
		break;
	    case KEY_LL:
	    case KEY_END:
		i = item_no - 1 - scrollamt;
		break;
	    case KEY_PPAGE:
		if (choice)
		    i = 0;
		else if (scrollamt != 0)
		    i = -MIN(scrollamt, max_choice);
		else
		    continue;
		break;
	    case KEY_NPAGE:
		i = MIN(choice + max_choice, item_no - scrollamt - 1);
		break;
	    case KEY_UP:
	    case '-':
		i = choice - 1;
		if (choice == 0 && scrollamt == 0)
		    continue;
		break;
	    case KEY_DOWN:
	    case '+':
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
			    print_item(menu,
				items[LLEN(scrollamt)],
				items[LLEN(scrollamt) + 1],
				0, FALSE);
			    scrollok(menu, TRUE);
			    wscrl(menu, -1);
			    scrollok(menu, FALSE);
			}
			scrollamt--;
			print_item(menu,
			    items[LLEN(scrollamt)],
			    items[LLEN(scrollamt) + 1],
			    0, TRUE);
		    } else if (i == max_choice) {
			if (menu_height > 1) {
			    /* De-highlight current last item before scrolling up */
			    print_item(menu,
				items[LLEN(scrollamt + max_choice - 1)],
				items[LLEN(scrollamt + max_choice - 1) + 1],
				max_choice - 1, FALSE);
			    scrollok(menu, TRUE);
			    wscrl(menu, 1);
			    scrollok(menu, FALSE);
			}
			scrollamt++;
			print_item(menu,
			    items[LLEN(scrollamt + max_choice - 1)],
			    items[LLEN(scrollamt + max_choice - 1) + 1],
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
				items[LLEN(scrollamt + i)],
				items[LLEN(scrollamt + i) + 1],
				i, i == choice);
			}
		    }
		    wnoutrefresh(menu);
		    dlg_draw_arrows(dialog, scrollamt,
			scrollamt + choice < item_no - 1,
			box_x + tag_x + 1,
			box_y,
			box_y + menu_height + 1);
		} else {
		    /* De-highlight current item */
		    print_item(menu,
			items[LLEN(scrollamt + choice)],
			items[LLEN(scrollamt + choice) + 1],
			choice, FALSE);
		    /* Highlight new item */
		    choice = i;
		    print_item(menu,
			items[LLEN(scrollamt + choice)],
			items[LLEN(scrollamt + choice) + 1],
			choice, TRUE);
		    wnoutrefresh(menu);
		    wmove(dialog, cur_y, cur_x);
		    wrefresh_lock(dialog);
		}
	    }
	    continue;		/* wait for another key press */
	}

	switch (key) {
	case 'O':
	case 'o':
	case M_EVENT + 'O':
	    delwin(dialog);
	    return scrollamt + choice;
	case 'C':
	case 'c':
	case M_EVENT + 'C':
	    delwin(dialog);
	    return -2;
	case M_EVENT + 'o':	/* mouse enter... */
	case M_EVENT + 'c':	/* use the code for toggling */
	    button = (key == M_EVENT + 'o');
	    /* FALLTHRU */
	case ' ':
	case KEY_BTAB:
	case TAB:
	case KEY_LEFT:
	case KEY_RIGHT:
	    if (!dialog_vars.nocancel)
		button = !button;
	    dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);
	    break;
	case '\n':
	    delwin(dialog);
	    return (button ? -2 : (scrollamt + choice));
	case ESC:
	    break;
	}
    }

    delwin(dialog);
    return -1;			/* ESC pressed */
}
