/*
 *  cdialog - Display simple dialog boxes from shell scripts
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

static void Usage (const char *name);
static int howmany_tags(int argc, const char * const * argv, int offset, int group);
static int arg_rest( int argc, const char * const * argv, int offset );

/* globals */
char *backtitle, *lock_refresh, *lock_tailbg_refreshed,
	*lock_tailbg_exit;
int beep_signal, is_tailbg = 0, print_siz, cr_wrap, size_err,
	tab_len, tab_correct;
int begin_x, begin_y, begin_set, aspect_ratio, screen_initialized = 0;
pid_t tailbg_pids[MAX_TAILBG], tailbg_lastpid = 0, tailbg_nokill_pids[MAX_TAILBG], tailbg_nokill_lastpid = 0;
int cant_kill;

int separate_output;	/* local file */

typedef int (jumperFn) (const char *title, int argc, const char * const * argv, int offset, int *offset_add);

struct Mode {
    char *name;
    int argmin, argmax;
    jumperFn *jumper;
};

jumperFn j_yesno, j_msgbox, j_infobox, j_textbox, j_menu;
jumperFn j_checklist, j_radiolist, j_inputbox, j_guage;
jumperFn j_tailbox, j_tailboxbg;

/*
 * All functions are used in the slackware root disk, apart from "guage"
 */

static struct Mode modes[] =
{
    {"--yesno", 5, 5, j_yesno},
    {"--msgbox", 5, 5, j_msgbox},
    {"--infobox", 5, 5, j_infobox},
    {"--textbox", 5, 5, j_textbox},
    {"--menu", 8, 0, j_menu},
    {"--checklist", 9, 0, j_checklist},
    {"--radiolist", 9, 0, j_radiolist},
    {"--inputbox", 5, 6, j_inputbox},
#ifdef HAVE_GUAGE
    {"--guage", 6, 6, j_guage},
#endif
#ifdef HAVE_TAILBOX
    {"--tailbox", 5, 5, j_tailbox},
    {"--tailboxbg", 5, 5, j_tailboxbg},
#endif
    {NULL, 0, 0, NULL}
};

static struct Mode *modePtr;

#ifdef LOCALE
#include <locale.h>
#endif

int
main (int argc, const char * const * argv)
{
    int offset = 0, clear_screen, end_common_opts, retval = 0, sleep_secs,
        beep_after_signal, tab_set, offset_add, esc_pressed = 0;
    char *title, separate_str[MAX_LEN];

#ifdef LOCALE
    (void) setlocale (LC_ALL, "");
#endif

    if (argc == 2) { 	/* if we don't want clear screen */
      if (!strcmp(argv[1], "--print-maxsize" ))
      {
        initscr ();
        fprintf(stderr, "MaxSize: %d, %d\n", SLINES, SCOLS);
        endwin ();
      }
      else if (!strcmp(argv[1], "--print-version" ))
      {
        fprintf(stderr, "Version: %s\n", VERSION);
      }

      return 0;
    }

    init_dialog ();

    if (argc < 2)
	Usage (argv[0]);
#ifdef HAVE_RC_FILE

    else if (!strcmp (argv[1], "--create-rc"))
#ifdef HAVE_NCURSES
    {
	if (argc != 3)
	    Usage (argv[0]);
	end_dialog();
	create_rc (argv[2]);
	return 0;
    }
#else
	exiterr("\nThis option is currently unsupported on your system.\n");
#endif
#endif

 if ((lock_refresh=make_lock_filename("/tmp/.lock_fileXXXXXX")) == NULL ||
     (lock_tailbg_refreshed=make_lock_filename("/tmp/.lock_tailbgXXXXXX")) == NULL ||
     (lock_tailbg_exit=make_lock_filename("/tmp/.lock_exitXXXXXX")) == NULL)
    exiterr("\nInternal error: can't make lock files.\n");

 strcpy (separate_str, DEFAULT_SEPARATE_STR);

 while (offset < argc-1 && !esc_pressed) {
  title = NULL; clear_screen = 0; end_common_opts = 0;
  separate_output = 0; backtitle = NULL; sleep_secs = 0;
  begin_set = 0; begin_x = 0; begin_y = 0; aspect_ratio = 0;
  beep_signal = 0; beep_after_signal = 0; print_siz = 0;
  cr_wrap = 0; size_err = 0; tab_set = 0; tab_correct = 0;
  cant_kill = 0;

  while (offset < argc - 1 && !end_common_opts) {	/* Common options */
    if (!strcmp(argv[offset+1], "--title")) {
      if (argc-offset < 3 || title != NULL)    /* No two "--title" please! */
        Usage(argv[0]);
      else {
        title = argv[offset+2];
        offset += 2;
      }
    }
    else if (!strcmp(argv[offset+1], "--backtitle" ))
    {
      if (backtitle != NULL)
        Usage(argv[0]);
      else
      {
        backtitle = argv[offset+2];
        offset += 2;
      }
    }
    else if (!strcmp(argv[offset+1], "--separate-widget" ))
    {
      strcpy(separate_str, argv[offset+2]);
      offset += 2;
    }
    else if (!strcmp(argv[offset+1], "--separate-output" ))
    {
        separate_output = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--cr-wrap" ))
    {
        cr_wrap = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--no-kill" ))
    {
        cant_kill = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--size-err" ))
    {
        size_err = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--beep" ))
    {
        beep_signal = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--beep-after" ))
    {
        beep_after_signal = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--shadow" ))
    {
        use_shadow = TRUE;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--no-shadow" ))
    {
        use_shadow = FALSE;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--print-size" ))
    {
        print_siz = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--print-maxsize" ))
    {
        fprintf(stderr, "MaxSize: %d, %d\n", SLINES, SCOLS);
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--print-version" ))
    {
        fprintf(stderr, "Version: %s\n", VERSION);
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--tab-correct" ))
    {
        tab_correct = 1;
        offset++;
    }
    else if (!strcmp(argv[offset+1], "--sleep")) {
      if (argc-offset < 3 || sleep_secs != 0)
        Usage(argv[0]);
      else {
        sleep_secs = atoi(argv[offset+2]);
        offset += 2;
      }
    }
    else if (!strcmp(argv[offset+1], "--tab-len")) {
      if (argc-offset < 3 || tab_set != 0)
        Usage(argv[0]);
      else {
        tab_len = atoi(argv[offset+2]);
        tab_set = 1;
        offset += 2;
      }
    }
    else if (!strcmp(argv[offset+1], "--aspect")) {
      if (argc-offset < 3 || aspect_ratio != 0)
        Usage(argv[0]);
      else {
        aspect_ratio=atoi(argv[offset+2]);
        offset += 2;
      }
    }
    else if (!strcmp(argv[offset+1], "--begin")) {
      if (argc-offset < 4 || begin_set != 0)
        Usage(argv[0]);
      else {
        begin_y = atoi(argv[offset+2]);
        begin_x = atoi(argv[offset+3]);
        begin_set=1;
        offset += 3;
      }
    }
    else if (!strcmp(argv[offset+1], "--clear")) {
      if (clear_screen)    /* Hey, "--clear" can't appear twice! */
        Usage(argv[0]);
      else if (argc == 2) {    /* we only want to clear the screen */
        killall_bg();
        refresh();
        end_dialog();
        return 0;
      }
      else {
        clear_screen = 1;
        offset++;
      }
    }
    else    /* no more common options */
      end_common_opts = 1;
  }

  /* if (separate_output==1 and no-checklist) or (no more options) ... */
  if ((strcmp(argv[offset+1], "--checklist") && (separate_output == 1)) || (argc-1 == offset))
    Usage(argv[0]);

  if (tab_set == 0)
    tab_len=TAB_LEN;

  if (aspect_ratio == 0)
    aspect_ratio=DEFAULT_ASPECT_RATIO;

  put_backtitle();

    /* use a table to look for the requested mode, to avoid code duplication */

    for (modePtr = modes; modePtr->name; modePtr++)	/* look for the mode */
	if (!strcmp (argv[offset + 1], modePtr->name))
	    break;

    if (!modePtr->name)
	Usage (argv[0]);
    if (arg_rest(argc, argv, offset) < modePtr->argmin)
	Usage (argv[0]);
    if (modePtr->argmax && arg_rest(argc, argv, offset) > modePtr->argmax)
	Usage (argv[0]);

    retval = (*(modePtr->jumper)) (title, argc, argv, offset, &offset_add);

    if (retval == -1) 
        esc_pressed = 1;

    offset+=offset_add;

    if (!esc_pressed) {

      if (beep_after_signal)        beep();

      if (sleep_secs)       sleep(sleep_secs);

      if (clear_screen)
	attr_clear (stdscr, LINES, COLS, screen_attr);

      if (offset+1 < argc) {
        if (!strcmp(argv[offset+1], "--and-widget")) {
          fprintf(stderr, separate_str);
          offset++;
        } else
          Usage(argv[0]);
      }
    }
    
 }

 quitall_bg(); /* instead of killall_bg() */
 refresh();
 end_dialog();
 return retval;
}
/* End of main() */

/*
 * Print program usage
 */
static void 
Usage(const char *name)
{
  char *str=(char *) malloc((size_t) 300); /* change this number for a longer string */

  sprintf(str, "\
\ndialog version 0.3, by Savio Lam (lam836@cs.cuhk.hk).\
\n  patched to version 0.4 by Stuart Herbert (S.Herbert@shef.ac.uk)\
\n    cdialog (ComeOn Dialog!) version %s by Pako (demarco_p@abramo.it)\
\n\
\n* Display dialog boxes from shell scripts *\
\n\
\nUsage: %s <Common options> <Box options> { --and-widget <Common options> <Box options> }\
\n\
\nCommon options: <Global options>\
\n       [--backtitle <backtitle>] [--sleep <secs>] [--beep] [--beep-after]\
\n       [--clear] [--begin <y> <x>] [--aspect <ratio>] [--print-size]\
\n       [--print-maxsize] [--size-err] [--separate-output] [--cr-wrap]\
\n       [--tab-len <n>] [--tab-correct] [--print-version] [--no-kill]\
\n       [--title <title>]\
\n\
\nGlobal options: [--shadow] [--no-shadow] [--separate-widget \"<str>\"]\
\n\
\nBox options:\
\n  --yesno     <text> <height> <width>\
\n  --msgbox    <text> <height> <width>\
\n  --infobox   <text> <height> <width>\
\n  --inputbox  <text> <height> <width> [<init>]\
\n  --textbox   <file> <height> <width>\
\n  --menu      <text> <height> <width> <menu height> <tag1> <item1>...\
\n  --checklist <text> <height> <width> <list height> <tag1> <item1> <status1>...\
\n  --radiolist <text> <height> <width> <list height> <tag1> <item1> <status1>...\
\n  --guage     <text> <height> <width> <percent>\
\n  --tailbox   <file> <height> <width>\
\n  --tailboxbg <file> <height> <width>\
\n\
\n (auto-size with height and width = 0. Maximize with height and width = -1)\
\n (global-auto-size if also menu_height/list_height = 0)\
\n", VERSION, name);

exiterr(str);
}


/*
 * In MultiWidget this function is needed to count how many tags
 * a widget (menu, checklist, radiolist) has
 */
int howmany_tags(int argc, const char * const * argv, int offset, int group)
{
  int i, ptr = offset;

  while (ptr+group <= argc) { /* quantity control */
    if (!strcmp(argv[ptr], "--and-widget")) break;
    for (i = 1; i < group; i++)
      if (!strcmp(argv[ptr+i], "--and-widget")) /* quality control */
        Usage(argv[0]);

    ptr+=group;
  }
  
  if ((ptr != argc) && (ptr+group <= argc) && strcmp(argv[ptr], "--and-widget"))
        Usage(argv[0]);
  
  return (ptr-offset)/group;
}

int arg_rest( int argc, const char * const * argv, int offset ) {
    int i=offset;
    
    while (++i < argc) {
        if (!strcmp (argv[i], "--and-widget"))
            break;
    }    
    return i-offset;
}

/*
 * These are the program jumpers
 */

int
j_yesno (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=4;
    return dialog_yesno (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]));
}

int
j_msgbox (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=4;
    return dialog_msgbox (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]), 1);
}

int
j_infobox (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=4;
    return dialog_msgbox (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]), 0);
}

int
j_textbox (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=4;
    return dialog_textbox (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]));
}

int
j_menu (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    int ret;
    int tags=howmany_tags(ac, av, offset+6, 2);
    *offset_add=5+tags*2;
    ret = dialog_menu (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]),
			atoi (av[offset+5]), tags, av + offset + 6);
    if (ret >= 0) {
	fprintf(stderr, av[offset+ 6 + ret * 2]);
	return 0;
    } else if (ret == -2)
	return 1;	/* CANCEL */
    return ret;		/* ESC pressed, ret == -1 */
}

int
j_checklist (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    int tags=howmany_tags(ac, av, offset+6, 3);
    *offset_add=5+tags*3;
    return dialog_checklist (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]),
	atoi (av[offset+5]), tags, av + offset + 6, FLAG_CHECK, separate_output);
}

int
j_radiolist (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    int tags=howmany_tags(ac, av, offset+6, 3);
    *offset_add=5+tags*3;
    return dialog_checklist (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]),
	atoi (av[offset+5]), tags, av + offset + 6, FLAG_RADIO, separate_output);
}

int
j_inputbox (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    int ret;
    char *init_inputbox=(char *) NULL;
    if (offset+5 < ac)
      if (strcmp(av[offset+5], "--and-widget"))
        init_inputbox=av[offset+5];

    *offset_add=4+((init_inputbox == NULL) ? 0 : 1);
    ret = dialog_inputbox (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]),
			    init_inputbox);
    if (ret == 0)
	fprintf(stderr, dialog_input_result);
    return ret;
}

#ifdef HAVE_GUAGE
int
j_guage (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=5;
    return dialog_guage (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]),
			 atoi (av[offset+5]));
}
#endif

#ifdef HAVE_TAILBOX
int
j_tailbox (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    *offset_add=4;
    return dialog_tailbox (t, av[offset+2], atoi (av[offset+3]), atoi (av[offset+4]));
}

int
j_tailboxbg (const char *t, int ac, const char * const * av, int offset, int *offset_add)
{
    if (tailbg_lastpid+1 >= MAX_TAILBG)
      Usage(av[0]);

    *offset_add=4;
    refresh(); /* doupdate(); */
    if (cant_kill)
        tailbg_nokill_pids[tailbg_nokill_lastpid]=tailbg_pids[tailbg_lastpid]=fork();
    else
        tailbg_pids[tailbg_lastpid]=fork();

    /* warning! here are 2 processes */

    if (tailbg_pids[tailbg_lastpid] == 0) {
        if (cant_kill) 
            fprintf (stderr, "%d", (int) getpid());
        is_tailbg = 1;
        dialog_tailboxbg(t, av[offset+2], atoi(av[offset+3]),
                           atoi(av[offset+4]), cant_kill);
                           
        refresh(); /* quitting for signal 15 and --no-kill */
        endwin();
        exit(0);
    }
    if (cant_kill)
        tailbg_nokill_lastpid++;
    tailbg_lastpid++;
    return 0;
}
#endif
