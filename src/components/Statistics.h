#ifndef STATISTICS_H
#define STATISTICS_H 

#include "Common.h"

boolean shouldBackflush();

int shotsUntilBackflush();

int readTotalBrewCount();  

int readBackflushBrewCount();

// Should call after brewing a shot
void increaseBrewCount();

// Should call after leaving cleaning state
void clearBackflushBrewCount();

#endif