#include "tty.h"
#include "string.h"
#include "writingmode.h"
#include "kmalloc.h"
#include "keyboardhandler.h"
#include "fatparser.h"
#include "printf.h"
#include "galatea.h"
#define extra_text_size 16
uint64_t char_len=0;
char* text_buffer=NULL;
uint64_t actual_text_length=0;
uint64_t text_buffer_pos;
uint64_t text_buffer_size;
int startline;
char* outputfile = NULL;
uint16_t* old_screen;
char* extra_info = NULL;
int init_editor(char* filename){
	char* fatname = plaintext_to_filename(filename);
	if(!fatname) return -1;
	kfree(fatname);
	old_screen = kmalloc(VGA_WIDTH*VGA_HEIGHT*2);
	memcpy(old_screen, terminal_buffer, VGA_WIDTH*VGA_HEIGHT*2);
	terminal_initialize();
	mode=2;
	terminal_row=VGA_HEIGHT-2;
	terminal_column=0;
	for(int i=0; i<VGA_WIDTH; i++){
		terminal_putchar('-');
	}
	terminal_writestring("~galatea~        ctrl+w: write ^q:quit ^s write+quit        ");
	terminal_row=0;
	char_len=0;
	text_buffer_pos=0;
	text_buffer_size=0;
	terminal_column=0;
	startline=0;
	outputfile=strdup(filename);
	char* filetext = file_contents(filename);
	if(filetext){
		text_buffer_size=strlen(filetext)+1;
		text_buffer=krealloc(text_buffer, text_buffer_size);
		memcpy(text_buffer, filetext, text_buffer_size);
		kfree(filetext);
	}
	if(!filetext){
		text_buffer_size=(512*sectors_per_cluster); 
		text_buffer=krealloc(text_buffer, text_buffer_size);
		 memset(text_buffer, 0, text_buffer_size);
	}
	char_len=strlen(text_buffer);
	extra_info=kmalloc(extra_text_size);
	memset(extra_info, 0, extra_text_size);
	return 0;
}
int write_editor_buffer(){
	if(!outputfile) return -1;
	create_file(outputfile, FILE);
	write_to_file(text_buffer, char_len, outputfile);
	strcpy(extra_info, "saved");
	update_to_screen();
	return 0;

}
void exit_editor(){
	mode=1;
	memmove(terminal_buffer, old_screen, VGA_WIDTH*VGA_HEIGHT*2);
	kfree(old_screen);
	kfree(outputfile);
	kfree(text_buffer);
	kfree(extra_info);
	terminal_shell_set();
}
int get_cursor_line() {
    Text_Lines lines = assign_line_starts();
    int cursor_line = 0;
    for (int i=0; i<lines.count; i++) {
        if (lines.line_starts[i] > text_buffer_pos) break;
        cursor_line = i;
    }
    kfree(lines.line_starts);
    return cursor_line;
}
void editor_write(char c){
	if(char_len+1>=text_buffer_size){
		int oldsize=text_buffer_size;
		text_buffer_size+=(512*sectors_per_cluster); //allocate by cluster
		text_buffer=krealloc(text_buffer, text_buffer_size);
		memset(&text_buffer[oldsize], 0, text_buffer_size-oldsize);
	}
	if(text_buffer_pos>char_len) text_buffer_pos=char_len;
	memmove(&text_buffer[text_buffer_pos + 1], &text_buffer[text_buffer_pos], char_len - text_buffer_pos+1);
	text_buffer[text_buffer_pos]=c;
	char_len++;
	text_buffer_pos++;
	text_buffer[char_len]='\0';
}
void editor_backspace(){
	if(char_len==0 || text_buffer_pos==0){
		return;
	}
	memmove(&text_buffer[text_buffer_pos-1], &text_buffer[text_buffer_pos], char_len-text_buffer_pos+1);
	char_len--;
	text_buffer_pos--;
	text_buffer[char_len]='\0';
}
void editor_parse(KeyEvent key){
	if(key.ascii=='q' && key.ctrl==1){
		exit_editor();
		return;
	}
	if(key.ascii=='w' && key.ctrl==1){
		write_editor_buffer();
		return;
	}
	if(key.ascii=='s' && key.ctrl==1){
		write_editor_buffer();
		exit_editor();
		return;
	}
	if(key.scancode==BACKSPACE_KEY){
		editor_backspace();
		update_to_screen();
		return;
	}
	if(key.scancode==CURSOR_UP && key.special==1){
		Text_Lines lines = assign_line_starts();
		int count = 0;
		int last_start = -1;
		while (count < lines.count) {
			if (lines.line_starts[count] >= text_buffer_pos) break;
			last_start = lines.line_starts[count];
			count++;
		}
		kfree(lines.line_starts);
		if (last_start == -1) return;
		text_buffer_pos = last_start;
		update_to_screen();
	}
	if(key.scancode==CURSOR_DOWN && key.special==1){
		Text_Lines lines = assign_line_starts();
		for (int i = 0; i < lines.count; i++) {
			if (lines.line_starts[i] > text_buffer_pos) {
				text_buffer_pos = lines.line_starts[i];
				kfree(lines.line_starts);
				update_to_screen();
				return;
			}
		}
	}
	if(key.scancode==CURSOR_LEFT && key.special==1){
		if(text_buffer_pos>0) text_buffer_pos--;
		update_to_screen();
	}
	if(key.scancode==CURSOR_RIGHT && key.special==1){
		if(text_buffer_pos<char_len) text_buffer_pos++;
		update_to_screen();
	}
	if(key.ascii != '\0'){
		editor_write(key.ascii);
		update_to_screen();
	}
}
Text_Lines assign_line_starts(){
	int line_len=0;
	int lines=1;
	Text_Lines text;
	text.line_starts = kmalloc(lines*sizeof(int));
	text.line_starts[0]=0;
	for(int i=0; i<char_len; i++){
		if(text_buffer[i]=='\t'){
			line_len+=4-line_len%4;
		}
		if(line_len>=VGA_WIDTH-1){
			lines++;
			text.line_starts=krealloc(text.line_starts, lines*sizeof(int));
			text.line_starts[lines-1]=i+1;
			line_len=0;
			continue;
		}
		if(text_buffer[i]=='\n'){
			line_len=0;
			lines++;
			text.line_starts=krealloc(text.line_starts, lines*sizeof(int));
			text.line_starts[lines-1]=i+1;
			continue;
		}
		line_len++;
	}
	text.count=lines;
	return text;
}
int update_to_screen(){
	int cursorpos = get_cursor_line();
	if(cursorpos<startline) startline=cursorpos;
	if(cursorpos>=startline+(VGA_HEIGHT-2)) startline=cursorpos-(VGA_HEIGHT-3);
	int text_length=0;
	int row=0;
	int col=0;
	int buffer_length=0;
	Text_Lines text = assign_line_starts();
	if(startline>text.count-1){
		kfree(text.line_starts);
		return -1;
	}
	int startchar = text.line_starts[startline];
	int screensize = (VGA_WIDTH*(VGA_HEIGHT-2)); //saving 2 rows for instructions
	char* screenbuffer = kmalloc(screensize);
	if(!screenbuffer){
		kfree(text.line_starts);
		return -1;
	}
	memset(screenbuffer, 0, screensize);
	int final_col=-1;
	int final_row=-1;
	while(text_length<screensize){
		if(startchar+buffer_length==text_buffer_pos){
			final_col=col;
			final_row=row;
		}

		if(startchar+buffer_length>= char_len) break;
		if(text_buffer[startchar+buffer_length]=='\n'){
			int len = VGA_WIDTH-text_length%VGA_WIDTH;
			col=0;
			row++;
			for(int i=0; i<len; i++){
				if (text_length >= screensize) break;
				screenbuffer[text_length]=' ';
				text_length++;
			}
			buffer_length++;
		}
		if(text_buffer[startchar+buffer_length]=='\t'){
			int len = 4-(text_length%VGA_WIDTH)%4;
			col+=len;
			for(int i=0; i<len; i++){
				if (text_length >= screensize) break;
				screenbuffer[text_length]=' ';
				text_length++;
			}
			buffer_length++;
		}
		if(text_buffer[startchar+buffer_length]!='\n' && text_buffer[startchar+buffer_length]!='\t'){
			screenbuffer[text_length]=text_buffer[buffer_length+startchar];
			text_length++;
			buffer_length++;
			col++;
		}
		if(col>=VGA_WIDTH){
			col=0;
			row++;
		}
	}
	terminal_column=0;
	terminal_row=0;
	for(int i=0; i<screensize; i++){
		terminal_putchar(screenbuffer[i]);
	}
	terminal_row=VGA_HEIGHT-2;
	terminal_column=0;
	for(int i=0; i<VGA_WIDTH; i++){
		terminal_putchar('-');
	}
	terminal_writestring("~galatea~        ctrl+w:write ctrl+q:quit ctrl+s:write+quit        ");
	terminal_writestring(extra_info);
	printf(" %d        ", char_len);
	memset(extra_info, 0, extra_text_size);
	update_cursor(final_col, final_row);
	kfree(screenbuffer);
	kfree(text.line_starts);
	return 0;
}
