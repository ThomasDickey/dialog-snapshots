/*
 *  $Id: guage.c,v 1.26 2004/12/20 23:42:40 tom Exp $
 *
 *  guage.c -- implements the gauge dialog
 *
 *  AUTHOR: Marc Ewing, Red Hat Software
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

#define MY_LEN (MAX_LEN)/2

#define MIN_HIGH (4)
#define MIN_WIDE (10 + 2 * (2 + MARGIN))

#define isMarker(buf) !strncmp(buf, "XXX", 3)

static char *
read_data(char *buffer, FILE *fp)
{
    char *result;
    if (feof(fp)) {
	result = 0;
    } else if ((result = fgets(buffer, MY_LEN, fp)) != 0) {
	dlg_trim_string(result);
    }
    return result;
}

static int
decode_percent(char *buffer)
{
    char *tmp = 0;
    long value = strtol(buffer, &tmp, 10);

    if (tmp != 0 && (*tmp == 0 || isspace(UCH(*tmp))) && value >= 0) {
	return TRUE;
    }
    return FALSE;
}

/*
 * Display a gauge, or progress meter.  Starts at percent% and reads stdin.  If
 * stdin is not XXX, then it is interpreted as a percentage, and the display is
 * updated accordingly.  Otherwise the next line is the percentage, and
 * subsequent lines up to another XXX are used for the new prompt.  Note that
 * the size of the window never changes, so the prompt can not get any larger
 * than the height and width specified.
 */
int
dialog_gauge(const char *title,
	     const char *prompt,
	     int height,
	     int width,
	     int percent)
{
    int i, x, y;
    char buf[MY_LEN];
    char prompt_buf[MY_LEN];
    WINDOW *dialog;

    dlg_auto_size(title, prompt, &height, &width, MIN_HIGH, MIN_WIDE);
    dlg_print_size(height, width);
    dlg_ctl_size(height, width);

    /* center dialog box on screen */
    x = dlg_box_x_ordinate(width);
    y = dlg_box_y_ordinate(height);

    dialog = dlg_new_window(height, width, y, x);

    curs_set(0);
    do {
	(void) werase(dialog);
	dlg_draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

	dlg_draw_title(dialog, title);

	wattrset(dialog, dialog_attr);
	dlg_print_autowrap(dialog, prompt, height, width - (2 * MARGIN));

	dlg_draw_box(dialog,
		     height - 4, 2 + MARGIN,
		     2 + MARGIN, width - 2 * (2 + MARGIN),
		     dialog_attr,
		     border_attr);

	/*
	 * Clear the area for the progress bar by filling it with spaces
	 * in the title-attribute, and write the percentage with that
	 * attribute.
	 */
	(void) wmove(dialog, height - 3, 4);
	wattrset(dialog, title_attr);

	for (i = 0; i < (width - 2 * (3 + MARGIN)); i++)
	    (void) waddch(dialog, ' ');

	(void) wmove(dialog, height - 3, (width / 2) - 2);
	(void) wprintw(dialog, "%3d%%", percent);

	/*
	 * Now draw a bar in reverse, relative to the background.
	 * The window attribute was useful for painting the background,
	 * but requires some tweaks to reverse it.
	 */
	x = (percent * (width - 2 * (3 + MARGIN))) / 100;
	if ((title_attr & A_REVERSE) != 0) {
	    wattroff(dialog, A_REVERSE);
	} else {
	    wattrset(dialog, A_REVERSE);
	}
	(void) wmove(dialog, height - 3, 4);
	for (i = 0; i < x; i++) {
	    chtype ch = winch(dialog);
	    if (title_attr & A_REVERSE) {
		ch &= ~A_REVERSE;
	    }
	    (void) waddch(dialog, ch);
	}

	(void) wrefresh(dialog);

	if (read_data(buf, dialog_state.pipe_input) == 0)
	    break;
	if (isMarker(buf)) {
	    /*
	     * Historically, next line should be percentage, but one of the
	     * worse-written clones of 'dialog' assumes the number is missing.
	     * (Gresham's Law applied to software).
	     */
	    if (read_data(buf, dialog_state.pipe_input) == 0)
		break;
	    prompt_buf[0] = '\0';
	    if (decode_percent(buf))
		percent = atoi(buf);
	    else
		strcpy(prompt_buf, buf);

	    /* Rest is message text */
	    while (read_data(buf, dialog_state.pipe_input) != 0
		   && !isMarker(buf)) {
		if (strlen(prompt_buf) + strlen(buf) < sizeof(prompt_buf) - 1) {
		    strcat(prompt_buf, buf);
		}
	    }
	    prompt = prompt_buf;
	} else if (decode_percent(buf)) {
	    percent = atoi(buf);
	}
    } while (1);

    curs_set(1);
    dlg_del_window(dialog);
    return (DLG_EXIT_OK);
}
