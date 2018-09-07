#ifndef TIME_H_07092018
#define TIME_H_07092018

#include <stdint.h>

typedef struct {

    uint8_t sec;
    uint8_t min;
    uint8_t hour;

    uint8_t day;
    uint8_t month;
    uint16_t year;

    uint8_t weekday;
} time_t;

static char* days_of_week[] = 
{
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

#define weekday_to_str(day) ( days_of_week[day] )

#endif