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

/*
 * Print menu item
 */
static void
print_item (WINDOW * win, const char *tag, const char *item,
		int choice, int selected)
{
    int i;

    /* Clear 'residue' of last item */
    wattrset (win, menubox_attr);
    wmove (win, choice, 0);
    for (i = 0; i < menu_width; i++)
	waddch (win, ' ');
    wmove (win, choice, tag_x);
    wattrset (win, selected ? tag_key_selected_attr : tag_key_attr);
    waddch (win, tag[0]);
    wattrset (win, selected ? tag_selected_attr : tag_attr);
    waddstr (win, tag + 1);
    wmove (win, choice, item_x);
    wattrset (win, selected ? item_selected_attr : item_attr);
    waddstr (win, item);
}

/*
 * Display a menu for choosing among a number of options
 */
int
dialog_menu (const char *title, const char *cprompt, int height, int width,
		int menu_height, int item_no, const char * const * items)
{
    int i, x, y, cur_x, cur_y, box_x, box_y;
    int key = 0, button = 0, choice = 0, scroll = 0, max_choice, min_width;
    WINDOW *dialog, *menu;
    char *prompt=strclone(cprompt);

    tab_correct_str(prompt);
    if (menu_height == 0) {
      min_width=calc_listw(item_no, items, 2)+10;
      /* calculate height without items (4) */
      prompt=auto_size(prompt, &height, &width, 4, MAX(26, min_width));
      calc_listh(&height, &menu_height, item_no);
    } else
      prompt=auto_size(prompt, &height, &width, 4+menu_height, 26);
    print_size(height, width);
    ctl_size(height, width);

    max_choice = MIN(menu_height, item_no);

    if (begin_set == 1) {
      x = begin_x;
      y = begin_y;
    } else {
      /* center dialog box on screen */
      x = (SCOLS - width)/2;
      y = (SLINES - height)/2;
    }

#ifdef HAVE_NCURSES
    if (use_shadow)
	draw_shadow (stdscr, y, x, height, width);
#endif
    dialog = newwin (height, width, y, x);
    mouse_setbase (x, y);
    keypad (dialog, TRUE);

    draw_box (dialog, 0, 0, height, width, dialog_attr, border_attr);
    wattrset (dialog, border_attr);
    wmove (dialog, height - 3, 0);
    waddch (dialog, ACS_LTEE);
    for (i = 0; i < width - 2; i++)
	waddch (dialog, ACS_HLINE);
    wattrset (dialog, dialog_attr);
    waddch (dialog, ACS_RTEE);
    wmove (dialog, height - 2, 1);
    for (i = 0; i < width - 2; i++)
	waddch (dialog, ' ');

    if (title != NULL) {
	wattrset (dialog, title_attr);
	wmove (dialog, 0, (width - strlen (title)) / 2 - 1);
	waddch (dialog, ' ');
	waddstr (dialog, title);
	waddch (dialog, ' ');
    }
    wattrset (dialog, dialog_attr);
    print_autowrap (dialog, prompt, width, 1, 2);

    menu_width = width - 6;
    getyx (dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - menu_width) / 2 - 1;

    /* create new window for the menu */
    menu = subwin (dialog, menu_height, menu_width, y + box_y + 1,
		   x + box_x + 1);
    keypad (menu, TRUE);

    /* draw a box around the menu items */
    draw_box (dialog, box_y, box_x, menu_height + 2, menu_width + 2,
	      menubox_border_attr, menubox_attr);

    tag_x = 0;
    item_x = 0;
    /* Find length of longest item in order to center menu */
    for (i = 0; i < item_no; i++) {
	tag_x = MAX (tag_x, strlen (items[i * 2 + 1]));
	item_x = MAX (item_x, strlen (items[i * 2]));
    }
    tag_x = (menu_width - tag_x - item_x - 2) / 2;
    item_x = tag_x + item_x + 2;

    /* Print the menu */
    for (i = 0; i < max_choice; i++)
	print_item (menu, items[i * 2], items[i * 2 + 1],
		    i, i == choice);
    wnoutrefresh (menu);

    /* register the new window, along with its borders */
    mouse_mkbigregion (box_y, box_x, menu_height + 2, menu_width + 2,
		 item_no, item_x /* the threshold */ , 1 /* dirty mode */ );

    if (menu_height < item_no) {
	wattrset (dialog, darrow_attr);
	wmove (dialog, box_y + menu_height + 1, box_x + tag_x + 1);
	waddch (dialog, ACS_DARROW);
	wmove (dialog, box_y + menu_height + 1, box_x + tag_x + 2);
	waddstr (dialog, "(+)");
    }
    x = width / 2 - 11;
    y = height - 2;
    print_button (dialog, "Cancel", y, x + 14, FALSE);
    print_button (dialog, "  OK  ", y, x, TRUE);
    wrefresh_lock (dialog);
    wtimeout(dialog, WTIMEOUT_VAL);

    while (key != ESC) {
	key = mouse_wgetch (dialog);
	/* Check if key pressed matches first character of any
	   item tag in menu */
	for (i = 0; i < max_choice; i++)
	    if (toupper (key) == toupper (items[(scroll + i) * 2][0]))
		break;

	if (i < max_choice ||
	    (key >= '1' && key <= MIN ('9', '0' + max_choice)) ||
	    key == KEY_UP || key == KEY_DOWN || key == '-' ||
	    key == '+' || (key >= M_EVENT && key - M_EVENT < ' ')) {
	    if (key >= '1' && key <= MIN ('9', '0' + max_choice))
		i = key - '1';
	    else if (key >= M_EVENT)
		i = key - M_EVENT;
	    else if (key == KEY_UP || key == '-') {
		if (!choice) {
		    if (scroll) {

			/* Scroll menu down */
			getyx (dialog, cur_y, cur_x);
			if (menu_height > 1) {
			    /* De-highlight current first item */
			    print_item (menu, items[scroll * 2],
					items[scroll * 2 + 1], 0, FALSE);
			    scrollok (menu, TRUE);
			    wscrl (menu, -1);
			    scrollok (menu, FALSE);
			}
			scroll--;
			print_item (menu, items[scroll * 2],
				    items[scroll * 2 + 1], 0, TRUE);
			wnoutrefresh (menu);

			/* print the up/down arrows */
			wmove (dialog, box_y, box_x + tag_x + 1);
			wattrset (dialog, scroll ? uarrow_attr : menubox_attr);
			waddch (dialog, scroll ? ACS_UARROW : ACS_HLINE);
			wmove (dialog, box_y, box_x + tag_x + 2);
			waddch (dialog, scroll ? '(' : ACS_HLINE);
			wmove (dialog, box_y, box_x + tag_x + 3);
			waddch (dialog, scroll ? '-' : ACS_HLINE);
			wmove (dialog, box_y, box_x + tag_x + 4);
			waddch (dialog, scroll ? ')' : ACS_HLINE);
			wattrset (dialog, darrow_attr);
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 1);
			waddch (dialog, ACS_DARROW);
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 2);
			waddstr (dialog, "(+)");
			wmove (dialog, cur_y, cur_x);
			wrefresh_lock (dialog);
		    }
		    continue;	/* wait for another key press */
		} else
		    i = choice - 1;
	    } else if (key == KEY_DOWN || key == '+')
		if (choice == max_choice - 1) {
		    if (scroll + choice < item_no - 1) {
			/* Scroll menu up */
			getyx (dialog, cur_y, cur_x);
			if (menu_height > 1) {
			    /* De-highlight current last item */
			    print_item (menu, items[(scroll + max_choice - 1)
				       * 2], items[(scroll + max_choice - 1)
					   * 2 + 1], max_choice - 1, FALSE);
			    scrollok (menu, TRUE);
			    scroll (menu);
			    scrollok (menu, FALSE);
			}
			scroll++;
			print_item (menu, items[(scroll + max_choice - 1) * 2],
				    items[(scroll + max_choice - 1) * 2 + 1],
				    max_choice - 1, TRUE);
			wnoutrefresh (menu);

			/* print the up/down arrows */
			wattrset (dialog, uarrow_attr);
			wmove (dialog, box_y, box_x + tag_x + 1);
			waddch (dialog, ACS_UARROW);
			wmove (dialog, box_y, box_x + tag_x + 2);
			waddstr (dialog, "(-)");
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 1);
			wattrset (dialog, scroll + choice < item_no - 1 ?
				  darrow_attr : menubox_border_attr);
			waddch (dialog, scroll + choice < item_no - 1 ?
				ACS_DARROW : ACS_HLINE);
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 2);
			waddch (dialog, scroll + choice < item_no - 1 ?
				'(' : ACS_HLINE);
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 3);
			waddch (dialog, scroll + choice < item_no - 1 ?
				'+' : ACS_HLINE);
			wmove (dialog, box_y + menu_height + 1,
			       box_x + tag_x + 4);
			waddch (dialog, scroll + choice < item_no - 1 ?
				')' : ACS_HLINE);
			wmove (dialog, cur_y, cur_x);
			wrefresh_lock (dialog);
		    }
		    continue;	/* wait for another key press */
		} else
		    i = choice + 1;

	    if (i != choice) {
		/* De-highlight current item */
		getyx (dialog, cur_y, cur_x);	/* Save cursor position */
		print_item (menu, items[(scroll + choice) * 2],
			    items[(scroll + choice) * 2 + 1], choice, FALSE);

		/* Highlight new item */
		choice = i;
		print_item (menu, items[(scroll + choice) * 2],
			    items[(scroll + choice) * 2 + 1], choice, TRUE);
		wnoutrefresh (menu);
		wmove (dialog, cur_y, cur_x);
		wrefresh_lock (dialog);
	    }
	    continue;		/* wait for another key press */
	}
	switch (key) {
	case 'O':
	case 'o':
	case M_EVENT + 'O':
	    delwin (dialog);
	    return scroll + choice;
	case 'C':
	case 'c':
	case M_EVENT + 'C':
	    delwin (dialog);
	    return -2;
	case M_EVENT + 'o':	/* mouse enter... */
	case M_EVENT + 'c':	/* use the code for toggling */
	    button = (key == M_EVENT + 'o');
	case ' ':
	case TAB:
	case KEY_LEFT:
	case KEY_RIGHT:
	    if (!button) {
		button = 1;	/* Indicates "Cancel" button is selected */
		print_button (dialog, "  OK  ", y, x, FALSE);
		print_button (dialog, "Cancel", y, x + 14, TRUE);
	    } else {
		button = 0;	/* Indicates "OK" button is selected */
		print_button (dialog, "Cancel", y, x + 14, FALSE);
		print_button (dialog, "  OK  ", y, x, TRUE);
	    }
	    wrefresh_lock (dialog);
	    break;
	case '\n':
	    delwin (dialog);
	    return (button? -2 : (scroll + choice));
	case ESC:
	    break;
	}
    }

    delwin (dialog);
    return -1;			/* ESC pressed */
}
