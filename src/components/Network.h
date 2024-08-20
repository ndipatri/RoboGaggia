#ifndef NETWORK_H
#define NETWORK_H

#include "Common.h"

struct NetworkState {
  boolean connected = false;
};

extern NetworkState networkState;

#endif