/*
 *  $Id: rc.c,v 1.21 2004/09/21 02:14:58 tom Exp $
 *
 *  rc.c -- routines for processing the configuration file
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
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

#ifdef HAVE_COLOR
#include <dlg_colors.h>
/*
 * For matching color names with color values
 */
static const color_names_st color_names[] =
{
    {"BLACK", COLOR_BLACK},
    {"RED", COLOR_RED},
    {"GREEN", COLOR_GREEN},
    {"YELLOW", COLOR_YELLOW},
    {"BLUE", COLOR_BLUE},
    {"MAGENTA", COLOR_MAGENTA},
    {"CYAN", COLOR_CYAN},
    {"WHITE", COLOR_WHITE},
};				/* color names */

#define GLOBALRC "/etc/dialogrc"
#define DIALOGRC ".dialogrc"

/* Types of values */
#define VAL_INT  0
#define VAL_STR  1
#define VAL_BOOL 2

/* Type of line in configuration file */
#define LINE_BLANK    2
#define LINE_COMMENT  1
#define LINE_OK       0
#define LINE_ERROR   -1

/* number of configuration variables */
#define VAR_COUNT        (sizeof(vars) / sizeof(vars_st))

/* check if character is white space */
#define whitespace(c)    (c == ' ' || c == '\t')

/* check if character is string quoting characters */
#define isquote(c)       (c == '"' || c == '\'')

/* get last character of string */
#define lastch(str)      str[strlen(str)-1]

/*
 * Configuration variables
 */
typedef struct {
    const char *name;		/* name of configuration variable as in DIALOGRC */
    void *var;			/* address of actual variable to change */
    int type;			/* type of value */
    const char *comment;	/* comment to put in "rc" file */
} vars_st;

static const vars_st vars[] =
{
    {"use_shadow",
     &dialog_state.use_shadow,
     VAL_BOOL,
     "Shadow dialog boxes? This also turns on color."},

    {"use_colors",
     &dialog_state.use_colors,
     VAL_BOOL,
     "Turn color support ON or OFF"},

};				/* vars */

/*
 * Convert an attribute to a string representation like this:
 *
 * "(foreground,background,highlight)"
 */
static char *
attr_to_str(char *str, int fg, int bg, int hl)
{
    int i;

    strcpy(str, "(");
    /* foreground */
    for (i = 0; fg != color_names[i].value; i++) ;
    strcat(str, color_names[i].name);
    strcat(str, ",");

    /* background */
    for (i = 0; bg != color_names[i].value; i++) ;
    strcat(str, color_names[i].name);

    /* highlight */
    strcat(str, hl ? ",ON)" : ",OFF)");

    return str;
}

/*
 * Extract the foreground, background and highlight values from an attribute
 * represented as a string in this form:
 *
 * "(foreground,background,highlight)"
 */
static int
str_to_attr(char *str, int *fg, int *bg, int *hl)
{
    int i = 0, j, get_fg = 1;
    char tempstr[MAX_LEN + 1], *part;

    if (str[0] != '(' || lastch(str) != ')')
	return -1;		/* invalid representation */

    /* remove the parenthesis */
    strcpy(tempstr, str + 1);
    lastch(tempstr) = '\0';

    /* get foreground and background */

    while (1) {
	/* skip white space before fg/bg string */
	while (whitespace(tempstr[i]) && tempstr[i] != '\0')
	    i++;
	if (tempstr[i] == '\0')
	    return -1;		/* invalid representation */
	part = tempstr + i;	/* set 'part' to start of fg/bg string */

	/* find end of fg/bg string */
	while (!whitespace(tempstr[i]) && tempstr[i] != ','
	       && tempstr[i] != '\0')
	    i++;

	if (tempstr[i] == '\0')
	    return -1;		/* invalid representation */
	else if (whitespace(tempstr[i])) {	/* not yet ',' */
	    tempstr[i++] = '\0';

	    /* skip white space before ',' */
	    while (whitespace(tempstr[i]) && tempstr[i] != '\0')
		i++;

	    if (tempstr[i] != ',')
		return -1;	/* invalid representation */
	}
	tempstr[i++] = '\0';	/* skip the ',' */
	for (j = 0; j < COLOR_COUNT && dlg_strcmp(part, color_names[j].name);
	     j++) ;
	if (j == COLOR_COUNT)	/* invalid color name */
	    return -1;
	if (get_fg) {
	    *fg = color_names[j].value;
	    get_fg = 0;		/* next we have to get the background */
	} else {
	    *bg = color_names[j].value;
	    break;
	}
    }				/* got foreground and background */

    /* get highlight */

    /* skip white space before highlight string */
    while (whitespace(tempstr[i]) && tempstr[i] != '\0')
	i++;
    if (tempstr[i] == '\0')
	return -1;		/* invalid representation */
    part = tempstr + i;		/* set 'part' to start of highlight string */

    /* trim trailing white space from highlight string */
    i = strlen(part) - 1;
    while (whitespace(part[i]))
	i--;
    part[i + 1] = '\0';

    if (!dlg_strcmp(part, "ON"))
	*hl = TRUE;
    else if (!dlg_strcmp(part, "OFF"))
	*hl = FALSE;
    else
	return -1;		/* invalid highlight value */

    return 0;
}

/*
 * Parse a line in the configuration file
 *
 * Each line is of the form:  "variable = value". On exit, 'var' will contain
 * the variable name, and 'value' will contain the value string.
 *
 * Return values:
 *
 * LINE_BLANK   - line is blank
 * LINE_COMMENT - line is comment
 * LINE_OK      - line is ok
 * LINE_ERROR   - syntax error in line
 */
static int
parse_line(char *line, char **var, char **value)
{
    int i = 0;

    /* ignore white space at beginning of line */
    while (whitespace(line[i]) && line[i] != '\0')
	i++;

    if (line[i] == '\0')	/* line is blank */
	return LINE_BLANK;
    else if (line[i] == '#')	/* line is comment */
	return LINE_COMMENT;
    else if (line[i] == '=')	/* variables names can't start with a '=' */
	return LINE_ERROR;

    /* set 'var' to variable name */
    *var = line + i++;		/* skip to next character */

    /* find end of variable name */
    while (!whitespace(line[i]) && line[i] != '=' && line[i] != '\0')
	i++;

    if (line[i] == '\0')	/* syntax error */
	return LINE_ERROR;
    else if (line[i] == '=')
	line[i++] = '\0';
    else {
	line[i++] = '\0';

	/* skip white space before '=' */
	while (whitespace(line[i]) && line[i] != '\0')
	    i++;

	if (line[i] != '=')	/* syntax error */
	    return LINE_ERROR;
	else
	    i++;		/* skip the '=' */
    }

    /* skip white space after '=' */
    while (whitespace(line[i]) && line[i] != '\0')
	i++;

    if (line[i] == '\0')
	return LINE_ERROR;
    else
	*value = line + i;	/* set 'value' to value string */

    /* trim trailing white space from 'value' */
    i = strlen(*value) - 1;
    while (whitespace((*value)[i]))
	i--;
    (*value)[i + 1] = '\0';

    return LINE_OK;		/* no syntax error in line */
}
#endif

/*
 * Create the configuration file
 */
void
dlg_create_rc(const char *filename)
{
#ifdef HAVE_COLOR
    char buffer[MAX_LEN + 1];
    unsigned i, limit;
    FILE *rc_file;

    if ((rc_file = fopen(filename, "wt")) == NULL)
	dlg_exiterr("Error opening file for writing in dlg_create_rc().");

    fprintf(rc_file, "#\
\n# Run-time configuration file for dialog\
\n#\
\n# Automatically generated by \"dialog --create-rc <file>\"\
\n#\
\n#\
\n# Types of values:\
\n#\
\n# Number     -  <number>\
\n# String     -  \"string\"\
\n# Boolean    -  <ON|OFF>\
\n# Attribute  -  (foreground,background,highlight?)\
\n#\n\n");

    /* Print an entry for each configuration variable */
    for (i = 0; i < VAR_COUNT; i++) {
	fprintf(rc_file, "\n# %s\n", vars[i].comment);
	switch (vars[i].type) {
	case VAL_INT:
	    fprintf(rc_file, "%s = %d\n", vars[i].name,
		    *((int *) vars[i].var));
	    break;
	case VAL_STR:
	    fprintf(rc_file, "%s = \"%s\"\n", vars[i].name,
		    (char *) vars[i].var);
	    break;
	case VAL_BOOL:
	    fprintf(rc_file, "%s = %s\n", vars[i].name,
		    *((bool *) vars[i].var) ? "ON" : "OFF");
	    break;
	}
    }
    limit = dlg_color_count();
    for (i = 0; i < limit; ++i) {
	fprintf(rc_file, "\n# %s\n", dlg_color_table[i].comment);
	fprintf(rc_file, "%s = %s\n", dlg_color_table[i].name,
		attr_to_str(buffer,
			    dlg_color_table[i].fg,
			    dlg_color_table[i].bg,
			    dlg_color_table[i].hilite));
    }

    (void) fclose(rc_file);
#endif
}

#ifdef HAVE_COLOR
static int
find_vars(char *name)
{
    int result = -1;
    unsigned i;

    for (i = 0; i < VAR_COUNT; i++) {
	if (dlg_strcmp(vars[i].name, name) == 0) {
	    result = i;
	    break;
	}
    }
    return result;
}

static int
find_color(char *name)
{
    int result = -1;
    int i;
    int limit = dlg_color_count();

    for (i = 0; i < limit; i++) {
	if (dlg_strcmp(dlg_color_table[i].name, name) == 0) {
	    result = i;
	    break;
	}
    }
    return result;
}
#endif

/*
 * Parse the configuration file and set up variables
 */
int
dlg_parse_rc(void)
{
#ifdef HAVE_COLOR
    int i;
    int l = 1, parse, fg, bg, hl;
    char str[MAX_LEN + 1], *var, *value, *tempptr;
    FILE *rc_file = 0;

    /*
     *  At start, 'dialog' determines the settings to use as follows:
     *
     *  a) if environment variable DIALOGRC is set, it's value determines the
     *     name of the configuration file.
     *
     *  b) if the file in (a) can't be found, use the file $HOME/.dialogrc
     *     as the configuration file.
     *
     *  c) if the file in (b) can't be found, try using the GLOBALRC file.
     *     Usually this will be /etc/dialogrc.
     *
     *  d) if the file in (c) can't be found, use compiled in defaults.
     *
     */

    /* try step (a) */
    if ((tempptr = getenv("DIALOGRC")) != NULL)
	rc_file = fopen(tempptr, "rt");

    if (tempptr == NULL || rc_file == NULL) {	/* step (a) failed? */
	/* try step (b) */
	if ((tempptr = getenv("HOME")) != NULL
	    && strlen(tempptr) < MAX_LEN - 20) {
	    if (tempptr[0] == '\0' || lastch(tempptr) == '/')
		sprintf(str, "%s%s", tempptr, DIALOGRC);
	    else
		sprintf(str, "%s/%s", tempptr, DIALOGRC);
	    rc_file = fopen(str, "rt");
	}
    }

    if (tempptr == NULL || rc_file == NULL) {	/* step (b) failed? */
	/* try step (c) */
	sprintf(str, "%s", GLOBALRC);
	if ((rc_file = fopen(str, "rt")) == NULL)
	    return 0;		/* step (c) failed, use default values */
    }

    /* Scan each line and set variables */
    while (fgets(str, MAX_LEN, rc_file) != NULL) {
	if (lastch(str) != '\n') {
	    /* ignore rest of file if line too long */
	    fprintf(stderr, "\nParse error: line %d of configuration"
		    " file too long.\n", l);
	    (void) fclose(rc_file);
	    return -1;		/* parse aborted */
	} else {
	    lastch(str) = '\0';
	    parse = parse_line(str, &var, &value);	/* parse current line */

	    switch (parse) {
	    case LINE_BLANK:	/* ignore blank lines and comments */
	    case LINE_COMMENT:
		break;
	    case LINE_OK:
		/* search table for matching config variable name */
		if ((i = find_vars(var)) >= 0) {
		    switch (vars[i].type) {
		    case VAL_INT:
			*((int *) vars[i].var) = atoi(value);
			break;
		    case VAL_STR:
			if (!isquote(value[0]) || !isquote(lastch(value))
			    || strlen(value) < 2) {
			    fprintf(stderr, "\nParse error: string value "
				    "expected at line %d of configuration "
				    "file.\n", l);
			    return -1;	/* parse aborted */
			} else {
			    /* remove the (") quotes */
			    value++;
			    lastch(value) = '\0';
			    strcpy((char *) vars[i].var, value);
			}
			break;
		    case VAL_BOOL:
			if (!dlg_strcmp(value, "ON"))
			    *((bool *) vars[i].var) = TRUE;
			else if (!dlg_strcmp(value, "OFF"))
			    *((bool *) vars[i].var) = FALSE;
			else {
			    fprintf(stderr, "\nParse error: boolean value "
				    "expected at line %d of configuration "
				    "file (found %s).\n", l, value);
			    return -1;	/* parse aborted */
			}
			break;
		    }
		} else if ((i = find_color(var)) >= 0) {
		    if (str_to_attr(value, &fg, &bg, &hl) == -1) {
			fprintf(stderr, "\nParse error: attribute "
				"value expected at line %d of configuration "
				"file.\n", l);
			return -1;	/* parse aborted */
		    }
		    dlg_color_table[i].fg = fg;
		    dlg_color_table[i].bg = bg;
		    dlg_color_table[i].hilite = hl;
		} else {
		    fprintf(stderr, "\nParse error: unknown variable "
			    "at line %d of configuration file:\n\t%s\n", l, var);
		    return -1;	/* parse aborted */
		}
		break;
	    case LINE_ERROR:
		fprintf(stderr, "\nParse error: syntax error at line %d of "
			"configuration file.\n", l);
		return -1;	/* parse aborted */
	    }
	}
	l++;			/* next line */
    }

    (void) fclose(rc_file);
#endif
    return 0;			/* parse successful */
}
