
#include "io.h"
#include "pic.h"
#include "rtc.h"
#include "tty.h"
#include "printf.h"
/*Status Register A- bit 0-3 sets divider output frequency, default is 0110 for 1024 Hz. 
bit 4 is bank control, sets CMOS ram bank that rtc acesses. bits 5-6 determine time-base frequency,
whatever that is. bit 7 signifies update in progress
B- bit 0- daylight-savings-time, bit 1- 24/12 hr, bit 2, binary/bcd, bit 3, square wave generator,
bit 4, update interrupt enable, bit 5, alarm interrupt enable (interrupt when date matches set value), bit 6
period interrupt enable, bit 7 disables updates when set to 1 (for changing time)
C-0-3 reserved, 4,5,6- update, alarm, periodic interrupt occurred. bit 7- general interrupt
D- 0-6 reserved, 7 is read only that is 1 when cmos ram is valid. Note- C MUST BE READ for an interrupt to repeat.*/
uint8_t second;
uint8_t minute;
uint8_t hour;
uint8_t day;
uint8_t month;
uint8_t year;
uint64_t timer;
_Bool is_24_hrs;
_Bool is_BCD; 
char *word_months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
void check_for_updates(){
    io_wait();
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_A | NMI_DISABLE); //7th bit of A is 1 if update in progress, prevents stuff like 8:60 instead of 9
    io_wait();
    while((inb(CMOS_READ_PORT) & 0b10000000) != 0) io_wait();
}
void check_status(){
    io_wait();
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_B | NMI_DISABLE);
    io_wait();
    uint8_t statusB = inb(CMOS_READ_PORT);
    if( (statusB & 0b00000010) != 0) is_24_hrs = 1;
    else is_24_hrs = 0;

    if((statusB & 0b00000100) != 0) is_BCD = 0;
    else is_BCD = 1;
}
void enable_rtc_interrupts(){
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_B | NMI_DISABLE);
    io_wait();
    uint8_t reg = inb(CMOS_READ_PORT);
    io_wait();
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_B | NMI_DISABLE);
    io_wait();
    outb(CMOS_READ_PORT, reg | 0b00010000); //bit 16 enables update interrupts
    io_wait();
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_B); //reenable nmis

}
uint8_t bcd_to_binary(uint8_t val){
    return (val & 0x0F) + ((val >> 4) * 10);
}
void read_startup_time(){ //bit 6 of status register b enables interrupts
    enable_rtc_interrupts();
    check_status();
    check_for_updates();
    outb(CMOS_REGISTER_SELECT_PORT, SECONDS_REGISTER | NMI_DISABLE);
    io_wait();
    second = inb(CMOS_READ_PORT);

    outb(CMOS_REGISTER_SELECT_PORT, MINUTES_REGISTER| NMI_DISABLE);
    io_wait();
    minute = inb(CMOS_READ_PORT);

    outb(CMOS_REGISTER_SELECT_PORT, HOURS_REGISTER| NMI_DISABLE);
    io_wait();
    hour = inb(CMOS_READ_PORT);

    outb(CMOS_REGISTER_SELECT_PORT, DAY_OF_MONTH_REGISTER| NMI_DISABLE);
    io_wait();
    day = inb(CMOS_READ_PORT);

    outb(CMOS_REGISTER_SELECT_PORT, MONTH_REGISTER| NMI_DISABLE);
    io_wait();
    month = inb(CMOS_READ_PORT);

    outb(CMOS_REGISTER_SELECT_PORT, YEAR_REGISTER| NMI_DISABLE);
    io_wait();
    year = inb(CMOS_READ_PORT);
    if(is_BCD){
        second=bcd_to_binary(second);
        minute=bcd_to_binary(minute);
        if((hour & 0b10000000)!=0 && !is_24_hrs){ //128 bit is on if PM
            hour &= 0b01111111;
            hour=bcd_to_binary(hour);
            if(hour != 12) hour+= 12;
        }
        else hour=bcd_to_binary(hour);
        day=bcd_to_binary(day);
        month=bcd_to_binary(month);
        year=bcd_to_binary(year);
    }
    else{
        if((hour & 0b10000000)!=0 && !is_24_hrs){
            hour &= 0b01111111;
            if(hour != 12) hour+= 12;
        }
    }
    outb(CMOS_REGISTER_SELECT_PORT, YEAR_REGISTER); //reenables non maskable interrupts
}
void update_time(){
    timer++;
    second++;
    if(second >= 60){
        second=0;
        minute++;
    }
    if(minute >= 60){
        minute=0;
        hour++;
    }
    if(hour >= 24){
        hour=0;
        day++;
    }
    if(month==2 && ((day==29 && year%4==0) || (day==28 && year%4!=0))){
        day=1;
        month++;
    }
    if(day>31 && (month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12)){
        day=1;
        month++;
    }
    if(day>30 && (month==4 || month==6 || month==9 || month==11)){
        day=1;
        month++;
    }
    if(month>12){
        month=1;
        year++;
    }
    // Register C tracks whether an interrupt occurred, and it must be read for it to occur again
    outb(CMOS_REGISTER_SELECT_PORT, STATUS_REGISTER_C);
    io_wait();
    inb(CMOS_READ_PORT);
    PIC_sendEOI(8);
}
void display_time(){
    char date_and_time[64]={0};
    snprintf(date_and_time, sizeof(date_and_time), "%s %d, 20%02d %02d:%02d:%02d\n", word_months[month-1], day, year, hour, minute, second);
    terminal_writestring(date_and_time);
}
