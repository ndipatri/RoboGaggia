#include "Network.h"

#include <Adafruit_IO_Client.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_SPARK.h"

NetworkState networkState;

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL

int MAX_MQTT_RECONNECT_ATTEMPS = 2;

TCPClient client; // TCP Client used by Adafruit IO library

Adafruit_MQTT_SPARK mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

String mqttRoboGaggiaTelemetryTopicName = String(AIO_USERNAME) + "/feeds/roboGaggiaTelemetry"; 
Adafruit_MQTT_Publish mqttRoboGaggiaTelemetryTopic = Adafruit_MQTT_Publish(&mqtt,  mqttRoboGaggiaTelemetryTopicName);

String mqttRoboGaggiaCommandTopicName = String(AIO_USERNAME) + "/feeds/robogaggiacommand"; 
Adafruit_MQTT_Subscribe commandsSubscription = Adafruit_MQTT_Subscribe(&mqtt, mqttRoboGaggiaCommandTopicName);

Adafruit_MQTT_Subscribe errors = Adafruit_MQTT_Subscribe(&mqtt, String(AIO_USERNAME) + "/errors");
Adafruit_MQTT_Subscribe throttle = Adafruit_MQTT_Subscribe(&mqtt, String(AIO_USERNAME) + "/throttle");


// this assume MQTT connection is already created
void sendMessageToCloud(const char* message) {
  if (networkState.connected) {
    if (mqttRoboGaggiaTelemetryTopic.publish(message)) {
        Log.error("Message SUCCESS (" + String(message) + ").");
        publishParticleLog("mqtt", "Message SUCCESS (" + String(message) + ").");
    } else {
        Log.error("Message FAIL (" + String(message) + ").");
        publishParticleLog("mqtt", "Message FAIL (" + String(message) + ").");
    }
  }
}

void pingMQTTIfNecessary() {
  if (mqtt.connected()) {
    if (millis() - networkState.lastMQTTPingTimeMillis > MQTT_CONN_KEEPALIVE*1000) {
      networkState.lastMQTTPingTimeMillis = millis();   
      if(!mqtt.ping()) {
        mqtt.disconnect();
      }
    }
  }
}

void MQTTConnect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }

    // Note that reconnecting more than 20 times per minute will cause a temporary ban
    // on account
    int count = 1;
    while (count++ <= MAX_MQTT_RECONNECT_ATTEMPS) {
      int result = mqtt.connect();
      if (result != 0) {
        publishParticleLogNow("mqtt", "MQTT connect error: " + String(mqtt.connectErrorString(ret)));
      } else {
        publishParticleLogNow("mqtt", "MQTT connected!");
        break;
      }
    }

    // connected!

    mqtt.subscribe(&commandsSubscription);

    // give up for now...
    // while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    //     publishParticleLog("mqtt", "Retrying MQTT connect from error: " + String(mqtt.connectErrorString(ret)));
    //     mqtt.disconnect();
    //     delay(250); 
    // }
}

void MQTTDisconnect() {
    
    if (!mqtt.connected()) {
        return;
    }

    mqtt.disconnect();
}

String readCurrentMQTTState() {
  if (mqtt.connected()) {
    return "connected";
  } else {
    return "disconnected";
  }
}

char* checkForMQTTCommand() {
  Adafruit_MQTT_Subscribe *subscription;
  subscription = mqtt.readSubscription(100);
  if (subscription == &commandsSubscription) {

    return (char *)commandsSubscription.lastread;
  } else {
    return NULL;
  }
}

void networkInit() {
  Particle.variable("mqttState", readCurrentMQTTState);

  mqtt.subscribe(&commandsSubscription);
}