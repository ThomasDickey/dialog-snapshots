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

#ifndef DIALOG_H_included
#define DIALOG_H_included 1

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

#if defined(HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined(ultrix)
#include <cursesX.h>
#else
#include <curses.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(s) gettext(s)
#else
#undef _
#define _(s) s
#endif

/*
 * Change these if you want
 */
#define USE_SHADOW TRUE
#define USE_COLORS TRUE

#ifdef HAVE_COLOR
#define SCOLS	COLS-(use_shadow ? 2 : 0)
#define SLINES	LINES-(use_shadow ? 1 : 0)
#else
#define SCOLS	COLS
#define SLINES	LINES
#endif

#define EXIT_ESC	-1
#define EXIT_OK		0
#define EXIT_CANCEL	1

#define CHR_BACKSPACE	8
#define CHR_REPAINT	12	/* control/L */
#define CHR_DELETE	127
#define CHR_NEXT	('n' & 0x1f)
#define CHR_PREVIOUS	('p' & 0x1f)

#define ESC 27
#define TAB 9

#define MARGIN 1

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

#ifndef A_CHARTEXT
#define A_CHARTEXT 0xff
#endif

#define CharOf(ch)  ((ch) & A_CHARTEXT)

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

/* these definitions really would work for ncurses 1.8.6, etc. */
#ifndef getparyx
#define getparyx(win,y,x)	(y = (win)?(win)->_pary:ERR, x = (win)?(win)->_parx:ERR)
#endif

#ifndef getparx
#define getparx(win)		((win)?(win)->_parx:ERR)
#endif

#ifndef getpary
#define getpary(win)		((win)?(win)->_pary:ERR)
#endif

#ifndef getcurx
#define getcurx(win)		((win)?(win)->_curx:ERR)
#endif

#ifndef getcury
#define getcury(win)		((win)?(win)->_cury:ERR)
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
#define itemhelp_attr                 attributes[29]

/* number of attributes */
#define ATTRIBUTE_COUNT               30

typedef struct {
	char *filename;
	int identifier;
} DIALOCK;

/*
 * Global variables
 */
typedef struct {
	FILE *output;
	bool beep_after_signal;
	bool beep_signal;
	bool begin_set;
	bool cant_kill;
	bool clear_screen;
	bool cr_wrap;
        bool item_help;
	bool nocancel;
	bool print_siz;
	bool separate_output;
	bool size_err;
	bool tab_correct;
	char *backtitle;
	char *title;
	char *default_item;
	int aspect_ratio;
	int begin_x;
	int begin_y;
	int sleep_secs;
	int tab_len;
} DIALOG_VARS;

#define CHECKBOX_TAGS (dialog_vars.item_help ? 4 : 3)
#define MENUBOX_TAGS (dialog_vars.item_help ? 3 : 2)

extern DIALOG_VARS dialog_vars;

#ifdef HAVE_COLOR
extern bool use_colors;
extern bool use_shadow;
#endif

#ifndef HAVE_TYPE_CHTYPE
#define chtype long
#endif

#define UCH(ch) ((unsigned char)(ch))

extern FILE *pipe_fp;
extern chtype attributes[];
extern int defaultno;
extern int is_tailbg;
extern int screen_initialized;
extern DIALOCK lock_refresh;
extern DIALOCK lock_tailbg_refreshed;
extern DIALOCK lock_tailbg_exit;
extern pid_t tailbg_pids[MAX_TAILBG];
extern pid_t tailbg_lastpid;
extern pid_t tailbg_nokill_pids[MAX_TAILBG];
extern pid_t tailbg_nokill_lastpid;

/*
 * Function prototypes
 */
#ifdef HAVE_RC_FILE
extern void create_rc (const char *filename);
extern int parse_rc (void);
#endif

void attr_clear (WINDOW * win, int height, int width, chtype attr);
void dialog_clear (void);
void end_dialog (void);
void init_dialog (void);
void put_backtitle(void);

void auto_size(const char * title, char *prompt, int *height, int *width, int boxlines, int mincols);
void auto_sizefile(const char * title, const char *file, int *height, int *width, int boxlines, int mincols);

int make_lock_filename(DIALOCK *dialock, const char *prefix);
void wrefresh_lock(WINDOW *win);
void ctl_idlemsg(WINDOW *win);
void wrefresh_lock_tailbg(WINDOW *win);

int exist_lock(DIALOCK *dialock);
void create_lock(DIALOCK *dialock);
void remove_lock(DIALOCK *dialock);

int quitall_bg(void);
void killall_bg(void);
void exiterr(const char *, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;
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
WINDOW * sub_window (WINDOW *win, int y, int x, int height, int width);

#ifdef HAVE_COLOR
void color_setup (void);
void draw_shadow (WINDOW * win, int height, int width, int y, int x);
#endif

void print_autowrap(WINDOW *win, const char *prompt, int height, int width);
void draw_box (WINDOW * win, int y, int x, int height, int width, chtype boxchar,
		chtype borderchar);

int dialog_yesno (const char *title, const char *cprompt, int height, int width, int dftno);
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
int dialog_fselect (const char *title, const char *path, int height, int width);
int dialog_calendar (const char *title, const char *subtitle, int height, int width, int day, int month, int year);
int dialog_timebox (const char *title, const char *subtitle, int height, int width, int hour, int minute, int second);

/* arrows.c */
void dlg_draw_arrows(WINDOW *dialog, int top_arrow, int bottom_arrow, int x, int top, int bottom);

/* button.c */
extern const char ** dlg_exit_label(void);
extern const char ** dlg_ok_label(void);
extern const char ** dlg_ok_labels(void);
extern const char ** dlg_yes_labels(void);
extern int dlg_char_to_button(int ch, const char **labels);
extern int dlg_button_x_step(const char **labels, int limit, int *gap, int *margin, int *step);
extern void dlg_button_layout(const char **labels, int *limit);
extern void dlg_button_sizes(const char **labels, int vertical, int *longest, int *length);
extern void dlg_draw_buttons(WINDOW *win, int y, int x, const char **labels, int selected, int vertical, int limit);

/* inputstr.c */
extern bool dlg_edit_string(char *string, int *offset, int key, bool force);
extern void dlg_show_string(WINDOW *win, char *string, int offset, chtype attr, int y_base, int x_base, int x_last, bool hidden, bool force);

/* util.c */
extern int dlg_default_item(char **items, int llen);
extern int dlg_getc(WINDOW *win);
extern void dlg_item_help(char *txt);
extern void dlg_set_focus(WINDOW *parent, WINDOW *win);
extern void dlg_trim_string(char *src);
#ifdef HAVE_STRCASECMP
#define dlg_strcmp(a,b) strcasecmp(a,b)
#else
extern int dlg_strcmp(char *a, char *b);
#endif

/*
 * The following stuff is needed for mouse support
 */
typedef struct mseRegion {
    int x, y, X, Y, code;
    struct mseRegion *next;
} mseRegion;

#if defined(NCURSES_MOUSE_VERSION)

#define	mouse_open() mousemask(BUTTON1_CLICKED, (mmask_t *) 0)
#define	mouse_close() mousemask(0, (mmask_t *) 0)

void mouse_free_regions (void);
void mouse_mkregion (int y, int x, int height, int width, int code);
void mouse_mkbigregion (int y, int x, int height, int width, int nitems, int th, int mode);
void mouse_setbase (int x, int y);

#define USE_MOUSE 1

#elif defined(HAVE_LIBGPM)

void mouse_open (void);
void mouse_close (void);
#define mouse_free_regions() /* nothing */
void mouse_mkregion (int y, int x, int height, int width, int code);
void mouse_mkbigregion (int y, int x, int height, int width, int nitems, int th, int mode);
void mouse_setbase (int x, int y);

#define USE_MOUSE 1

#else

#define	mouse_open() /*nothing*/
#define	mouse_close() /*nothing*/
#define mouse_free_regions() /* nothing */
#define	mouse_mkregion(y, x, height, width, code) /*nothing*/
#define	mouse_mkbigregion(y, x, height, width, nitems, th, mode) /*nothing*/
#define	mouse_setbase(x, y) /*nothing*/

#define USE_MOUSE 0

#endif

extern mseRegion *mouse_region (int y, int x);
extern int mouse_wgetch (WINDOW *);

#define mouse_mkbutton(y,x,len,code) mouse_mkregion(y,x,1,len,code);

/*
 * This is the base for fictitious keys, which activate
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

#endif /* DIALOG_H_included */
