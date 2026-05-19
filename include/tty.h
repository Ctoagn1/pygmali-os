#include <stdint.h>
#include "keyboardhandler.h"
#include <stddef.h>
#include "fd.h"
void print_input(char newinput);
void del_input();
void clear_input_buffer();
void stdout_parse();
void parse_esc_sequence(char* seq);
void handle_sequence(char command, int params[4], int param_count);
void keyevent_translate();
void flush_buffer();
int stdin_read(File* f, void* buf, int n);
int stdout_write(File* f, const void* buf, int n);
void tty_receive_char(char c);
void tty_init();