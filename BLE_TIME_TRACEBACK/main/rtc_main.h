#ifndef RTC_MAIN_H_
#define RTC_MAIN_H_

#include <stddef.h>

extern char current_timestamp_str[];

void millisInit(void);
void rtcInit(void);
void update_timestamp_string(void);

#endif /* RTC_MAIN_H_ */