#include <Arduino.h>

void printTime(tm &time) {
  Serial.println(&time, "%A, %B %d %Y %H:%M:%S");
}

int countDaysUntilFriday(tm * time) {
  if (5 == time->tm_wday) {
    return 7;
  } else if (5 > time->tm_wday) {
    return (5 - time->tm_wday);
  } else {
    return 6;
  }
}

#define TIMEZONE 60 * 60 * 2

time_t moveDateToFriday(time_t tt) {
  tm * time = localtime(&tt);
  time->tm_sec += 60 * 60 * 24 * countDaysUntilFriday(time) - TIMEZONE;
  time->tm_hour = 12;
  time->tm_min = 00;
  return mktime(time);  
}
