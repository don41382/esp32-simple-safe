#include <Arduino.h>

void printTime(tm &time) {
  Serial.println(&time, "%A, %B %d %Y %H:%M:%S");
}

int countDaysUntilWeekDay(uint8_t targetWd, tm * time) {
  uint8_t days = (targetWd - time->tm_wday) % 7;
  if (days == 0) {
    return 7;
  } else {
    return days;
  }
}


#define TIMEZONE 60 * 60 * 2
#define TARGET_WEEKDAY 4 // Thursday

time_t moveDateToFriday(time_t tt) {
  tm * time = localtime(&tt);
  time->tm_sec += 60 * 60 * 24 * countDaysUntilWeekDay(TARGET_WEEKDAY, time) - TIMEZONE;
  time->tm_hour = 12;
  time->tm_min = 00;
  return mktime(time);  
}
