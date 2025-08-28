#ifndef ESP32TIME_H
#define ESP32TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    long offset;
} ESP32Time;

// Constructor functions
ESP32Time* ESP32Time_create();
ESP32Time* ESP32Time_create_with_offset(long offset);
void ESP32Time_destroy(ESP32Time* rtc);

// Time setting functions
void ESP32Time_setTime(ESP32Time* rtc, int sc, int mn, int hr, int dy, int mt, int yr, int ms);
void ESP32Time_setTimeStruct(ESP32Time* rtc, struct tm t);
void ESP32Time_setTimeEpoch(ESP32Time* rtc, unsigned long epoch, int ms);

// Time getting functions
struct tm ESP32Time_getTimeStruct(ESP32Time* rtc);
char* ESP32Time_getDateTime(ESP32Time* rtc, bool mode);
char* ESP32Time_getTimeDate(ESP32Time* rtc, bool mode);
char* ESP32Time_getTime(ESP32Time* rtc);
char* ESP32Time_getTimeFormat(ESP32Time* rtc, const char* format);
char* ESP32Time_getDate(ESP32Time* rtc, bool mode);

// Raw time functions
unsigned long ESP32Time_getMillis(ESP32Time* rtc);
unsigned long ESP32Time_getMicros(ESP32Time* rtc);
unsigned long ESP32Time_getEpoch(ESP32Time* rtc);
unsigned long ESP32Time_getLocalEpoch(ESP32Time* rtc);

// Time component functions
int ESP32Time_getSecond(ESP32Time* rtc);
int ESP32Time_getMinute(ESP32Time* rtc);
int ESP32Time_getHour(ESP32Time* rtc, bool mode);
char* ESP32Time_getAmPm(ESP32Time* rtc, bool lowercase);
int ESP32Time_getDay(ESP32Time* rtc);
int ESP32Time_getDayofWeek(ESP32Time* rtc);
int ESP32Time_getDayofYear(ESP32Time* rtc);
int ESP32Time_getMonth(ESP32Time* rtc);
int ESP32Time_getYear(ESP32Time* rtc);

// Memory management for returned strings
void ESP32Time_free_string(char* str);

#ifdef __cplusplus
}
#endif

#endif