#ifndef NETWORK_H
#define NETWORK_H

#include "Common.h"

struct NetworkState {
  boolean connected = false;
  
  // MQTT broker needs to be pinged every five minutes while
  // we are connected...
  float lastMQTTPingTimeMillis = -1;
};

extern NetworkState networkState;

void pingMQTTIfNecessary();

void sendMessageToCloud(const char* message);

void MQTTDisconnect();

void MQTTConnect();

void networkInit();

#endif