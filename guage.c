/*
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

/*
 * Display a gauge, or progress meter.  Starts at percent% and
 * reads stdin.  If stdin is not XXX, then it is interpreted as
 * a percentage, and the display is updated accordingly.  Otherwise
 * the next line is the percentage, and subsequent lines up to
 * another XXX are used for the new prompt.  Note that the size
 * of the window never changes, so the prompt can not get any
 * larger than the height and width specified.
 */
int
dialog_gauge(const char *title, const char *prompt, int height,
    int width, int percent)
{
    int i, x, y;
    char buf[1024];
    char prompt_buf[1024];
    WINDOW *dialog;

    /* center dialog box on screen */
    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    dialog = new_window(height, width, y, x);

    do {
	werase(dialog);
	draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);

	draw_title(dialog, title);

	wattrset(dialog, dialog_attr);
	print_autowrap(dialog, prompt, width - 2, 1, 2);

	draw_box(dialog, height - 4, 3, 3, width - 6, dialog_attr,
	    border_attr);

	wmove(dialog, height - 3, 4);
	wattrset(dialog, title_attr);
	for (i = 0; i < (width - 8); i++)
	    waddch(dialog, ' ');

	wattrset(dialog, title_attr);
	wmove(dialog, height - 3, (width / 2) - 2);
	wprintw(dialog, "%3d%%", percent);

	x = (percent * (width - 8)) / 100;
	wattrset(dialog, A_REVERSE);
	wmove(dialog, height - 3, 4);
	for (i = 0; i < x; i++)
	    waddch(dialog, winch(dialog));

	wrefresh(dialog);

	if (feof(pipe_fp)
	    || fgets(buf, sizeof(buf), pipe_fp) == 0)
	    break;
	if (buf[0] == 'X') {
	    /* Next line is percentage */
	    if (feof(pipe_fp)
		|| fgets(buf, sizeof(buf), pipe_fp) == 0)
		break;
	    percent = atoi(buf);

	    /* Rest is message text */
	    prompt_buf[0] = '\0';
	    while (fgets(buf, sizeof(buf), pipe_fp) != 0
		&& strncmp(buf, "XXX", 3)
		&& strlen(prompt_buf) + strlen(buf) < sizeof(prompt_buf) - 1)
		strcat(prompt_buf, buf);
	    prompt = prompt_buf;
	} else
	    percent = atoi(buf);
    } while (1);

    delwin(dialog);
    return (0);
}
