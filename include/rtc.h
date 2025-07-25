#define CMOS_REGISTER_SELECT_PORT 0x70
#define CMOS_READ_PORT 0x71
#define NMI_DISABLE 0b10000000 //1 to disable NMIs
#define SECONDS_REGISTER 0x00
#define MINUTES_REGISTER 0x02
#define HOURS_REGISTER 0x04
#define WEEKDAY_REGISTER 0x06 //ranges from 1-7, sunday being 1. Unreliable, apparently.
#define DAY_OF_MONTH_REGISTER 0x07
#define MONTH_REGISTER 0x08
#define YEAR_REGISTER 0x09
#define CENTURY_REGISTER 0x32 //probably won't use it but good to know
#define STATUS_REGISTER_A 0x0A
#define STATUS_REGISTER_B 0x0B //on some cmos chips, B can't be modified. Bit 1 (value 2)- if 1, 24 hrs, 0, 12 hrs. Bit 2- if 1, binary, 0, BCD. 
//Note- if 12 hr time is enabled, the 128 bit (2nd to last) on the hour byte is 1 for PM, 0 for AM
#define STATUS_REGISTER_C 0x0C
extern uint8_t second;
extern uint8_t minute;
extern uint8_t hour;
extern uint8_t day;
extern uint8_t month;
extern uint8_t year;
extern uint64_t timer;
extern _Bool is_24_hrs;
extern _Bool is_BCD; 
#include <stdint.h>
void read_startup_time();
void update_time_wrapper();
void display_time();