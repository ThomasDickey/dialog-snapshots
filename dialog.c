/*
 * $Id: dialog.c,v 1.38 2000/07/03 02:00:20 tom Exp $
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
DIALOG_VARS dialog_vars;
char *lock_refresh;
char *lock_tailbg_exit;
char *lock_tailbg_refreshed;
int defaultno = FALSE;
int is_tailbg = FALSE;
int screen_initialized = 0;
pid_t tailbg_lastpid = 0;
pid_t tailbg_nokill_lastpid = 0;
pid_t tailbg_nokill_pids[MAX_TAILBG];
pid_t tailbg_pids[MAX_TAILBG];

#define JUMPARGS const char *t, char *av[], int *offset_add
typedef int (jumperFn) (JUMPARGS);

typedef enum {
    o_unknown = 0
    ,o_and_widget
    ,o_aspect
    ,o_backtitle
    ,o_beep
    ,o_beep_after
    ,o_begin
    ,o_checklist
    ,o_clear
    ,o_cr_wrap
    ,o_create_rc
    ,o_default_item
    ,o_defaultno
    ,o_fselect
    ,o_fullbutton
    ,o_gauge
    ,o_help
    ,o_infobox
    ,o_inputbox
    ,o_menu
    ,o_msgbox
    ,o_no_kill
    ,o_no_shadow
    ,o_nocancel
    ,o_noitem
    ,o_passwordbox
    ,o_print_maxsize
    ,o_print_size
    ,o_print_version
    ,o_radiolist
    ,o_separate_output
    ,o_separate_widget
    ,o_shadow
    ,o_size_err
    ,o_sleep
    ,o_stderr
    ,o_stdout
    ,o_tab_correct
    ,o_tab_len
    ,o_tailbox
    ,o_tailboxbg
    ,o_textbox
    ,o_title
    ,o_yesno
} eOptions;

typedef struct {
    const char *name;
    eOptions code;
    int pass;
    const char *help;
} Options;

typedef struct {
    eOptions code;
    int argmin, argmax;
    jumperFn *jumper;
} Mode;

static const char *program = "dialog";
static const char *and_widget = "--and-widget";
/* *INDENT-OFF* */
static Options options[] = {
    { "and-widget",	o_and_widget,		4, NULL },
    { "aspect",		o_aspect,		1, "<ratio>" },
    { "backtitle",	o_backtitle,		1, "<backtitle>" },
    { "beep",		o_beep,			1, "" },
    { "beep-after",	o_beep_after,		1, "" },
    { "begin",		o_begin,		1, "<y> <x>" },
    { "checklist",	o_checklist,		2, "<text> <height> <width> <list height> <tag1> <item1> <status1>..." },
    { "clear",		o_clear,		1, "" },
    { "cr-wrap",	o_cr_wrap,		1, "" },
    { "create-rc",	o_create_rc,		1, "" },
    { "defaultno",	o_defaultno,		1, "" },
    { "default-item",	o_default_item,		1, "<str>" },
    { "fb",		o_fullbutton,		1, NULL },
    { "fselect",	o_fselect,		2, "<text> <directory> <height> <width>" },
    { "fullbutton",	o_fullbutton,		1, NULL },
    { "gauge",		o_gauge,		2, "<text> <height> <width> [<percent>]" },
    { "guage",		o_gauge,		2, NULL },
    { "help",		o_help,			4, "" },
    { "infobox",	o_infobox,		2, "<text> <height> <width>" },
    { "inputbox",	o_inputbox,		2, "<text> <height> <width> [<init>]" },
    { "menu",		o_menu,			2, "<text> <height> <width> <menu height> <tag1> <item1>..." },
    { "msgbox",		o_msgbox,		2, "<text> <height> <width>" },
    { "no-kill",	o_no_kill,		1, "" },
    { "no-shadow",	o_no_shadow,		1, "" },
    { "nocancel",	o_nocancel,		1, "" },
    { "noitem",		o_noitem,		1, NULL },
    { "passwordbox",	o_passwordbox,		2, "<text> <height> <width> [<init>]" },
    { "print-maxsize",	o_print_maxsize,	1, "" },
    { "print-size",	o_print_size,		1, "" },
    { "print-version",	o_print_version,	5, "" },
    { "radiolist",	o_radiolist,		2, "<text> <height> <width> <list height> <tag1> <item1> <status1>..." },
    { "separate-output",o_separate_output,	1, "" },
    { "separate-widget",o_separate_widget,	1, "<str>" },
    { "shadow",		o_shadow,		1, "" },
    { "size-err",	o_size_err,		1, "" },
    { "sleep",		o_sleep,		1, "<secs>" },
    { "stderr",		o_stderr,		1, "" },
    { "stdout",		o_stdout,		1, "" },
    { "tab-correct",	o_tab_correct,		1, "" },
    { "tab-len",	o_tab_len,		1, "<n>" },
    { "tailbox",	o_tailbox,		2, "<file> <height> <width>" },
    { "tailboxbg",	o_tailboxbg,		2, "<file> <height> <width>" },
    { "textbox",	o_textbox,		2, "<file> <height> <width>" },
    { "title",		o_title,		1, "<title>" },
    { "yesno",		o_yesno,		2, "<text> <height> <width>" },
};
/* *INDENT-ON* */

static eOptions
lookupOption(const char *name, int pass)
{
    unsigned n;

    if (name != 0
	&& !strncmp(name, "--", 2)) {
	name += 2;
	for (n = 0; n < sizeof(options) / sizeof(options[0]); n++) {
	    if ((pass & options[n].pass) != 0
		&& !strcmp(name, options[n].name)) {
		return options[n].code;
	    }
	}
    }
    return o_unknown;
}

static void
Usage(char *msg)
{
    const char *format = "Error: %s.\nUse --help to list options.\n\n";
    char *str = malloc(strlen(msg) + strlen(format) + 1);
    sprintf(str, format, msg);
    exiterr(str);
}

/*
 * Count arguments, stopping at the end of the argument list, or on any of our
 * "--" tokens.
 */
static int
arg_rest(char *argv[])
{
    int i = 1;			/* argv[0] points to a "--" token */

    while (argv[i] != 0
	&& (strncmp(argv[i], "--", 2)
	    || lookupOption(argv[i], 7) == o_unknown))
	i++;
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
	av[1],
	atoi(av[2]),
	atoi(av[3]), defaultno);
}

static int
j_msgbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_msgbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]), 1);
}

static int
j_infobox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_msgbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]), 0);
}

static int
j_textbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_textbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]));
}

static int
j_menu(JUMPARGS)
{
    int ret;
    int tags = howmany_tags(av + 5, 2);
    *offset_add = 5 + tags * 2;
    ret = dialog_menu(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	atoi(av[4]),
	tags, av + 5);
    if (ret >= 0) {
	fprintf(dialog_vars.output, av[5 + ret * 2]);
	return 0;
    } else if (ret == -2)
	return 1;		/* CANCEL */
    return ret;			/* ESC pressed, ret == -1 */
}

static int
j_checklist(JUMPARGS)
{
    int tags = howmany_tags(av + 5, 3);
    *offset_add = 5 + tags * 3;
    return dialog_checklist(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	atoi(av[4]),
	tags, av + 5, FLAG_CHECK, dialog_vars.separate_output);
}

static int
j_radiolist(JUMPARGS)
{
    int tags = howmany_tags(av + 5, 3);
    *offset_add = 5 + tags * 3;
    return dialog_checklist(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	atoi(av[4]),
	tags, av + 5, FLAG_RADIO, dialog_vars.separate_output);
}

static int
j_inputbox(JUMPARGS)
{
    int ret;
    char *init_inputbox = 0;

    if (arg_rest(av) >= 4)
	init_inputbox = av[4];

    *offset_add = arg_rest(av);
    ret = dialog_inputbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	init_inputbox, 0);
    if (ret == 0)
	fprintf(dialog_vars.output, "%s", dialog_input_result);
    return ret;
}

static int
j_passwordbox(JUMPARGS)
{
    int ret;
    char *init_inputbox = 0;

    if (arg_rest(av) >= 4)
	init_inputbox = av[4];

    *offset_add = arg_rest(av);
    ret = dialog_inputbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	init_inputbox, 1);
    if (ret == 0)
	fprintf(dialog_vars.output, "%s", dialog_input_result);
    return ret;
}

#ifdef HAVE_FSELECT
static int
j_fselect(JUMPARGS)
{
    int ret;

    *offset_add = arg_rest(av);
    ret = dialog_fselect(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]));
    if (ret == 0)
	fprintf(dialog_vars.output, "%s", dialog_input_result);
    return ret;
}
#endif

#ifdef HAVE_GAUGE
static int
j_gauge(JUMPARGS)
{
    int percent = 0;

    if (arg_rest(av) >= 4)
	percent = atoi(av[4]);

    *offset_add = arg_rest(av);
    return dialog_gauge(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]),
	percent);
}
#endif

#ifdef HAVE_TAILBOX
static int
j_tailbox(JUMPARGS)
{
    *offset_add = 4;
    return dialog_tailbox(t,
	av[1],
	atoi(av[2]),
	atoi(av[3]));
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
    if (dialog_vars.cant_kill)
	tailbg_nokill_pids[tailbg_nokill_lastpid] =
	    tailbg_pids[tailbg_lastpid] = fork();
    else
	tailbg_pids[tailbg_lastpid] = fork();

    /* warning! here are 2 processes */

    if (tailbg_pids[tailbg_lastpid] == 0) {
	if (dialog_vars.cant_kill)
	    fprintf(dialog_vars.output, "%d", (int) getpid());
	is_tailbg = TRUE;
	dialog_tailboxbg(t,
	    av[1],
	    atoi(av[2]),
	    atoi(av[3]),
	    dialog_vars.cant_kill);

	refresh();		/* quitting for signal 15 and --no-kill */
	endwin();
	exit(0);
    }
    if (dialog_vars.cant_kill)
	tailbg_nokill_lastpid++;
    tailbg_lastpid++;
    return 0;
}
#endif
/* *INDENT-OFF* */
static Mode modes[] =
{
    {o_yesno, 4, 4, j_yesno},
    {o_msgbox, 4, 4, j_msgbox},
    {o_infobox, 4, 4, j_infobox},
    {o_textbox, 4, 4, j_textbox},
    {o_menu, 7, 0, j_menu},
    {o_checklist, 8, 0, j_checklist},
    {o_radiolist, 8, 0, j_radiolist},
    {o_inputbox, 4, 5, j_inputbox},
    {o_passwordbox, 4, 5, j_passwordbox},
#ifdef HAVE_FSELECT
    {o_fselect, 4, 5, j_fselect},
#endif
#ifdef HAVE_GAUGE
    {o_gauge, 4, 5, j_gauge},
#endif
#ifdef HAVE_TAILBOX
    {o_tailbox, 4, 4, j_tailbox},
    {o_tailboxbg, 4, 4, j_tailboxbg},
#endif
};
/* *INDENT-ON* */

static char *
optionString(char **argv, int *num)
{
    int next = *num + 1;
    char *result = argv[next];
    if (result == 0) {
	char temp[80];
	sprintf(temp, "Expected a string-parameter for %.20s", argv[0]);
	Usage(temp);
    }
    *num = next;
    return result;
}

static int
optionValue(char **argv, int *num)
{
    int next = *num + 1;
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
	sprintf(temp, "Expected a numeric-parameter for %.20s", argv[0]);
	Usage(temp);
    }
    *num = next;
    return result;
}

/*
 * Print parts of a message
 */
static void
PrintList(const char **list)
{
    const char *leaf = strrchr(program, '/');
    unsigned n = 0;

    if (leaf != 0)
	leaf++;
    else
	leaf = program;

    while (*list != 0) {
	fprintf(dialog_vars.output, *list++, n ? leaf : VERSION);
	fputc('\n', dialog_vars.output);
	n = 1;
    }
}

static Mode *
lookupMode(eOptions code)
{
    Mode *modePtr = 0;
    unsigned n;
    for (n = 0; n < sizeof(modes) / sizeof(modes[0]); n++) {
	if (modes[n].code == code) {
	    modePtr = &modes[n];
	    break;
	}
    }
    return modePtr;
}

/*
 * Print program help-message
 */
static void
Help(void)
{
    static const char *tbl_1[] =
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
	"  [--create-rc \"Ifile\"]",
#endif
	0
    }, *tbl_3[] =
    {
	"",
	"Auto-size with height and width = 0. Maximize with height and width = -1.",
	"Global-auto-size if also menu_height/list_height = 0.",
	0
    };
    unsigned j, k;

    PrintList(tbl_1);
    fprintf(dialog_vars.output, "Common options:\n ");
    for (j = k = 0; j < sizeof(options) / sizeof(options[0]); j++) {
	if ((options[j].pass & 1)
	    && options[j].help != 0) {
	    unsigned len = 6 + strlen(options[j].name) + strlen(options[j].help);
	    k += len;
	    if (k > 75) {
		fprintf(dialog_vars.output, "\n ");
		k = len;
	    }
	    fprintf(dialog_vars.output, " [--%s%s%s]", options[j].name,
		*(options[j].help) ? " " : "", options[j].help);
	}
    }
    fprintf(dialog_vars.output, "\nBox options:\n");
    for (j = 0; j < sizeof(options) / sizeof(options[0]); j++) {
	if ((options[j].pass & 2)
	    && options[j].help != 0
	    && lookupMode(options[j].code))
	    fprintf(dialog_vars.output, "  --%-12s %s\n", options[j].name,
		options[j].help);
    }
    PrintList(tbl_3);

    exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
    FILE *output = stderr;
    char temp[80];
    const char *separate_str = DEFAULT_SEPARATE_STR;
    int esc_pressed = 0;
    int offset = 1;
    int offset_add;
    int retval = 0;
    int done;
    int j;
    eOptions code;
    Mode *modePtr;
#ifndef HAVE_COLOR
    int use_shadow = FALSE;	/* ignore corresponding option */
#endif

#if defined(ENABLE_NLS)
    /* initialize locale support */
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#elif defined(HAVE_SETLOCALE)
    (void) setlocale(LC_ALL, "");
#endif

    program = argv[0];
    dialog_vars.output = output;

    if (argc == 2) {		/* if we don't want clear screen */
	switch (lookupOption(argv[1], 7)) {
	case o_print_maxsize:
	    initscr();
	    fprintf(output, "MaxSize: %d, %d\n", SLINES, SCOLS);
	    endwin();
	    break;
	case o_print_version:
	    fprintf(output, "Version: %s\n", VERSION);
	    break;
	default:
	case o_help:
	    Help();
	    break;
	}
	return 0;
    }

    if (argc < 2) {
	Help();
    }

    init_dialog();

#ifdef HAVE_RC_FILE
    if (!strcmp(argv[1], "--create-rc")) {
	if (argc != 3) {
	    sprintf(temp, "Expected a filename for %s", argv[1]);
	    Usage(temp);
	}
	end_dialog();
	create_rc(argv[2]);
	return 0;
    }
#endif
    for (j = 1; j < argc; j++) {
	dlg_trim_string(argv[j]);
    }

    if ((lock_refresh = make_lock_filename("/tmp/.lock_fileXXXXXX")) == NULL ||
	(lock_tailbg_refreshed =
	    make_lock_filename("/tmp/.lock_tailbgXXXXXX")) == NULL ||
	(lock_tailbg_exit = make_lock_filename("/tmp/.lock_exitXXXXXX")) == NULL)
	exiterr("Internal error: can't make lock files.");

    while (offset < argc && !esc_pressed) {
	memset(&dialog_vars, 0, sizeof(dialog_vars));
	dialog_vars.aspect_ratio = DEFAULT_ASPECT_RATIO;
	dialog_vars.tab_len = TAB_LEN;
	dialog_vars.output = output;
	done = FALSE;

	while (offset < argc && !done) {	/* Common options */
	    switch (lookupOption(argv[offset], 1)) {
	    case o_title:
		dialog_vars.title = optionString(argv, &offset);
		break;
	    case o_backtitle:
		dialog_vars.backtitle = optionString(argv, &offset);
		break;
	    case o_separate_widget:
		separate_str = optionString(argv, &offset);
		break;
	    case o_separate_output:
		dialog_vars.separate_output = TRUE;
		break;
	    case o_cr_wrap:
		dialog_vars.cr_wrap = TRUE;
		break;
	    case o_no_kill:
		dialog_vars.cant_kill = TRUE;
		break;
	    case o_nocancel:
		dialog_vars.nocancel = TRUE;
		break;
	    case o_size_err:
		dialog_vars.size_err = TRUE;
		break;
	    case o_beep:
		dialog_vars.beep_signal = TRUE;
		break;
	    case o_beep_after:
		dialog_vars.beep_after_signal = TRUE;
		break;
	    case o_shadow:
		use_shadow = TRUE;
		break;
	    case o_defaultno:
		defaultno = TRUE;
		break;
	    case o_default_item:
		dialog_vars.default_item = optionString(argv, &offset);
		break;
	    case o_no_shadow:
		use_shadow = FALSE;
		break;
	    case o_print_size:
		dialog_vars.print_siz = TRUE;
		break;
	    case o_print_maxsize:
		fprintf(output, "MaxSize: %d, %d\n", SLINES, SCOLS);
		break;
	    case o_print_version:
		fprintf(output, "Version: %s\n", VERSION);
		break;
	    case o_tab_correct:
		dialog_vars.tab_correct = TRUE;
		break;
	    case o_sleep:
		dialog_vars.sleep_secs = optionValue(argv, &offset);
		break;
	    case o_stderr:
		dialog_vars.output = output = stderr;
		break;
	    case o_stdout:
		dialog_vars.output = output = stdout;
		break;
	    case o_tab_len:
		dialog_vars.tab_len = optionValue(argv, &offset);
		break;
	    case o_aspect:
		dialog_vars.aspect_ratio = optionValue(argv, &offset);
		break;
	    case o_begin:
		dialog_vars.begin_set = TRUE;
		dialog_vars.begin_y = optionValue(argv, &offset);
		dialog_vars.begin_x = optionValue(argv, &offset);
		break;
	    case o_clear:
		if (argc == 2) {	/* we only want to clear the screen */
		    killall_bg();
		    refresh();
		    end_dialog();
		    return 0;
		}
		dialog_vars.clear_screen = TRUE;
		break;
	    case o_noitem:
	    case o_fullbutton:
		/* ignore */
		break;
	    default:		/* no more common options */
		done = TRUE;
		break;
	    }
	    if (!done)
		offset++;
	}

	if (argv[offset] == NULL) {
	    Usage("Expected a box option");
	}

	if (lookupOption(argv[offset], 2) != o_checklist
	    && dialog_vars.separate_output) {
	    sprintf(temp, "Expected --checklist, not %.20s", argv[offset]);
	    Usage(temp);
	}

	if (dialog_vars.aspect_ratio == 0)
	    dialog_vars.aspect_ratio = DEFAULT_ASPECT_RATIO;

	put_backtitle();

	/* use a table to look for the requested mode, to avoid code duplication */

	modePtr = 0;
	if ((code = lookupOption(argv[offset], 2)) != o_unknown)
	    modePtr = lookupMode(code);
	if (modePtr == 0) {
	    sprintf(temp, "Unknown option %.20s", argv[offset]);
	    Usage(temp);
	}

	if (arg_rest(&argv[offset]) < modePtr->argmin) {
	    sprintf(temp, "Expected at least %d tokens for %.20s, have %d",
		modePtr->argmin, argv[offset], arg_rest(&argv[offset]));
	    Usage(temp);
	}
	if (modePtr->argmax && arg_rest(&argv[offset]) > modePtr->argmax) {
	    sprintf(temp,
		"Expected no more than %d tokens for %.20s, have %d",
		modePtr->argmax, argv[offset], arg_rest(&argv[offset]));
	    Usage(temp);
	}

	retval = (*(modePtr->jumper)) (dialog_vars.title, argv + offset, &offset_add);
	offset += offset_add;

	if (retval == -1) {
	    esc_pressed = 1;
	} else {

	    if (dialog_vars.beep_after_signal)
		beep();

	    if (dialog_vars.sleep_secs)
		napms(dialog_vars.sleep_secs * 1000);

	    if (offset < argc) {
		switch (lookupOption(argv[offset], 7)) {
		case o_and_widget:
		    fputs(separate_str, output);
		    offset++;
		    break;
		case o_unknown:
		    sprintf(temp, "Expected --and-widget, not %.20s",
			argv[offset]);
		    Usage(temp);
		    break;
		default:
		    /* if we got a cancel, etc., stop chaining */
		    if (retval != 0)
			esc_pressed = 2;
		    else
			dialog_vars.clear_screen = TRUE;
		    break;
		}
	    }
	    if (dialog_vars.clear_screen)
		attr_clear(stdscr, LINES, COLS, screen_attr);
	}

    }

    quitall_bg();		/* instead of killall_bg() */
    refresh();
    end_dialog();
    return retval;
}
/* End of main() */
