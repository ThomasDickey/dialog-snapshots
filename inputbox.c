/*
 *  inputbox.c -- implements the input box
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

unsigned char dialog_input_result[MAX_LEN + 1];

/*
 * Display a dialog box for inputing a string
 */
int
dialog_inputbox (const char *title, const char *cprompt, int height, int width,
		 const char *init, const int password)
{
    int i, x, y, box_y, box_x, box_width;
    int input_x = 0, scrollamt = 0, key = 0, button = -1;
    unsigned char *input = dialog_input_result;
    WINDOW *dialog;
    char *prompt=strclone(cprompt);

    tab_correct_str(prompt);
    if (init != NULL) {
      prompt=auto_size(title, prompt, &height, &width, 5, MIN(MAX((int)strlen(init)+7,26),SCOLS-(begin_set ? begin_x : 0)));
    } else
      prompt=auto_size(title, prompt, &height, &width, 5, 26);
    print_size(height, width);
    ctl_size(height, width);
  
    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

    dialog = new_window (height, width, y, x);

    mouse_setbase (x, y);

    draw_box (dialog, 0, 0, height, width, dialog_attr, border_attr);
    draw_bottom_box (dialog);
    draw_title (dialog, title);

    wattrset (dialog, dialog_attr);
    print_autowrap (dialog, prompt, width, 1, 2);

    /* Draw the input field box */
    box_width = width - 6;
    getyx (dialog, y, x);
    box_y = y + 2;
    box_x = (width - box_width) / 2;
    mouse_mkregion (y + 1, box_x - 1, 3, box_width + 2, 'i');
    draw_box (dialog, y + 1, box_x - 1, 3, box_width + 2,
	      border_attr, dialog_attr);

    x = width / 2 - 11;
    y = height - 2;
    print_button (dialog, "Cancel", y, x + 14, FALSE);
    print_button (dialog, "  OK  ", y, x, TRUE);

    /* Set up the initial value */
    wmove (dialog, box_y, box_x);
    wattrset (dialog, inputbox_attr);
    if (!init)
	input[0] = '\0';
    else
	strcpy (input, init);
    input_x = strlen (input);
    if (input_x >= box_width) {
	scrollamt = input_x - box_width + 1;
	input_x = box_width - 1;
      	if (! password)
	  for (i = 0; i < box_width - 1; i++)
	      waddch (dialog, input[scrollamt + i]);
    } else if (! password)
	waddstr (dialog, input);
        for (i = 0; i < box_width - 1 - input_x; i++)
          waddch (dialog, ' ');
    
    wmove (dialog, box_y, box_x + (password ? 0 : input_x));
  
    wrefresh_lock(dialog);
    wtimeout(dialog, WTIMEOUT_VAL);

    while (key != ESC) {
	key = mouse_wgetch (dialog);

	if (button == -1) {	/* Input box selected */
	    switch (key) {
	    case TAB:
	    case KEY_UP:
	    case KEY_DOWN:
	    case M_EVENT + 'i':
	    case M_EVENT + 'o':
	    case M_EVENT + 'c':
		break;
	    case KEY_LEFT:
		continue;
	    case KEY_RIGHT:
		continue;
	    case KEY_BACKSPACE:
	    case KEY_DC:
	    case 127:
		if (input_x || scrollamt) {
		    wattrset (dialog, inputbox_attr);
		    if (!input_x) {
			scrollamt = scrollamt < box_width - 1 ?
			    0 : scrollamt - (box_width - 1);
		        if (! password) {
			  wmove (dialog, box_y, box_x);
			  for (i = 0; i < box_width; i++)
			      waddch (dialog, input[scrollamt + input_x + i] ?
				      input[scrollamt + input_x + i] : ' ');
			}
			input_x = strlen (input) - scrollamt;
		    } else
			input_x--;
		    input[scrollamt + input_x] = '\0';
		    if (! password) {
		      wmove (dialog, box_y, input_x + box_x);
		      waddch (dialog, ' ');
		      wmove (dialog, box_y, input_x + box_x);
		      wrefresh_lock (dialog);
		    }
		}
		continue;
            case 21:  /* ^U   support by Martin Schulze <joey@artis.uni-oldenburg.de> */
                input_x = 0;
                scrollamt = 0;
	        if (! password) {
                  wmove (dialog, box_y, box_x);
                  for (i = 0; i < box_width - 1; i++)
                    waddch (dialog, ' ');
                  input[0] = '\0';
                  wmove (dialog, box_y, input_x + box_x);
                  wrefresh_lock (dialog);
		}
                continue;
	    default:
		if (key < 0x100 && isprint (key)) {
		    if (scrollamt + input_x < MAX_LEN) {
			wattrset (dialog, inputbox_attr);
			input[scrollamt + input_x] = key;
			input[scrollamt + input_x + 1] = '\0';
			if (input_x == box_width - 1) {
			    scrollamt++;
			    if (! password) {
			      wmove (dialog, box_y, box_x);
			      for (i = 0; i < box_width - 1; i++)
				  waddch (dialog, input[scrollamt + i]);
			    }
			} else if (! password) {
			    wmove (dialog, box_y, input_x++ + box_x);
			    waddch (dialog, key);
			}
		        else
			    input_x++;
		      
		        if (! password)
			  wrefresh_lock (dialog);
		    } else
			flash ();	/* Alarm user about overflow */
		    continue;
		}
	    }
	}
	switch (key) {
	case 'O':
	case 'o':
	    delwin (dialog);
	    return 0;
	case 'C':
	case 'c':
	    delwin (dialog);
	    return 1;
	case M_EVENT + 'i':	/* mouse enter events */
	case M_EVENT + 'o':	/* use the code for 'UP' */
	case M_EVENT + 'c':
	    button = (key == M_EVENT + 'o') - (key == M_EVENT + 'c');
	case KEY_UP:
	case KEY_LEFT:
	    switch (button) {
	    case -1:
		button = 1;	/* Indicates "Cancel" button is selected */
		print_button (dialog, "  OK  ", y, x, FALSE);
		print_button (dialog, "Cancel", y, x + 14, TRUE);
		wrefresh_lock (dialog);
		break;
	    case 0:
		button = -1;	/* Indicates input box is selected */
		print_button (dialog, "Cancel", y, x + 14, FALSE);
		print_button (dialog, "  OK  ", y, x, TRUE);
		wmove (dialog, box_y, box_x + input_x);
		wrefresh_lock (dialog);
		break;
	    case 1:
		button = 0;	/* Indicates "OK" button is selected */
		print_button (dialog, "Cancel", y, x + 14, FALSE);
		print_button (dialog, "  OK  ", y, x, TRUE);
		wrefresh_lock (dialog);
		break;
	    }
	    break;
	case TAB:
	case KEY_DOWN:
	case KEY_RIGHT:
	    switch (button) {
	    case -1:
		button = 0;	/* Indicates "OK" button is selected */
		print_button (dialog, "Cancel", y, x + 14, FALSE);
		print_button (dialog, "  OK  ", y, x, TRUE);
		wrefresh_lock (dialog);
		break;
	    case 0:
		button = 1;	/* Indicates "Cancel" button is selected */
		print_button (dialog, "  OK  ", y, x, FALSE);
		print_button (dialog, "Cancel", y, x + 14, TRUE);
		wrefresh_lock (dialog);
		break;
	    case 1:
		button = -1;	/* Indicates input box is selected */
		print_button (dialog, "Cancel", y, x + 14, FALSE);
		print_button (dialog, "  OK  ", y, x, TRUE);
		wmove (dialog, box_y, box_x + input_x);
		wrefresh_lock (dialog);
		break;
	    }
	    break;
	case ' ':
	case '\n':
	    delwin (dialog);
	    return (button == -1 ? 0 : button);
	case ESC:
	    break;
	}
    }

    delwin (dialog);
    return -1;			/* ESC pressed */
}
