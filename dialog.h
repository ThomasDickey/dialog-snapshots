/*
 *  dialog.h -- common declarations for all dialog modules
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define RETSIGTYPE void
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>	/* fork() etc. */
#include <math.h>	/* sqrt() */

#ifdef ultrix
#include <cursesX.h>
#else
#include <curses.h>
#endif

/*
 * Change these if you want
 */
#define USE_SHADOW TRUE
#define USE_COLORS TRUE

#define SCOLS	COLS-(use_shadow ? 2 : 0)
#define SLINES	LINES-(use_shadow ? 1 : 0)

#define ESC 27
#define TAB 9
#define MAX_LEN 2048
#define BUF_SIZE (10*1024)
#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

#define MAX_TAILBG 9
#define DEFAULT_SEPARATE_STR "\t"
#define DEFAULT_ASPECT_RATIO 9
/* how many spaces is a tab long (default)? */
#define TAB_LEN 8
#define WTIMEOUT_VAL        10


#ifndef NCURSES_VERSION
#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE '-'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE '|'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE '+'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE '+'
#endif
#ifndef ACS_UARROW
#define ACS_UARROW '^'
#endif
#ifndef ACS_DARROW
#define ACS_DARROW 'v'
#endif
#endif

/* 
 * Attribute names
 */
#define screen_attr                   attributes[0]
#define shadow_attr                   attributes[1]
#define dialog_attr                   attributes[2]
#define title_attr                    attributes[3]
#define border_attr                   attributes[4]
#define button_active_attr            attributes[5]
#define button_inactive_attr          attributes[6]
#define button_key_active_attr        attributes[7]
#define button_key_inactive_attr      attributes[8]
#define button_label_active_attr      attributes[9]
#define button_label_inactive_attr    attributes[10]
#define inputbox_attr                 attributes[11]
#define inputbox_border_attr          attributes[12]
#define searchbox_attr                attributes[13]
#define searchbox_title_attr          attributes[14]
#define searchbox_border_attr         attributes[15]
#define position_indicator_attr       attributes[16]
#define menubox_attr                  attributes[17]
#define menubox_border_attr           attributes[18]
#define item_attr                     attributes[19]
#define item_selected_attr            attributes[20]
#define tag_attr                      attributes[21]
#define tag_selected_attr             attributes[22]
#define tag_key_attr                  attributes[23]
#define tag_key_selected_attr         attributes[24]
#define check_attr                    attributes[25]
#define check_selected_attr           attributes[26]
#define uarrow_attr                   attributes[27]
#define darrow_attr                   attributes[28]

/* number of attributes */
#define ATTRIBUTE_COUNT               29

/*
 * Global variables
 */
#ifdef HAVE_COLOR
extern bool use_colors;
extern bool use_shadow;
#endif

extern chtype attributes[];

extern int is_tailbg, print_siz, tab_len, tab_correct;

/* Coordinate in begin_y, begin_x  if begin_set == 1 */
extern int begin_y, begin_x, begin_set, aspect_ratio, screen_initialized;

extern char *backtitle, *lock_refresh, *lock_tailbg_refreshed,
        *lock_tailbg_exit;
extern int beep_signal, is_tailbg, print_siz, cr_wrap, size_err,
        tab_len, tab_correct;
extern pid_t tailbg_pids[MAX_TAILBG], tailbg_lastpid, tailbg_nokill_pids[MAX_TAILBG], tailbg_nokill_lastpid;

/*
 * Function prototypes
 */
#ifdef NCURSES_VERSION
extern void create_rc (const char *filename);
extern int parse_rc (void);
#endif


void init_dialog (void);
void end_dialog (void);
void attr_clear (WINDOW * win, int height, int width, chtype attr);
void put_backtitle(void);

char *auto_size(const char * title, char *prompt, int *height, int *width, int boxlines, int mincols);
void auto_sizefile(const char * title, const char *file, int *height, int *width, int boxlines, int mincols);

char *make_lock_filename(const char *filename);
void wrefresh_lock(WINDOW *win);
void ctl_idlemsg(WINDOW *win);
void wrefresh_lock_tailbg(WINDOW *win);
void wrefresh_lock_sub(WINDOW *win);

int exist_lock(char *filename);
void while_exist_lock(char *filename);
void create_lock(char *filename);
void remove_lock(char *filename);

void killall_bg(void);
void quitall_bg(void);
void exiterr(const char *);
void beeping(void);
void print_size(int height, int width);
void ctl_size(int height, int width);
void tab_correct_str(char *prompt);
void calc_listh(int *height, int *list_height, int item_no);
int calc_listw(int item_no, char **items, int group);
char *strclone(const char *cprompt);
int box_x_ordinate(int width);
int box_y_ordinate(int height);
void draw_title(WINDOW *win, const char *title);
void draw_bottom_box(WINDOW *win);
WINDOW * new_window (int y, int x, int height, int width);

#ifdef HAVE_COLOR
void color_setup (void);
void draw_shadow (WINDOW * win, int height, int width, int y, int x);
#endif

void print_autowrap(WINDOW *win, const char *prompt, int width, int y, int x);
void print_button (WINDOW * win, const char *label, int y, int x, int selected);
void draw_box (WINDOW * win, int y, int x, int height, int width, chtype boxchar,
		chtype borderchar);

int dialog_yesno (const char *title, const char *cprompt, int height, int width, int defaultno);
int dialog_msgbox (const char *title, const char *cprompt, int height,
		int width, int pauseopt);
int dialog_textbox (const char *title, const char *file, int height, int width);
int dialog_menu (const char *title, const char *cprompt, int height, int width,
		int menu_height, int item_no, char **items);
int dialog_checklist (const char *title, const char *cprompt, int height,
		int width, int list_height, int item_no,
		char ** items, int flag, int separate_output);
extern unsigned char dialog_input_result[];
int dialog_inputbox (const char *title, const char *cprompt, int height,
		int width, const char *init, const int password);
int dialog_gauge (const char *title, const char *cprompt, int height, int width,
		int percent);
int dialog_tailbox (const char *title, const char *file, int height, int width);
void dialog_tailboxbg (const char *title, const char *file, int height, int width, int cant_kill);

/*
 * The following stuff is needed for mouse support
 */
#ifndef HAVE_LIBGPM

/*
 * This used to be defined as "extern __inline__", which caused problems
 * when building with -g . That might be a gcc bug, but what does a compiler
 * do with an "extern" inline? Inline it and then generate an
 * externaly-callable version? Ugh!
 */
#define	mouse_open() {}
#define	mouse_close() {}
#define	mouse_mkregion(y, x, height, width, code) {}
#define	mouse_mkbigregion(y, x, height, width, nitems, th, mode) {}
#define	mouse_setbase(x, y) {}

#else

void mouse_open (void);
void mouse_close (void);
void mouse_mkregion (int y, int x, int height, int width, int code);
void mouse_mkbigregion (int y, int x, int height, int width, int nitems,
			int th, int mode);
void mouse_setbase (int x, int y);

#endif

int mouse_wgetch (WINDOW *);

#define mouse_mkbutton(y,x,len,code) mouse_mkregion(y,x,1,len,code);

/*
 * This is the base for fictious keys, which activate
 * the buttons.
 *
 * Mouse-generated keys are the following:
 *   -- the first 32 are used as numbers, in addition to '0'-'9'
 *   -- the lowercase are used to signal mouse-enter events (M_EVENT + 'o')
 *   -- uppercase chars are used to invoke the button (M_EVENT + 'O')
 */
#define M_EVENT (KEY_MAX+1)


/*
 * The `flag' parameter in checklist is used to select between
 * radiolist and checklist
 */
#define FLAG_CHECK 1
#define FLAG_RADIO 0
