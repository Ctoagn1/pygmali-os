#include "keyboardhandler.h"
typedef struct{
	int* line_starts;
	int count;
} Text_Lines;
int init_editor(char* filename);
void editor_write(char c);
void editor_backspace();
void editor_parse(KeyEvent key);
Text_Lines assign_line_starts();
int update_to_screen();
