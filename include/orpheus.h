int play_song(char* filename);
int note_dur(int bpm, char* type);
char** tokenize_song(char* contents);
void free_tokens(char** tokens);