/*
 *  util.c
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

#ifdef HAVE_NCURSES
/* use colors by default? */
bool use_colors = USE_COLORS;
/* shadow dialog boxes by default?
   Note that 'use_shadow' implies 'use_colors' */
bool use_shadow = USE_SHADOW;
#endif

const char *dialog_result;

/* 
 * Attribute values, default is for mono display
 */
chtype attributes[] =
{
    A_NORMAL,			/* screen_attr */
    A_NORMAL,			/* shadow_attr */
    A_REVERSE,			/* dialog_attr */
    A_REVERSE,			/* title_attr */
    A_REVERSE,			/* border_attr */
    A_BOLD,			/* button_active_attr */
    A_DIM,			/* button_inactive_attr */
    A_UNDERLINE,		/* button_key_active_attr */
    A_UNDERLINE,		/* button_key_inactive_attr */
    A_NORMAL,			/* button_label_active_attr */
    A_NORMAL,			/* button_label_inactive_attr */
    A_REVERSE,			/* inputbox_attr */
    A_REVERSE,			/* inputbox_border_attr */
    A_REVERSE,			/* searchbox_attr */
    A_REVERSE,			/* searchbox_title_attr */
    A_REVERSE,			/* searchbox_border_attr */
    A_REVERSE,			/* position_indicator_attr */
    A_REVERSE,			/* menubox_attr */
    A_REVERSE,			/* menubox_border_attr */
    A_REVERSE,			/* item_attr */
    A_NORMAL,			/* item_selected_attr */
    A_REVERSE,			/* tag_attr */
    A_REVERSE,			/* tag_selected_attr */
    A_NORMAL,			/* tag_key_attr */
    A_BOLD,			/* tag_key_selected_attr */
    A_REVERSE,			/* check_attr */
    A_REVERSE,			/* check_selected_attr */
    A_REVERSE,			/* uarrow_attr */
    A_REVERSE			/* darrow_attr */
};

#ifdef HAVE_NCURSES
#include "colors.h"

/*
 * Table of color values
 */
int color_table[][3] =
{
    {SCREEN_FG, SCREEN_BG, SCREEN_HL},
    {SHADOW_FG, SHADOW_BG, SHADOW_HL},
    {DIALOG_FG, DIALOG_BG, DIALOG_HL},
    {TITLE_FG, TITLE_BG, TITLE_HL},
    {BORDER_FG, BORDER_BG, BORDER_HL},
    {BUTTON_ACTIVE_FG, BUTTON_ACTIVE_BG, BUTTON_ACTIVE_HL},
    {BUTTON_INACTIVE_FG, BUTTON_INACTIVE_BG, BUTTON_INACTIVE_HL},
    {BUTTON_KEY_ACTIVE_FG, BUTTON_KEY_ACTIVE_BG, BUTTON_KEY_ACTIVE_HL},
    {BUTTON_KEY_INACTIVE_FG, BUTTON_KEY_INACTIVE_BG, BUTTON_KEY_INACTIVE_HL},
    {BUTTON_LABEL_ACTIVE_FG, BUTTON_LABEL_ACTIVE_BG, BUTTON_LABEL_ACTIVE_HL},
    {BUTTON_LABEL_INACTIVE_FG, BUTTON_LABEL_INACTIVE_BG,
     BUTTON_LABEL_INACTIVE_HL},
    {INPUTBOX_FG, INPUTBOX_BG, INPUTBOX_HL},
    {INPUTBOX_BORDER_FG, INPUTBOX_BORDER_BG, INPUTBOX_BORDER_HL},
    {SEARCHBOX_FG, SEARCHBOX_BG, SEARCHBOX_HL},
    {SEARCHBOX_TITLE_FG, SEARCHBOX_TITLE_BG, SEARCHBOX_TITLE_HL},
    {SEARCHBOX_BORDER_FG, SEARCHBOX_BORDER_BG, SEARCHBOX_BORDER_HL},
    {POSITION_INDICATOR_FG, POSITION_INDICATOR_BG, POSITION_INDICATOR_HL},
    {MENUBOX_FG, MENUBOX_BG, MENUBOX_HL},
    {MENUBOX_BORDER_FG, MENUBOX_BORDER_BG, MENUBOX_BORDER_HL},
    {ITEM_FG, ITEM_BG, ITEM_HL},
    {ITEM_SELECTED_FG, ITEM_SELECTED_BG, ITEM_SELECTED_HL},
    {TAG_FG, TAG_BG, TAG_HL},
    {TAG_SELECTED_FG, TAG_SELECTED_BG, TAG_SELECTED_HL},
    {TAG_KEY_FG, TAG_KEY_BG, TAG_KEY_HL},
    {TAG_KEY_SELECTED_FG, TAG_KEY_SELECTED_BG, TAG_KEY_SELECTED_HL},
    {CHECK_FG, CHECK_BG, CHECK_HL},
    {CHECK_SELECTED_FG, CHECK_SELECTED_BG, CHECK_SELECTED_HL},
    {UARROW_FG, UARROW_BG, UARROW_HL},
    {DARROW_FG, DARROW_BG, DARROW_HL},
};				/* color_table */
#endif

/*
 * Display background title if it exists ...
 */
void put_backtitle(void)
{

  int i;

  if (backtitle != NULL)
  {
    wattrset(stdscr, screen_attr);
    wmove(stdscr, 0, 1);
    waddstr(stdscr,backtitle);
    for(i=0; i<COLS-strlen(backtitle); i++)
      waddch(stdscr, ' ');
    wmove(stdscr, 1, 1);
    for(i=0; i<COLS-2; i++)
      waddch(stdscr, ACS_HLINE);
  }

  wnoutrefresh(stdscr);
}

/*
 * Set window to attribute 'attr'
 */
void
attr_clear (WINDOW * win, int height, int width, chtype attr)
{
    int i, j;

    wattrset (win, attr);
    for (i = 0; i < height; i++) {
	wmove (win, i, 0);
	for (j = 0; j < width; j++)
	    waddch (win, ' ');
    }
    touchwin (win);
}

/*
 * Do some initialization for dialog
 */
void
init_dialog (void)
{
    mouse_open ();
#ifdef HAVE_RC_FILE
#ifdef HAVE_NCURSES
    if (parse_rc () == -1)	/* Read the configuration file */
	exiterr("");
#endif
#endif

    initscr ();			/* Init curses */
    keypad (stdscr, TRUE);
    cbreak ();
    noecho ();
    screen_initialized = 1;

#ifdef HAVE_NCURSES
    if (use_colors || use_shadow)	/* Set up colors */
	color_setup ();
#endif

    /* Set screen to screen attribute */
    attr_clear (stdscr, LINES, COLS, screen_attr);
}

#ifdef HAVE_NCURSES
/*
 * Setup for color display
 */
void
color_setup (void)
{
    int i;

    if (has_colors ()) {	/* Terminal supports color? */
	start_color ();

	/* Initialize color pairs */
	for (i = 0; i < ATTRIBUTE_COUNT; i++)
	    init_pair (i + 1, color_table[i][0], color_table[i][1]);

	/* Setup color attributes */
	for (i = 0; i < ATTRIBUTE_COUNT; i++)
	    attributes[i] = C_ATTR (color_table[i][2], i + 1);
    }
}
#endif

/*
 * End using dialog functions.
 */
void
end_dialog (void)
{
    mouse_close ();
    endwin ();
}

/*
 * Print a string of text in a window, automatically wrap around to the
 * next line if the string is too long to fit on one line. Note that the
 * string may contain "\n" to represent a newline character or the real
 * newline '\n', but in that case, auto wrap around will be disabled.
 */
void print_autowrap(WINDOW *win, const char *prompt, int width, int y, int x)
{
  int first = 1, cur_x, cur_y;
  char tempstr[MAX_LEN+1], *word, *tempptr, *tempptr1;

  strcpy(tempstr, prompt);
  if ((strstr(tempstr, "\\n") != NULL) ||
      (strchr(tempstr, '\n') != NULL)) {    /* Prompt contains "\n" or '\n' */
    word = tempstr;
    cur_y = y;
    if (cr_wrap) cur_y++;
    wmove(win, cur_y, x);
    while (1) {
      tempptr = strstr(word, "\\n");
      tempptr1 = strchr(word, '\n');
      if (tempptr == NULL && tempptr1 == NULL)
        break;
      else if (tempptr == NULL) {    /* No more "\n" */
        tempptr = tempptr1;
        tempptr[0] = '\0';
      }
      else if (tempptr1 == NULL) {    /* No more '\n' */
        tempptr[0] = '\0';
        tempptr++;
      }
      else {    /* Prompt contains both "\n" and '\n' */
        if (strlen(tempptr) <= strlen(tempptr1)) {
          tempptr = tempptr1;
          tempptr[0] = '\0';
        }
        else {
          tempptr[0] = '\0';
          tempptr++;
        }
      }

      waddstr(win, word);
      word = tempptr + 1;
      wmove(win, ++cur_y, x);
    }
    waddstr(win, word);
  }
  else if (strlen(tempstr) <= width-x*2) {     /* If prompt is short */
    cur_y=y;
    if (cr_wrap) cur_y++;
    wmove(win, cur_y, (width - strlen(tempstr)) / 2);
    waddstr(win, tempstr);
  }
  else {
    cur_x = x;
    cur_y = y;
    if (cr_wrap) cur_y++;
    /* Print prompt word by word, wrap around if necessary */
    while ((word = strtok(first ? tempstr : NULL, " ")) != NULL) {
      if (first)    /* First iteration */
        first = 0;
      if (cur_x+strlen(word) > width-x) {    /* wrap around to next line */
        cur_y++;
        cur_x = x;
      }
      wmove(win, cur_y, cur_x);
      waddstr(win, word);
      getyx(win, cur_y, cur_x);
      cur_x++;
    }
  }
  if (cr_wrap) wmove(win, cur_y+1, x);
}

/*
 * if (height or width == -1) Maximize()
 * if (height or width == 0)
 *    if (there are \n in prompt)
 *       width=MAX(longer line+n, mincols)
 *       height=num.lines
 *    else
 *       calculate with aspect_ratio
 */
static char *
real_auto_size(const char * title, char *prompt, int *height, int *width, int boxlines, int mincols) {
  int count = 0, len = title ? strlen(title) : 0, street, i, first, nc = 4, nl = 3,
      cur_x, text_width;
  char *cr1, *cr2, *ptr = prompt, *str, *word;

  if ((*height == -1) || (*width == -1)) {
    *height=SLINES-(begin_set ? begin_y : 0);
    *width=SCOLS-(begin_set ? begin_x : 0);
  }
  if ((*height != 0) && (*width != 0))
    return prompt;

  cr1=strstr(ptr, "\\n");
  cr2=strchr(ptr, '\n');

  while ((cr1 != NULL) || (cr2 != NULL)) {  /* how many ("\n" or '\n') prompt contains? */

    if (cr1 == NULL)
      street=2;
    else if (cr2 == NULL)
      street=1;
    else if (cr1 < cr2)
      street=1;
    else
      street=2;

    if (street == 1) {
      if ((cr1-ptr) > len)
        len=cr1-ptr;
      ptr=cr1+2;
    } else {
      if ((cr2-ptr) > len)
        len=cr2-ptr;
      ptr=cr2+1;
    }

    cr1=strstr(ptr, "\\n");
    cr2=strchr(ptr, '\n');

    count++;
  }

  if (strlen(ptr) > len)
    len=strlen(ptr);

  /* now 'count' has the number of lines of text and 'len' the max lenght */

  if (count > 0) {  /* there are any '\n' or "\\n" */
    *height=count+nl+boxlines+(cr_wrap ? 2 : 0);
    *width=MAX((len+nc), mincols);
    return prompt;
  } else {          /* all chars in only a line */
    str = (char *)malloc(strlen(prompt)*2);
    text_width = MAX(((int) sqrt((double)(aspect_ratio*strlen(prompt)))), (mincols-nc));
	text_width = MAX(text_width, len);
    ptr = (char *)malloc(strlen(prompt)+1);

    while (1) {
      i = 0;

      first = 1;
      cur_x = 0;

      *width=text_width+nc;

      count=0;
      strcpy(ptr, prompt);
      while (((word=strtok(first ? ptr : NULL, " ")) != NULL) && (strlen(word) <= text_width)) {
        if (first)
          first = 0;

        if ((cur_x+strlen(word)) > text_width) {
          count++;
          cur_x=0;
          str[i++]='\n';
        } else if (cur_x > 0)
          str[i++]=' ';

        strcpy(str+i, word);
        i+=strlen(word);

        cur_x+=strlen(word)+1;
      }

      if (word == NULL) break;
      if (strlen(word) > text_width)
        text_width = strlen(word); /* strlen(word) is surely > (mincols-nc) */
      else
        break;
    }
    str[i]=0;

    *height=count+nl+boxlines+(cr_wrap ? 2 : 0);
 /* *height=(strlen(prompt)/text_width)+nl+boxlines */

    return str;
  }
}
/* End of auto_size() */

char *
auto_size(const char * title, char *prompt, int *height, int *width, int boxlines, int mincols) {
	char * s = real_auto_size(title, prompt, height, width, boxlines, mincols);

	if ( *width > SCOLS ) {
		(*height)++;
		*width = SCOLS;
	}

	if ( *height > SLINES )
		*height = SLINES;

	return s;
}

/*
 * if (height or width == -1) Maximize()
 * if (height or width == 0)
 *    height=MIN(SLINES, num.lines in fd+n);
 *    width=MIN(SCOLS, MAX(longer line+n, mincols));
 */
void auto_sizefile(const char * title, const char *file, int *height, int *width, int boxlines, int mincols) {
  int count = 0, len = title ? strlen(title) : 0, nc = 4, nl = 2;
  long offset;
  char ch;
  FILE *fd;

  /* Open input file for reading */
  if ((fd = fopen(file, "rb")) == NULL)
    exiterr("\nCan't open input file in auto_sizefile().\n");

  if ((*height == -1) || (*width == -1)) {
    *height=SLINES-(begin_set ? begin_y : 0);
    *width=SCOLS-(begin_set ? begin_x : 0);
  }
  if ((*height != 0) && (*width != 0)) {
    fclose(fd);
    return;
  }

  while (!feof(fd)) {
    offset = 0;
    while (((ch=getc(fd)) != '\n') && !feof(fd))
      if ((ch == TAB) && (tab_correct))
        offset+=tab_len-(offset%tab_len);
      else
        offset++;

    if (offset > len)
      len=offset;

    count++;
  }

  /* now 'count' has the number of lines of fd and 'len' the max lenght */

  *height=MIN(SLINES, count+nl+boxlines);
  *width=MIN(SCOLS, MAX((len+nc), mincols));
  /* here width and height can be maximized if > SCOLS|SLINES because
     textbox-like widgets don't put all <file> on the screen.
     Msgbox-like widget instead have to put all <text> correctly. */

  fclose(fd);
}
/* End of auto_sizefile() */

/*
 * Print a button
 */
void
print_button (WINDOW * win, const char *label, int y, int x, int selected)
{
    int i, temp;

    wmove (win, y, x);
    wattrset (win, selected ? button_active_attr : button_inactive_attr);
    waddstr (win, "<");
    temp = strspn (label, " ");
    mouse_mkbutton (y, x, strlen (label) + 2, tolower (label[temp]));
    label += temp;
    wattrset (win, selected ? button_label_active_attr
	      : button_label_inactive_attr);
    for (i = 0; i < temp; i++)
	waddch (win, ' ');
    wattrset (win, selected ? button_key_active_attr
	      : button_key_inactive_attr);
    waddch (win, label[0]);
    wattrset (win, selected ? button_label_active_attr
	      : button_label_inactive_attr);
    waddstr (win, label + 1);
    wattrset (win, selected ? button_active_attr : button_inactive_attr);
    waddstr (win, ">");
    wmove (win, y, x + temp + 1);
}

/*
 * Draw a rectangular box with line drawing characters
 */
void
draw_box (WINDOW * win, int y, int x, int height, int width,
	  chtype box, chtype border)
{
    int i, j;

    wattrset (win, 0);
    for (i = 0; i < height; i++) {
	wmove (win, y + i, x);
	for (j = 0; j < width; j++)
	    if (!i && !j)
		waddch (win, border | ACS_ULCORNER);
	    else if (i == height - 1 && !j)
		waddch (win, border | ACS_LLCORNER);
	    else if (!i && j == width - 1)
		waddch (win, box | ACS_URCORNER);
	    else if (i == height - 1 && j == width - 1)
		waddch (win, box | ACS_LRCORNER);
	    else if (!i)
		waddch (win, border | ACS_HLINE);
	    else if (i == height - 1)
		waddch (win, box | ACS_HLINE);
	    else if (!j)
		waddch (win, border | ACS_VLINE);
	    else if (j == width - 1)
		waddch (win, box | ACS_VLINE);
	    else
		waddch (win, box | ' ');
    }
}

#ifdef HAVE_NCURSES
/*
 * Draw shadows along the right and bottom edge to give a more 3D look
 * to the boxes
 */
void
draw_shadow (WINDOW * win, int y, int x, int height, int width)
{
    int i;

    if (has_colors ()) {	/* Whether terminal supports color? */
	wattrset (win, shadow_attr);
	wmove (win, y + height, x + 2);
	for (i = 0; i < width; i++)
	    waddch (win, winch (win) & A_CHARTEXT);
	for (i = y + 1; i < y + height + 1; i++) {
	    wmove (win, i, x + width);
	    waddch (win, winch (win) & A_CHARTEXT);
	    waddch (win, winch (win) & A_CHARTEXT);
	}
	wnoutrefresh (win);
    }
}
#endif

/*
 * make_tmpfile()
 */
char *make_lock_filename(char *filename) {
   char *file=(char *) malloc(30);
   strcpy(file, filename);
   if (mktemp(file) == NULL) {
     endwin();
     fprintf(stderr, "\nCan't make tempfile...\n");
     return NULL;
   }
   return file;
}
/* End of make_tmpfile() */

/*
 * Only a process at a time can call wrefresh()
 */
 
/* called by all normal widget */
void wrefresh_lock(WINDOW *win) {
  wrefresh_lock_sub(win);
  
  if (exist_lock(lock_tailbg_refreshed))
    remove_lock(lock_tailbg_refreshed);
  
}

/* If a tailboxbg was wrefreshed, wrefresh again the principal (normal) widget.
 *
 * The important thing here was to put cursor in the principal widget, 
 * but with touchwin(win) the principal widget is also rewritten. This 
 * is needed if a tailboxbg has overwritten this widget. 
 */
void ctl_idlemsg(WINDOW *win) {
  if (exist_lock(lock_tailbg_refreshed))
    wrefresh_lock(win);
    
  /* if a tailboxbg exited, quit */
  if (exist_lock(lock_tailbg_exit)) { 
    remove_lock(lock_tailbg_exit);
    exiterr("");
  }
}

/* call by tailboxbg, the resident widget */
void wrefresh_lock_tailbg(WINDOW *win) {
  wrefresh_lock_sub(win);
  create_lock(lock_tailbg_refreshed);
  /* it's right also if more tailboxbg refresh */
  /* with --no-kill flag the lock_tailbg_refreshed file
     will not be removed :-/ :-| */
}

/* private function, used by wrefresh_lock() and wrefresh_lock_tailbg() */
void wrefresh_lock_sub(WINDOW *win) {
  while_exist_lock(lock_refresh);
  create_lock(lock_refresh);
  wrefresh(win);
  beeping();
  wrefresh(win);
  remove_lock(lock_refresh);
}
/* End of wrefresh functions */

/*
 * To work with lock files, here are some functions
 */
int exist_lock(char *filename) {
  FILE *fil;
  if ((fil=fopen(filename, "r")) != NULL) {
    fclose(fil);
    return 1;
  }
  return 0;
}

void while_exist_lock(char *filename) {
  while (exist_lock(filename)) ;
}

void create_lock(char *filename) {
  FILE *fil=fopen(filename, "w");
  fclose(fil);
}

void remove_lock(char *filename) {
  remove(filename);
}
/* End of functions to work with lock files */

/* killall_bg: kill all tailboxbg, if any */
void killall_bg(void) {
  int i;
  for (i = 0; i < tailbg_lastpid; i++)
    kill(tailbg_pids[i], 15);
}

int is_nokill(int pid) { /* private func for quitall_bg() */
  int i;
  for (i = 0; i < tailbg_nokill_lastpid; i++)
    if (tailbg_nokill_pids[i] == pid) return 1;
  return 0;
}
/* quitall_bg: kill all tailboxbg without --no-kill flag */
void quitall_bg(void) {
  int i;
  for (i = 0; i < tailbg_lastpid; i++)
    if (!is_nokill(tailbg_pids[i]))
      kill(tailbg_pids[i], 15);
}

/* exiterr quit program killing all tailbg */
void exiterr(char *str) {
  if (screen_initialized)
    endwin();

  fprintf(stderr, str);

  if (is_tailbg)	/* this is a child process */
    create_lock(lock_tailbg_exit);
  else 
    killall_bg();
    
  exit(-1);
}

void beeping() {
  if (beep_signal) {
    beep();
    beep_signal = 0;
  }
}

void print_size(int height, int width) {
  if (print_siz)
    fprintf(stderr, "Size: %d, %d\n", height, width);
}

void ctl_size(int height, int width) {
  char *tempstr=(char *) malloc((size_t) MAX_LEN+1);
  if (size_err)
    if ((width > COLS) || (height > LINES)) {
      sprintf(tempstr,"\nWindow too big. (Height, width) = (%d, %d). Max allowed (%d, %d).\n", height, width, LINES, COLS);
      exiterr(tempstr);
    } else if ((use_shadow) && ((width > SCOLS || height > SLINES))) {
      sprintf(tempstr,"\nWindow+Shadow too big. (Height, width) = (%d, %d). Max allowed (%d, %d).\n", height, width, SLINES, SCOLS);
      exiterr(tempstr);
    }
}

void tab_correct_str(char *prompt) {
  char *ptr;
  
  if (!tab_correct) return;
  
  while ((ptr=strchr(prompt, TAB)) != NULL) {
    *ptr=' ';
    prompt=ptr;
  }
}

void calc_listh(int *height, int *list_height, int item_no) {
    /* calculate new height and list_height */
    int lines=SLINES-(begin_set ? begin_y : 0);
    if (lines-(*height) > 0)
      if (lines-(*height) > item_no)
        *list_height=item_no;
      else
        *list_height=lines-(*height);
    (*height)+=(*list_height);
}

int calc_listw(int item_no, const char * const * items, int group) {
    int n, i, len1=0, len2=0;
    for (i=0; i<(item_no*group); i+=group) {
      if ((n=strlen(items[i])) > len1) len1=n;
      if ((n=strlen(items[i+1])) > len2) len2=n;
    }
    return len1+len2; 
}

char *strclone(const char *cprompt) {
    char *prompt=(char *) malloc(strlen(cprompt) + 1);
    strcpy(prompt, cprompt);
    return prompt;
}
