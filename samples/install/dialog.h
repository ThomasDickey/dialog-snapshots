/* This should be extended and then integrated back into the normal
   dialog source code. At the moment, I cannot use header files from
   the current dialog source to get prototypes for the library. */

void init_dialog (void);
void dialog_clear (void);
void end_dialog (void);

extern int dialog_result;

int dialog_msgbox(const char *title, const char *prompt, int height,
		int width, int pause);
int dialog_menu(const char *title, const char *prompt, int height,
		int width, int menu_height, int item_no,
		const char * const * items);
int dialog_textbox(const char *title, const char *file, int height, int width);
extern unsigned char dialog_input_result[];
int dialog_inputbox(const char *title, const char *prompt, int height,
		int width, const char *init);

