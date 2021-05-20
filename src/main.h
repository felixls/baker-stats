#ifndef MAIN_H
#define MAIN_H

#include "local.h"

#define TIME_TO_SLEEP 60
#define USE_DEEP_SLEEP 0

#if USE_DEEP_SLEEP
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define sleepEnable() esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP *uS_TO_S_FACTOR)
#define sleepStart() esp_deep_sleep_start()
#else
#define mS_TO_S_FACTOR 1000 /* Conversion factor for ms seconds to seconds */
#define sleepEnable()
#define sleepStart() delay(TIME_TO_SLEEP *mS_TO_S_FACTOR);
#endif

#define DEBUG 0

#if DEBUG
#define Sprintln(a) (Serial.println(a))
#define Sprint(a) (Serial.print(a))
#define Sprintf(a, b) (Serial.printf(a, b))
#define Sbegin(a) Serial.begin(a)
#else
#define Sprintln(a)
#define Sprint(a)
#define Sprintf(a, b)
#define Sbegin(a)
#endif

#endif