/*
 *  $Id: guage.c,v 1.17 2001/04/28 23:09:27 tom Exp $
 *
 *  guage.c -- implements the gauge dialog
 *
 *  AUTHOR: Marc Ewing, Red Hat Software
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

#define isMarker(buf) !strncmp(buf, "XXX", 3)

static char *
read_data(char *buffer, FILE * fp)
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

    if (tmp != 0 && (*tmp == 0 || isspace(*tmp)) && value >= 0) {
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
dialog_gauge(const char *title, const char *prompt, int height,
	     int width, int percent)
{
    int i, x, y;
    char buf[MY_LEN];
    char prompt_buf[MY_LEN];
    WINDOW *dialog;

    /* center dialog box on screen */
    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    dialog = new_window(height, width, y, x);

    do {
	(void) werase(dialog);
	draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

	draw_title(dialog, title);

	wattrset(dialog, dialog_attr);
	print_autowrap(dialog, prompt, height, width - 2);

	draw_box(dialog, height - 4, 3, 3, width - 6, dialog_attr,
		 border_attr);

	(void) wmove(dialog, height - 3, 4);
	wattrset(dialog, title_attr);

	for (i = 0; i < (width - 8); i++)
	    (void) waddch(dialog, ' ');

	wattrset(dialog, title_attr);
	(void) wmove(dialog, height - 3, (width / 2) - 2);
	(void) wprintw(dialog, "%3d%%", percent);

	x = (percent * (width - 8)) / 100;
	wattrset(dialog, A_REVERSE);
	(void) wmove(dialog, height - 3, 4);
	for (i = 0; i < x; i++)
	    (void) waddch(dialog, winch(dialog));

	(void) wrefresh(dialog);

	if (read_data(buf, pipe_fp) == 0)
	    break;
	if (isMarker(buf)) {
	    /*
	     * Historically, next line should be percentage, but one of the
	     * worse-written clones of 'dialog' assumes the number is missing.
	     * (Gresham's Law applied to software).
	     */
	    if (read_data(buf, pipe_fp) == 0)
		break;
	    prompt_buf[0] = '\0';
	    if (decode_percent(buf))
		percent = atoi(buf);
	    else
		strcpy(prompt_buf, buf);

	    /* Rest is message text */
	    while (read_data(buf, pipe_fp) != 0
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

    (void) delwin(dialog);
    return (0);
}
