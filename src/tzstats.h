#ifndef TZSTATS_H
#define TZSTATS_H

#include <Arduino.h>
#include "main.h"

struct Right {
    bool baking;
    long block;
    int priority;
    bool used;
    bool lost;
    bool stolen;
    bool missed;
    bool bondMiss;
};

class Tzstats {
  public:
    long block;
    long cycle;
    long start;
    long end;
    String date;
    String time;
    int numRights = 0;
    Right *rights;
    bool getHead();
    bool getCycleHead();
    bool getRights();
};

#endif