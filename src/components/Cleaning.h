#ifndef CLEANING_H
#define CLEANING_H

#include "Common.h"

boolean shouldBackflush();

int readBrewCount();  

// Should call after brewing a shot
void increaseBrewCountForBackflush();

// Should call after leaving cleaning state
void clearBrewCountForBackflush();

#endif