/*
 *  textbox.c -- implements the text box
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


static void back_lines(int n);
static void print_page(WINDOW *win, int height, int width);
static void print_line(WINDOW *win, int row, int width);
static char *get_line(void);
static int get_search_term(WINDOW *win, char *search_term, int height, int width);
static void print_position(WINDOW *win, int height, int width);
static char *read_high(int fd, char *buf, size_t size_read);
static int tabize(int val, int pos);

static int hscroll = 0, fd, file_size, bytes_read, begin_reached = 1,
           end_reached = 0, page_length, buffer_len = 0, buffer_first = 1,
           fd_bytes_read;

static char *buf;
static char *page;


/*
 * Display text from a file in a dialog box.
 */
int dialog_textbox(const char *title, const char *file, int height, int width)
{
  int i, x, y, cur_x, cur_y, fpos, key = 0, dir, temp, temp1;
#ifdef NCURSES_VERSION
  int passed_end;
#endif
  char search_term[MAX_LEN+1], *tempptr, *found;
  WINDOW *dialog, *text;

  auto_sizefile(title, file, &height, &width, 2, 12);
  print_size(height, width);
  ctl_size(height, width);

  search_term[0] = '\0';    /* no search term entered yet */

  /* Open input file for reading */
  if ((fd = open(file, O_RDONLY)) == -1)
    exiterr("\nCan't open input file in dialog_textbox().\n");

  /* Get file size. Actually, 'file_size' is the real file size - 1,
     since it's only the last byte offset from the beginning */
  if ((file_size = lseek(fd, 0, SEEK_END)) == -1)
    exiterr("\nError getting file size in dialog_textbox().\n");

  /* Restore file pointer to beginning of file after getting file size */
  if (lseek(fd, 0, SEEK_SET) == -1)
    exiterr("\nError moving file pointer in dialog_textbox().\n");

  if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
    exiterr("\nError reading file in dialog_textbox().\n");

  page = buf;    /* page is pointer to start of page to be displayed */

  x = box_x_ordinate(width);
  y = box_y_ordinate(height);

  dialog = new_window(height, width, y, x);

  mouse_setbase (x, y);

  /* Create window for text region, used for scrolling text */
  text = subwin(dialog, height-4, width-2, y+1, x+1);

  keypad (text, TRUE);

  /* register the new window, along with its borders */
  mouse_mkbigregion (0, 0, height - 2, width, 1, 0, 2 /* not normal */ );
  draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
  draw_bottom_box(dialog);
  draw_title(dialog, title);

  print_button(dialog, " EXIT ", height-2, width/2-4, TRUE);
  wnoutrefresh(dialog);
  getyx(dialog, cur_y, cur_x);    /* Save cursor position */

  /* Print first page of text */
  attr_clear(text, height-4, width-2, dialog_attr);
  print_page(text, height-4, width-2);
  print_position(dialog, height, width);
  wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
  wrefresh_lock(dialog);
  wtimeout(dialog, WTIMEOUT_VAL);

  while (key != ESC) {
    key = mouse_wgetch(dialog);
    switch (key) {
      case M_EVENT + 'E':
      case 'E':			/* Exit */
      case 'e':
      case '\n':
        delwin(dialog);
        free(buf);
        close(fd);
        return 0;
      case 'g':    /* First page */
      case KEY_HOME:
        if (!begin_reached) {
          begin_reached = 1;
          /* First page not in buffer? */
          if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
            exiterr("\nError moving file pointer in dialog_textbox().\n");

          if (fpos > fd_bytes_read) {    /* Yes, we have to read it in */
            if (lseek(fd, 0, SEEK_SET) == -1)
              exiterr("\nError moving file pointer in dialog_textbox().\n");

            if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
              exiterr("\nError reading file in dialog_textbox().\n");
          }
          page = buf;
          print_page(text, height-4, width-2);
          print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case 'G':    /* Last page */
#ifdef NCURSES_VERSION
      case KEY_END:
#endif
        end_reached = 1;
        /* Last page not in buffer? */
        if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
          exiterr("\nError moving file pointer in dialog_textbox().\n");

        if (fpos < file_size) {    /* Yes, we have to read it in */
          if (lseek(fd, -BUF_SIZE, SEEK_END) == -1)
            exiterr("\nError moving file pointer in dialog_textbox().\n");

          if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
            exiterr("\nError reading file in dialog_textbox().\n");
        }
        page = buf + bytes_read;
        back_lines(height-4);
        print_page(text, height-4, width-2);
        print_position(dialog, height, width);
        wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
        wrefresh_lock(dialog);
        break;
      case 'K':    /* Previous line */
      case 'k':
      case KEY_UP:
        if (!begin_reached) {
          back_lines(page_length+1);
#ifdef NCURSES_VERSION
          /* We don't call print_page() here but use scrolling to ensure
             faster screen update. However, 'end_reached' and 'page_length'
             should still be updated, and 'page' should point to start of
             next page. This is done by calling get_line() in the following
             'for' loop. */
          scrollok(text, TRUE);
          wscrl(text, -1);    /* Scroll text region down one line */
          scrollok(text, FALSE);
          page_length = 0;
          passed_end = 0;
          for (i = 0; i < height-4; i++) {
            if (!i) {
              print_line(text, 0, width-2);    /* print first line of page */
              wnoutrefresh(text);
            }
            else
              get_line();    /* Called to update 'end_reached' and 'page' */
            if (!passed_end)
              page_length++;
            if (end_reached && !passed_end)
              passed_end = 1;
          }
#else
          print_page(text, height-4, width-2);
#endif
          print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case 'B':    /* Previous page */
      case 'b':
      case KEY_PPAGE:
        if (!begin_reached) {
          back_lines(page_length + height-4);
          print_page(text, height-4, width-2);
          print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case 'J':    /* Next line */
      case 'j':
      case KEY_DOWN:
        if (!end_reached) {
          begin_reached = 0;
          scrollok(text, TRUE);
          scroll(text);    /* Scroll text region up one line */
          scrollok(text, FALSE);
          print_line(text, height-5, width-2);
#ifndef NCURSES_VERSION
          wmove(text, height-5, 0);
          waddch(text, ' ');
          wmove(text, height-5, width-3);
          waddch(text, ' ');
#endif
          wnoutrefresh(text);
          print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case ' ':    /* Next page */
      case KEY_NPAGE:
        if (!end_reached) {
          begin_reached = 0;
          print_page(text, height-4, width-2);
          print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case '0':    /* Beginning of line */
      case 'H':    /* Scroll left */
      case 'h':
      case KEY_LEFT:
        if (hscroll > 0) {
          if (key == '0')
            hscroll = 0;
          else
            hscroll--;
          /* Reprint current page to scroll horizontally */
          back_lines(page_length);
          print_page(text, height-4, width-2);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case 'L':    /* Scroll right */
      case 'l':
      case KEY_RIGHT:
        if (hscroll < MAX_LEN) {
          hscroll++;
          /* Reprint current page to scroll horizontally */
          back_lines(page_length);
          print_page(text, height-4, width-2);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        break;
      case '/':    /* Forward search */
      case 'n':    /* Repeat forward search */
      case '?':    /* Backward search */
      case 'N':    /* Repeat backward search */
        /* set search direction */
        dir = (key == '/' || key == 'n') ? 1 : 0;
        if (dir ? !end_reached : !begin_reached) {
          if (key == 'n' || key == 'N') {
            if (search_term[0] == '\0') {    /* No search term yet */
              fprintf(stderr, "\a");    /* beep */
              break;
            }
	  }
          else    /* Get search term from user */
            if (get_search_term(text, search_term, height-4, width-2) == -1) {
              /* ESC pressed in get_search_term(). Reprint page to clear box */
              wattrset(text, dialog_attr);
              back_lines(page_length);
              print_page(text, height-4, width-2);
              wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
              wrefresh_lock(dialog);
              break;
            }
          /* Save variables for restoring in case search term can't be found */
          tempptr = page;
          temp = begin_reached;
          temp1 = end_reached;
          if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
            exiterr("\nError moving file pointer in dialog_textbox().\n");

          fpos -= fd_bytes_read;
          /* update 'page' to point to next (previous) line before
             forward (backward) searching */
          back_lines(dir ? page_length-1 : page_length+1);
          found = NULL;
          if (dir)    /* Forward search */
            while((found = strstr(get_line(), search_term)) == NULL) {
              if (end_reached)
                break;
	    }
          else    /* Backward search */
            while((found = strstr(get_line(), search_term)) == NULL) {
              if (begin_reached)
                break;
              back_lines(2);
            }
          if (found == NULL) {    /* not found */
            fprintf(stderr, "\a");    /* beep */
            /* Restore program state to that before searching */
            if (lseek(fd, fpos, SEEK_SET) == -1)
              exiterr("\nError moving file pointer in dialog_textbox().\n");

            if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
              exiterr("\nError reading file in dialog_textbox().\n");

            page = tempptr;
            begin_reached = temp;
            end_reached = temp1;
            /* move 'page' to point to start of current page in order to
               re-print current page. Note that 'page' always points to
               start of next page, so this is necessary */
            back_lines(page_length);
          }
          else    /* Search term found */
            back_lines(1);
          /* Reprint page */
          wattrset(text, dialog_attr);
          print_page(text, height-4, width-2);
          if (found != NULL)
            print_position(dialog, height, width);
          wmove(dialog, cur_y, cur_x);    /* Restore cursor position */
          wrefresh_lock(dialog);
        }
        else    /* no need to find */
          fprintf(stderr, "\a");    /* beep */
        break;
      case ESC:
        break;
    }
  }

  delwin(dialog);
  free(buf);
  close(fd);
  return -1;    /* ESC pressed */
}
/* End of dialog_textbox() */


/*
 * Go back 'n' lines in text file. Called by dialog_textbox().
 * 'page' will be updated to point to the desired line in 'buf'.
 */
static void back_lines(int n)
{
  int i, fpos, val_to_tabize;

  begin_reached = 0;
  /* We have to distinguish between end_reached and !end_reached since at end
     of file, the line is not ended by a '\n'. The code inside 'if' basically
     does a '--page' to move one character backward so as to skip '\n' of the
     previous line */
  if (!end_reached) {
    /* Either beginning of buffer or beginning of file reached? */

    if (page == buf) {
      if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
        exiterr("\nError moving file pointer in back_lines().\n");

      if (fpos > fd_bytes_read) {    /* Not beginning of file yet */
        /* We've reached beginning of buffer, but not beginning of file yet,
           so read previous part of file into buffer. Note that we only
           move backward for BUF_SIZE/2 bytes, but not BUF_SIZE bytes to
           avoid re-reading again in print_page() later */
        /* Really possible to move backward BUF_SIZE/2 bytes? */
        if (fpos < BUF_SIZE/2 + fd_bytes_read) {
          /* No, move less then */
          if (lseek(fd, 0, SEEK_SET) == -1)
            exiterr("\nError moving file pointer in back_lines().\n");

          val_to_tabize = fpos - fd_bytes_read;
        }
        else {    /* Move backward BUF_SIZE/2 bytes */
          if (lseek(fd, -(BUF_SIZE/2 + fd_bytes_read), SEEK_CUR) == -1)
            exiterr("\nError moving file pointer in back_lines().\n");

          val_to_tabize = BUF_SIZE/2;
        }
        if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
          exiterr("\nError reading file in back_lines().\n");

        page = buf + tabize(val_to_tabize, 0);
        
      }
      else {    /* Beginning of file reached */
        begin_reached = 1;
        return;
      }
    }
    if (*(--page) != '\n')    /* '--page' here */ /* Something's wrong... */
      exiterr("\nInternal error in back_lines().\n");
  }

  /* Go back 'n' lines */
  for (i = 0; i < n; i++)
    do {
      if (page == buf) {
        if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
          exiterr("\nError moving file pointer in back_lines().\n");

        if (fpos > fd_bytes_read) {
          /* Really possible to move backward BUF_SIZE/2 bytes? */
          if (fpos < BUF_SIZE/2 + fd_bytes_read) {
            /* No, move less then */
            if (lseek(fd, 0, SEEK_SET) == -1)
              exiterr("\nError moving file pointer in back_lines().\n");

            val_to_tabize = fpos - fd_bytes_read;
          }
          else {    /* Move backward BUF_SIZE/2 bytes */
            if (lseek(fd, -(BUF_SIZE/2 + fd_bytes_read), SEEK_CUR) == -1)
              exiterr("\nError moving file pointer in back_lines().\n");

            val_to_tabize = BUF_SIZE/2;
          }
          if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
            exiterr("\nError reading file in back_lines().\n");
            
          page = buf + tabize(val_to_tabize, 0);

        }
        else {    /* Beginning of file reached */
          begin_reached = 1;
          return;
        }
      }
    } while (*(--page) != '\n');
  page++;
}
/* End of back_lines() */


/*
 * Print a new page of text. Called by dialog_textbox().
 */
static void print_page(WINDOW *win, int height, int width)
{
  int i, passed_end = 0;

  page_length = 0;
  for (i = 0; i < height; i++) {
    print_line(win, i, width);
    if (!passed_end)
      page_length++;
    if (end_reached && !passed_end)
      passed_end = 1;
  }
  wnoutrefresh(win);
}
/* End of print_page() */


/*
 * Print a new line of text. Called by dialog_textbox() and print_page().
 */
static void print_line(WINDOW *win, int row, int width)
{
  int i, y, x;
  char *line;
  line = get_line();
  line += MIN((int)strlen(line),hscroll);    /* Scroll horizontally */
  wmove(win, row, 0);    /* move cursor to correct line */
  waddch(win,' ');
#ifdef NCURSES_VERSION
  waddnstr(win, line, MIN((int)strlen(line),width-2));
#else
  line[MIN(strlen(line),width-2)] = '\0';
  waddstr(win, line);
#endif

  getyx(win, y, x);
  /* Clear 'residue' of previous line */
  for (i = 0; i < width-x; i++)
    waddch(win, ' ');
}
/* End of print_line() */


/*
 * Return current line of text. Called by dialog_textbox() and print_line().
 * 'page' should point to start of current line before calling, and will be
 * updated to point to start of next line.
 */
static char *get_line(void)
{
  int i = 0, fpos;
  static char line[MAX_LEN+1];

  end_reached = 0;
  while (*page != '\n') {
    if (*page == '\0') {    /* Either end of file or end of buffer reached */
      if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
        exiterr("\nError moving file pointer in get_line().\n");

      if (fpos < file_size) {    /* Not end of file yet */
        /* We've reached end of buffer, but not end of file yet, so read next
           part of file into buffer */
        if ((buf = read_high(fd, buf, BUF_SIZE)) == NULL)
          exiterr("\nError reading file in get_line().\n");
        page = buf;
      }
      else {
        if (!end_reached)
          end_reached = 1;
        break;
      }
    }
    else
      if (i < MAX_LEN)
        line[i++] = *(page++);
      else {
        if (i == MAX_LEN)  /* Truncate lines longer than MAX_LEN characters */
          line[i++] = '\0';
        page++;
      }
  }
  if (i <= MAX_LEN)
    line[i] = '\0';
  if (!end_reached)
    page++;    /* move pass '\n' */

  return line;
}
/* End of get_line() */


/*
 * Display a dialog box and get the search term from user
 */
static int get_search_term(WINDOW *win, char *search_term, int height, int width)
{
  int i, x, y, input_x = 0, scrollamt = 0, key = 0,
      box_height = 3, box_width = 30;

  x = (width - box_width)/2;
  y = (height - box_height)/2;
#ifdef HAVE_COLOR
  if (use_shadow)
    draw_shadow(win, y, x, box_height, box_width);
#endif
  draw_box(win, y, x, box_height, box_width, dialog_attr, searchbox_border_attr);
  wattrset(win, searchbox_title_attr);
  wmove(win, y, x+box_width/2-4);
  waddstr(win, " Search ");

  box_width -= 2;
  wmove(win, y+1, x+1);
  wrefresh_lock(win);
  search_term[0] = '\0';
  wattrset(win, searchbox_attr);
  while (key != ESC) {
    key = wgetch(win);
    switch (key) {
      case '\n':
        if (search_term[0] != '\0')
          return 0;
        break;
      case KEY_BACKSPACE:
      case KEY_DC:
      case 127:
        if (input_x || scrollamt) {
          if (!input_x) {
            scrollamt = scrollamt < box_width-1 ? 0 : scrollamt-(box_width-1);
            wmove(win, y+1, x+1);
            for (i = 0; i < box_width; i++)
              waddch(win, search_term[scrollamt+input_x+i] ?
                            search_term[scrollamt+input_x+i] : ' ');
            input_x = strlen(search_term) - scrollamt;
          }
          else
            input_x--;
          search_term[scrollamt+input_x] = '\0';
          wmove(win, y+1, input_x + x+1);
          waddch(win, ' ');
          wmove(win, y+1, input_x + x+1);
          wrefresh_lock(win);
        }
        break;
      case ESC:
        break;
      default:
        if (isprint(key))
          if (scrollamt+input_x < MAX_LEN) {
            search_term[scrollamt+input_x] = key;
            search_term[scrollamt+input_x+1] = '\0';
            if (input_x == box_width-1) {
              scrollamt++;
              wmove(win, y+1, x+1);
              for (i = 0; i < box_width-1; i++)
                waddch(win, search_term[scrollamt+i]);
            }
            else {
              wmove(win, y+1, input_x++ + x+1);
              waddch(win, key);
            }
            wrefresh_lock(win);
          }
    }
  }

  return -1;    /* ESC pressed */
}
/* End of get_search_term() */


/*
 * Print current position
 */
static void print_position(WINDOW *win, int height, int width)
{
  int fpos, percent;

  if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
    exiterr("\nError moving file pointer in print_position().\n");

  wattrset(win, position_indicator_attr);
  percent = !file_size ? 100 : ((fpos-fd_bytes_read+tabize(page-buf, 1))*100)/file_size;
  wmove(win, height-3, width-9);
  wprintw(win, "(%3d%%)", percent);
}
/* End of print_position() */

/*
 * read_high() substitutes read() for tab->spaces conversion
 *
 * buffer_len, fd_bytes_read, bytes_read are modified
 * buf is allocated
 *
 * fd_bytes_read is the effective number of bytes read from file
 * bytes_read is the length of buf, that can be different if tab_correct
 */
static char *read_high(int my_fd, char *my_buf, size_t size_read)
{
  char *buftab, ch;
  int i = 0, j, n, begin_line, tmpint, disp = 0;

  /* Allocate space for read buffer */
  if ((buftab = malloc(size_read+1)) == NULL)
    exiterr("\nCan't allocate memory in read_high().\n");

  if ((fd_bytes_read = read(my_fd, buftab, size_read)) == -1)
    exiterr("\nError reading file in read_high().\n");

  buftab[fd_bytes_read] = '\0';    /* mark end of valid data */
  
  if (tab_correct) {
  
    /* calculate bytes_read by buftab and fd_bytes_read */
    bytes_read=begin_line=0;
    for (j=0; j < fd_bytes_read; j++)
      if (buftab[j] == TAB)
        bytes_read+=tab_len-((bytes_read-begin_line)%tab_len);
      else if (buftab[j] == '\n') {
        bytes_read++;
        begin_line=bytes_read;
      } else
        bytes_read++;
  
  
    if (bytes_read > buffer_len) {
      if (buffer_first)
        buffer_first = 0; /* disp = 0 */
      else {
        disp = page-my_buf;
        free(my_buf);
      }
        
      buffer_len = bytes_read;
        
      /* Allocate space for read buffer */
      if ((my_buf = malloc(buffer_len+1)) == NULL)
        exiterr("\nCan't allocate memory in read_high().\n");
        
      page=my_buf+disp;
    }
    
  }
  else {
    if (buffer_first) {
      buffer_first = 0;
      
      /* Allocate space for read buffer */
      if ((my_buf = malloc(size_read+1)) == NULL)
        exiterr("\nCan't allocate memory in read_high().\n");
    }
    
    bytes_read = fd_bytes_read;
  }
  
  j=0; begin_line=0;
  while (j < fd_bytes_read)
    if (((ch=buftab[j++]) == TAB) && (tab_correct)) {
      tmpint=tab_len-((i-begin_line)%tab_len);
      for(n = 0; n < tmpint; n++)
        my_buf[i++]=' ';
    } else {
      if (ch == '\n')
        begin_line=i+1;
      my_buf[i++] = ch;
    }
    
  my_buf[i] = '\0';	/* mark end of valid data */
  
  if (bytes_read == -1) return NULL;
  else return my_buf;
}

int tabize(int val, int pos) {
  int fpos, i, count, begin_line;
  char *buftab;

  if (!tab_correct) return val;

  if ((fpos = lseek(fd, 0, SEEK_CUR)) == -1)
    exiterr("\nError moving file pointer in tabize().\n");
    
  if ((lseek(fd, fpos-fd_bytes_read, SEEK_SET)) == -1)
    exiterr("\nError moving file pointer in tabize().\n");

  /* Allocate space for read buffer */
  if ((buftab = malloc(val+1)) == NULL)
    exiterr("\nCan't allocate memory in tabize().\n");

  if ((read(fd, buftab, val)) == -1)
    exiterr("\nError reading file in tabize().\n");

  begin_line=count=0;
  for (i=0; i<val; i++) {
    if (pos && count >= val) {
      count=i;  /* it's the retval */
      break;
    }
    if (buftab[i] == TAB)
      count+=tab_len-((count-begin_line)%tab_len);
    else if (buftab[i] == '\n') {
      count++;
      begin_line=count;
    } else
      count++;
  }

  if (lseek(fd, fpos, SEEK_SET) == -1)
    exiterr("\nError moving file pointer in tabize().\n");
  
  return count;
}
