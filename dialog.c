/*
 * $Id: dialog.c,v 1.22 2000/02/04 01:13:27 tom Exp $
 *
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
#include <string.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

/* globals */
char *backtitle;
char *lock_refresh;
char *lock_tailbg_exit;
char *lock_tailbg_refreshed;
int aspect_ratio;
int beep_signal;
int begin_set;
int begin_x;
int begin_y;
int cant_kill;
int cr_wrap;
int defaultno = FALSE;
int is_tailbg = 0;
int print_siz;
int screen_initialized = 0;
int separate_output;		/* local file */
int size_err;
int tab_correct;
int tab_len;
pid_t tailbg_lastpid = 0;
pid_t tailbg_nokill_lastpid = 0;
pid_t tailbg_nokill_pids[MAX_TAILBG];
pid_t tailbg_pids[MAX_TAILBG];

#define JUMPARGS const char *t, char *av[], int offset, int *offset_add
typedef int (jumperFn) (JUMPARGS);

struct Mode {
    const char *name;
    int argmin, argmax;
    jumperFn *jumper;
};

static const char *program = "dialog";
static const char *and_widget = "--and-widget";

static void
Usage(char *msg)
{
    const char *format = "Error: %s.\nUse --help to list options.\n\n";
    char *str = malloc(strlen(msg) + strlen(format) + 1);
    sprintf(str, format, msg);
    exiterr(str);
}

/*
 * Count arguments, stopping at the end of the argument list, or on the token
 * --and-widget.
 */
static int
arg_rest(char *argv[])
{
    int i = 0;

    while (argv[++i] != 0
	&& strcmp(argv[i], and_widget) != 0)
	continue;
    return i;
}

/*
 * In MultiWidget this function is needed to count how many tags
 * a widget (menu, checklist, radiolist) has
 */
static int
howmany_tags(char *argv[], int group)
{
    int result = 0;
    int have;
    const char *format = "Expected %d arguments, found only %d";
    char temp[80];

    while (argv[0] != 0) {
	if (!strcmp(argv[0], and_widget))
	    break;
	if ((have = arg_rest(argv)) < group) {
	    sprintf(temp, format, group, have);
	    Usage(temp);
	}

	argv += group;
	result++;
    }
    if (argv[0] != 0
	&& strcmp(argv[0], and_widget) != 0
	&& (have = arg_rest(argv)) < group) {
	sprintf(temp, format, group, have);
	Usage(temp);
    }

    return result;
}

/*
 * These are the program jumpers
 */

static int
j_yesno(JUMPARGS)
{
    *offset_add = 4;
    return dialog_yesno(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]), defaultno);
}

static int
j_msgbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_msgbox(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]), 1);
}

static int
j_infobox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_msgbox(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]), 0);
}

static int
j_textbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_textbox(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]));
}

static int
j_menu(JUMPARGS)
{
    int ret;
    int tags = howmany_tags(av + offset + 6, 2);
    *offset_add = 5 + tags * 2;
    ret = dialog_menu(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	atoi(av[offset + 5]),
	tags, av + offset + 6);
    if (ret >= 0) {
	fprintf(stderr, av[offset + 6 + ret * 2]);
	return 0;
    } else if (ret == -2)
	return 1;		/* CANCEL */
    return ret;			/* ESC pressed, ret == -1 */
}

static int
j_checklist(JUMPARGS)
{
    int tags = howmany_tags(av + offset + 6, 3);
    *offset_add = 5 + tags * 3;
    return dialog_checklist(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	atoi(av[offset + 5]),
	tags, av + offset + 6, FLAG_CHECK, separate_output);
}

static int
j_radiolist(JUMPARGS)
{
    int tags = howmany_tags(av + offset + 6, 3);
    *offset_add = 5 + tags * 3;
    return dialog_checklist(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	atoi(av[offset + 5]),
	tags, av + offset + 6, FLAG_RADIO, separate_output);
}

static int
j_inputbox(JUMPARGS)
{
    int ret;
    char *init_inputbox = 0;

    if (arg_rest(av + offset) >= 5)
	init_inputbox = av[offset + 5];

    *offset_add = 4 + ((init_inputbox == NULL) ? 0 : 1);
    ret = dialog_inputbox(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	init_inputbox, 0);
    if (ret == 0)
	fprintf(stderr, "%s", dialog_input_result);
    return ret;
}

static int
j_passwordbox(JUMPARGS)
{
    int ret;
    char *init_inputbox = 0;

    if (arg_rest(av + offset) >= 5)
	init_inputbox = av[offset + 5];

    *offset_add = 4 + ((init_inputbox == NULL) ? 0 : 1);
    ret = dialog_inputbox(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	init_inputbox, 1);
    if (ret == 0)
	fprintf(stderr, "%s", dialog_input_result);
    return ret;
}

#ifdef HAVE_GAUGE
static int
j_gauge(JUMPARGS)
{
    *offset_add = 5;
    return dialog_gauge(t,
	av[offset + 2],
	atoi(av[offset + 3]),
	atoi(av[offset + 4]),
	atoi(av[offset + 5]));
}
#endif

#ifdef HAVE_TAILBOX
static int
j_tailbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_tailbox(t, av[offset + 2], atoi(av[offset + 3]),
	atoi(av[offset + 4]));
}

static int
j_tailboxbg(JUMPARGS)
{
    if (tailbg_lastpid + 1 >= MAX_TAILBG) {
	char temp[80];
	sprintf(temp, "lastpid value %d is greater than %d",
	    tailbg_lastpid, MAX_TAILBG);
	Usage(temp);
    }

    *offset_add = 4;
    refresh();			/* doupdate(); */
    if (cant_kill)
	tailbg_nokill_pids[tailbg_nokill_lastpid] =
	    tailbg_pids[tailbg_lastpid] = fork();
    else
	tailbg_pids[tailbg_lastpid] = fork();

    /* warning! here are 2 processes */

    if (tailbg_pids[tailbg_lastpid] == 0) {
	if (cant_kill)
	    fprintf(stderr, "%d", (int) getpid());
	is_tailbg = 1;
	dialog_tailboxbg(t,
	    av[offset + 2],
	    atoi(av[offset + 3]),
	    atoi(av[offset + 4]), cant_kill);

	refresh();		/* quitting for signal 15 and --no-kill */
	endwin();
	exit(0);
    }
    if (cant_kill)
	tailbg_nokill_lastpid++;
    tailbg_lastpid++;
    return 0;
}
#endif

/*
 * All functions are used in the slackware root disk, apart from "gauge"
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
    {"--passwordbox", 5, 6, j_passwordbox},
#ifdef HAVE_GAUGE
    {"--gauge", 6, 6, j_gauge},
    {"--guage", 6, 6, j_gauge},	/* obsolete - for compatibility only */
#endif
#ifdef HAVE_TAILBOX
    {"--tailbox", 5, 5, j_tailbox},
    {"--tailboxbg", 5, 5, j_tailboxbg},
#endif
    {NULL, 0, 0, NULL}
};

static char *
optionString(char **argv, int *num)
{
    int next = *num + 2;
    char *result = argv[next];
    if (result == 0) {
	char temp[80];
	sprintf(temp, "Expected a string-parameter for %.20s", argv[1]);
	Usage(temp);
    }
    *num = (next - 1);
    return result;
}

static int
optionValue(char **argv, int *num)
{
    int next = *num + 2;
    char *src = argv[next];
    char *tmp = 0;
    int result = 0;

    if (src != 0) {
	result = strtol(src, &tmp, 0);
	if (tmp == 0 || *tmp != 0)
	    src = 0;
    }

    if (src == 0) {
	char temp[80];
	sprintf(temp, "Expected a numeric-parameter for %.20s", argv[1]);
	Usage(temp);
    }
    *num = (next - 1);
    return result;
}

/*
 * Print program help-message
 */
static void
Help(void)
{
    static const char *tbl[] =
    {
	"cdialog (ComeOn Dialog!) version %s by Pako (demarco_p@abramo.it)",
	"originally dialog version 0.3, by Savio Lam (lam836@cs.cuhk.hk).",
	"  patched to version 0.4 by Stuart Herbert (S.Herbert@shef.ac.uk)",
	"",
	"* Display dialog boxes from shell scripts *",
	"",
	"Usage: %s <options> { --and-widget <options> }",
	"where options are \"common\" options, followed by \"box\" options",
	"",
#ifdef HAVE_RC_FILE
	"Special options:",
	" [--create-rc \"Ifile\"]",
#endif
	"Common options:"
	," "
	" [--aspect <ratio>]"
	" [--backtitle <backtitle>]"
	" [--beep]"
	" [--beep-after]"
	," "
	" [--begin <y> <x>]"
	" [--clear]"
	" [--cr-wrap]"
	" [--defaultno]"
	" [--no-kill]"
	," "
	" [--no-shadow]"
	" [--print-maxsize]"
	" [--print-size]"
	" [--print-version]"
	," "
	" [--separate-output]"
	" [--separate-widget \"<str>\"]"
	" [--shadow]"
	" [--size-err]"
	," "
	" [--sleep <secs>]"
	" [--tab-correct]"
	" [--tab-len <n>]"
	" [--title <title>]"
	,"",
	"Box options:",
	"  --checklist <text> <height> <width> <list height> <tag1> <item1> <status1>...",
	"  --gauge     <text> <height> <width> <percent>",
	"  --infobox   <text> <height> <width>",
	"  --inputbox  <text> <height> <width> [<init>]",
	"  --menu      <text> <height> <width> <menu height> <tag1> <item1>...",
	"  --msgbox    <text> <height> <width>",
	"  --passwordbox <text> <height> <width> [<init>]",
	"  --radiolist <text> <height> <width> <list height> <tag1> <item1> <status1>...",
	"  --tailboxbg <file> <height> <width>",
	"  --tailbox   <file> <height> <width>",
	"  --textbox   <file> <height> <width>",
	"  --yesno     <text> <height> <width>",
	"",
	"Auto-size with height and width = 0. Maximize with height and width = -1.",
	"Global-auto-size if also menu_height/list_height = 0.",
    };
    unsigned n;
    const char *leaf = strrchr(program, '/');

    if (leaf != 0)
	leaf++;
    else
	leaf = program;

    for (n = 0; n < sizeof(tbl) / sizeof(tbl[0]); n++) {
	fprintf(stderr, tbl[n], n ? leaf : VERSION);
	fputc('\n', stderr);
    }

    exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
    char temp[80];
    char *title;
    const char *separate_str = DEFAULT_SEPARATE_STR;
    int beep_after_signal;
    int clear_screen;
    int esc_pressed = 0;
    int offset = 0;
    int offset_add;
    int retval = 0;
    int sleep_secs;
    struct Mode *modePtr;
#ifndef HAVE_COLOR
    int use_shadow = FALSE;	/* ignore corresponding option */
#endif

#ifdef HAVE_SETLOCALE
    (void) setlocale(LC_ALL, "");
#endif

    program = argv[0];

    if (argc == 2) {		/* if we don't want clear screen */
	if (!strcmp(argv[1], "--print-maxsize")) {
	    initscr();
	    fprintf(stderr, "MaxSize: %d, %d\n", SLINES, SCOLS);
	    endwin();
	} else if (!strcmp(argv[1], "--print-version")) {
	    fprintf(stderr, "Version: %s\n", VERSION);
	} else if (!strcmp(argv[1], "--help")) {
	    Help();
	}
	return 0;
    }

    if (argc < 2) {
	Help();
    }

    init_dialog();

#ifdef HAVE_RC_FILE
    if (!strcmp(argv[1], "--create-rc"))
#ifdef NCURSES_VERSION
    {
	if (argc != 3) {
	    sprintf(temp, "Expected a filename for %s", argv[1]);
	    Usage(temp);
	}
	end_dialog();
	create_rc(argv[2]);
	return 0;
    }
#else
	exiterr("This option is currently unsupported on your system.");
#endif
#endif

    if ((lock_refresh = make_lock_filename("/tmp/.lock_fileXXXXXX")) == NULL ||
	(lock_tailbg_refreshed =
	    make_lock_filename("/tmp/.lock_tailbgXXXXXX")) == NULL ||
	(lock_tailbg_exit = make_lock_filename("/tmp/.lock_exitXXXXXX")) == NULL)
	exiterr("Internal error: can't make lock files.");

    while (offset < argc - 1 && !esc_pressed) {
	aspect_ratio = DEFAULT_ASPECT_RATIO;
	backtitle = NULL;
	beep_after_signal = 0;
	beep_signal = 0;
	begin_set = 0;
	begin_x = 0;
	begin_y = 0;
	cant_kill = 0;
	clear_screen = 0;
	cr_wrap = 0;
	print_siz = 0;
	separate_output = 0;
	size_err = 0;
	sleep_secs = 0;
	tab_correct = 0;
	tab_len = TAB_LEN;
	title = NULL;

	while (offset < argc - 1) {	/* Common options */
	    char *option = argv[offset + 1];
	    if (!strcmp(option, "--title")) {
		title = optionString(argv, &offset);
	    } else if (!strcmp(option, "--backtitle")) {
		backtitle = optionString(argv, &offset);
	    } else if (!strcmp(option, "--separate-widget")) {
		separate_str = optionString(argv, &offset);
	    } else if (!strcmp(option, "--separate-output")) {
		separate_output = 1;
	    } else if (!strcmp(option, "--cr-wrap")) {
		cr_wrap = 1;
	    } else if (!strcmp(option, "--no-kill")) {
		cant_kill = 1;
	    } else if (!strcmp(option, "--size-err")) {
		size_err = 1;
	    } else if (!strcmp(option, "--beep")) {
		beep_signal = 1;
	    } else if (!strcmp(option, "--beep-after")) {
		beep_after_signal = 1;
	    } else if (!strcmp(option, "--shadow")) {
		use_shadow = TRUE;
	    } else if (!strcmp(option, "--defaultno")) {
		defaultno = TRUE;
	    } else if (!strcmp(option, "--no-shadow")) {
		use_shadow = FALSE;
	    } else if (!strcmp(option, "--print-size")) {
		print_siz = 1;
	    } else if (!strcmp(option, "--print-maxsize")) {
		fprintf(stderr, "MaxSize: %d, %d\n", SLINES, SCOLS);
	    } else if (!strcmp(option, "--print-version")) {
		fprintf(stderr, "Version: %s\n", VERSION);
	    } else if (!strcmp(option, "--tab-correct")) {
		tab_correct = 1;
	    } else if (!strcmp(option, "--sleep")) {
		sleep_secs = optionValue(argv, &offset);
	    } else if (!strcmp(option, "--tab-len")) {
		tab_len = optionValue(argv, &offset);
	    } else if (!strcmp(option, "--aspect")) {
		aspect_ratio = optionValue(argv, &offset);
	    } else if (!strcmp(option, "--begin")) {
		begin_set = 1;
		begin_y = optionValue(argv, &offset);
		begin_x = optionValue(argv, &offset);
	    } else if (!strcmp(option, "--clear")) {
		if (argc == 2) {	/* we only want to clear the screen */
		    killall_bg();
		    refresh();
		    end_dialog();
		    return 0;
		} else {
		    clear_screen = 1;
		}
	    } else {		/* no more common options */
		break;
	    }
	    offset++;
	}

	/* if (separate_output==1 and no-checklist) or (no more options) ... */

	if ((argv[offset + 1] != NULL
		&& (strcmp(argv[offset + 1], "--checklist"))
		&& (separate_output == 1))
	    || (argc - 1 == offset)) {
	    sprintf(temp, "Expected --checklist, not %.20s", argv[offset + 1]);
	    Usage(temp);
	}

	if (aspect_ratio == 0)
	    aspect_ratio = DEFAULT_ASPECT_RATIO;

	put_backtitle();

	/* use a table to look for the requested mode, to avoid code duplication */

	for (modePtr = modes; modePtr->name; modePtr++)		/* look for the mode */
	    if (!strcmp(argv[offset + 1], modePtr->name))
		break;

	if (!modePtr->name) {
	    sprintf(temp, "Unknown option %.20s", argv[offset + 1]);
	    Usage(temp);
	}
	if (arg_rest(&argv[offset]) < modePtr->argmin) {
	    sprintf(temp, "Expected at least %d tokens for %.20s",
		modePtr->argmin, argv[offset + 1]);
	    Usage(temp);
	}
	if (modePtr->argmax && arg_rest(&argv[offset]) > modePtr->argmax) {
	    sprintf(temp, "Expected no more than %d tokens for %.20s",
		modePtr->argmax, argv[offset + 1]);
	    Usage(temp);
	}

	retval = (*(modePtr->jumper)) (title, argv, offset, &offset_add);

	if (retval == -1)
	    esc_pressed = 1;

	offset += offset_add;

	if (!esc_pressed) {

	    if (beep_after_signal)
		beep();

	    if (sleep_secs)
		napms(sleep_secs * 1000);

	    if (clear_screen)
		attr_clear(stdscr, LINES, COLS, screen_attr);

	    if (offset + 1 < argc) {
		if (!strcmp(argv[offset + 1], and_widget)) {
		    fprintf(stderr, separate_str);
		    offset++;
		} else {
		    sprintf(temp, "Expected --and-widget, not %.20s",
			    argv[offset + 1]);
		    Usage(temp);
		}
	    }
	}

    }

    quitall_bg();		/* instead of killall_bg() */
    refresh();
    end_dialog();
    return retval;
}
/* End of main() */
