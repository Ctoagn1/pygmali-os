#include "fatparser.h"
#include "string.h"
#include "pit.h"
#include "orpheus.h"
int play_song(char* filename){
    int bpm=1;
    char* contents = file_contents(filename);
    if(!contents) return -1;
    char** tokens = tokenize_song(contents);
    kfree(contents);
    int len=0;
    while(tokens[len]) len++;
    if((len%2)!=0 || len==0){
        free_tokens(tokens);
        return -1;
    }
    if(strcmp(tokens[0], "BPM")!=0){
        free_tokens(tokens);
        return -1;
    }
    int bpm_val=str_to_int(tokens[1]);
    if(bpm<=0){
        free_tokens(tokens);
        return -1;
    }
    bpm=60000/bpm_val;
    for(int i=0; i<(len-1)/2; i++){
        int frequency=0;
        int duration=0;
        _Bool is_rest=0;
        for(int j=0; j<sizeof(note_table)/sizeof(Note); j++){
            if(strcmp(tokens[2+(2*i)], note_table[j].name)==0){
                frequency = note_table[j].frequency;
            }
        }
        if(strcmp(tokens[2+(2*i)], "REST")==0) is_rest=1;
        duration=note_dur(bpm, tokens[3+(2*i)]);
        if(duration==-1 || (frequency==0 && is_rest==0)){
            free_tokens(tokens);
            return -1;
        }
        if(is_rest) msleep(duration);
        else play_sound(frequency, duration);  
    }
    free_tokens(tokens);
    return 0;
}
int note_dur(int bpm, char* type){
    if(strcmp(type, "W")==0) return bpm*4;
    if(strcmp(type, "H")==0) return bpm*2;
    if(strcmp(type, "Q")==0) return bpm;
    if(strcmp(type, "E")==0) return bpm/2;
    if(strcmp(type, "S")==0) return bpm/4;
    return -1;
}
char** tokenize_song(char* contents){
    int pos=0;
    int word_count=0;
    while(contents[pos]!='\0'){
        while((contents[pos]==' ' || contents[pos]=='\t' || contents[pos]=='\n') && contents[pos]!='\0'){
            pos++;
        }
        if(contents[pos]!='\0') word_count++;
        while((contents[pos]!=' ' && contents[pos]!='\t' && contents[pos]!='\n') && contents[pos]!='\0'){
            pos++;
        }
    }
    char** word_array = kmalloc(sizeof(char*)*(word_count+1));
    if(!word_array) return NULL;
    word_array[word_count]=NULL;
    pos=0;
    word_count=0;
    while(contents[pos]!='\0'){
        int word_len=0;
        while((contents[pos]==' ' || contents[pos]=='\t' || contents[pos]=='\n') && contents[pos]!='\0'){
            pos++;
        }
        while((contents[pos]!=' ' && contents[pos]!='\t' && contents[pos]!='\n') && contents[pos]!='\0'){
            word_len++;
            pos++;
        }
        if(word_len!=0){
            char* arg = kmalloc(word_len+1);
            if(!arg){
                for(int i=0; i<word_count; i++) kfree(word_array[i]);
                kfree(word_array);
                return NULL;
            }
            memcpy(arg, &contents[pos-word_len], word_len);
            arg[word_len]='\0';
            word_array[word_count]=arg;
            word_count++;
        }
    }
    return word_array;
}
void free_tokens(char** tokens){
    int i=0;
    while(tokens[i]){
        kfree(tokens[i]);
        i++;
    }
    kfree(tokens);
}
