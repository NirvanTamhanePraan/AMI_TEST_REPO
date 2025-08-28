#include "ESP32Time.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Static variable for overflow tracking
#ifdef RTC_DATA_ATTR
RTC_DATA_ATTR static bool overflow;
#else
static bool overflow;
#endif

ESP32Time* ESP32Time_create() {
    ESP32Time* rtc = (ESP32Time*)malloc(sizeof(ESP32Time));
    if (rtc) {
        rtc->offset = 0;
    }
    return rtc;
}

ESP32Time* ESP32Time_create_with_offset(long offset) {
    ESP32Time* rtc = (ESP32Time*)malloc(sizeof(ESP32Time));
    if (rtc) {
        rtc->offset = offset;
    }
    return rtc;
}

void ESP32Time_destroy(ESP32Time* rtc) {
    if (rtc) {
        free(rtc);
    }
}

void ESP32Time_setTime(ESP32Time* rtc, int sc, int mn, int hr, int dy, int mt, int yr, int ms) {
    struct tm t = {0};
    t.tm_year = yr - 1900;
    t.tm_mon = mt - 1;
    t.tm_mday = dy;
    t.tm_hour = hr;
    t.tm_min = mn;
    t.tm_sec = sc;
    time_t timeSinceEpoch = mktime(&t);
    ESP32Time_setTimeEpoch(rtc, timeSinceEpoch, ms);
}

void ESP32Time_setTimeStruct(ESP32Time* rtc, struct tm t) {
    time_t timeSinceEpoch = mktime(&t);
    ESP32Time_setTimeEpoch(rtc, timeSinceEpoch, 0);
}

void ESP32Time_setTimeEpoch(ESP32Time* rtc, unsigned long epoch, int ms) {
    struct timeval tv;
    if (epoch > 2082758399) {
        overflow = true;
        tv.tv_sec = epoch - 2082758399;
    } else {
        overflow = false;
        tv.tv_sec = epoch;
    }
    tv.tv_usec = ms * 1000; // Convert ms to microseconds
    settimeofday(&tv, NULL);
}

struct tm ESP32Time_getTimeStruct(ESP32Time* rtc) {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    time_t tt = mktime(&timeinfo);
    
    if (overflow) {
        tt += 63071999;
    }
    if (rtc->offset > 0) {
        tt += (unsigned long) rtc->offset;
    } else {
        tt -= (unsigned long) (rtc->offset * -1);
    }
    
    struct tm* tn = localtime(&tt);
    if (overflow) {
        tn->tm_year += 64;
    }
    return *tn;
}

char* ESP32Time_getDateTime(ESP32Time* rtc, bool mode) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* s = (char*)malloc(51 * sizeof(char));
    if (s) {
        if (mode) {
            strftime(s, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo);
        } else {
            strftime(s, 50, "%a, %b %d %Y %H:%M:%S", &timeinfo);
        }
    }
    return s;
}

char* ESP32Time_getTimeDate(ESP32Time* rtc, bool mode) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* s = (char*)malloc(51 * sizeof(char));
    if (s) {
        if (mode) {
            strftime(s, 50, "%H:%M:%S %A, %B %d %Y", &timeinfo);
        } else {
            strftime(s, 50, "%H:%M:%S %a, %b %d %Y", &timeinfo);
        }
    }
    return s;
}

char* ESP32Time_getTime(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* s = (char*)malloc(51 * sizeof(char));
    if (s) {
        strftime(s, 50, "%H:%M:%S", &timeinfo);
    }
    return s;
}

char* ESP32Time_getTimeFormat(ESP32Time* rtc, const char* format) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* s = (char*)malloc(128 * sizeof(char));
    if (s) {
        strftime(s, 127, format, &timeinfo);
    }
    return s;
}

char* ESP32Time_getDate(ESP32Time* rtc, bool mode) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* s = (char*)malloc(51 * sizeof(char));
    if (s) {
        if (mode) {
            strftime(s, 50, "%A, %B %d %Y", &timeinfo);
        } else {
            strftime(s, 50, "%a, %b %d %Y", &timeinfo);
        }
    }
    return s;
}

unsigned long ESP32Time_getMillis(ESP32Time* rtc) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec / 1000;
}

unsigned long ESP32Time_getMicros(ESP32Time* rtc) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec;
}

unsigned long ESP32Time_getEpoch(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return mktime(&timeinfo);
}

unsigned long ESP32Time_getLocalEpoch(ESP32Time* rtc) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long epoch = tv.tv_sec;
    if (overflow) {
        epoch += 63071999 + 2019686400;
    }
    return epoch;
}

int ESP32Time_getSecond(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_sec;
}

int ESP32Time_getMinute(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_min;
}

int ESP32Time_getHour(ESP32Time* rtc, bool mode) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    if (mode) {
        return timeinfo.tm_hour;
    } else {
        int hour = timeinfo.tm_hour;
        if (hour > 12) {
            return timeinfo.tm_hour - 12;
        } else if (hour == 0) {
            return 12; // 12 am
        } else {
            return timeinfo.tm_hour;
        }
    }
}

char* ESP32Time_getAmPm(ESP32Time* rtc, bool lowercase) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    char* result = (char*)malloc(3 * sizeof(char));
    if (result) {
        if (timeinfo.tm_hour >= 12) {
            if (lowercase) {
                strcpy(result, "pm");
            } else {
                strcpy(result, "PM");
            }
        } else {
            if (lowercase) {
                strcpy(result, "am");
            } else {
                strcpy(result, "AM");
            }
        }
    }
    return result;
}

int ESP32Time_getDay(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_mday;
}

int ESP32Time_getDayofWeek(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_wday;
}

int ESP32Time_getDayofYear(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_yday;
}

int ESP32Time_getMonth(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_mon;
}

int ESP32Time_getYear(ESP32Time* rtc) {
    struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
    return timeinfo.tm_year + 1900;
}

void ESP32Time_free_string(char* str) {
    if (str) {
        free(str);
    }
}