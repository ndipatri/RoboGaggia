#include "Network.h"

#include <Adafruit_IO_Client.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_SPARK.h"

NetworkState networkState;

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL

TCPClient client; // TCP Client used by Adafruit IO library

Adafruit_MQTT_SPARK mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

String mqttRoboGaggiaTelemetryTopicName = String(AIO_USERNAME) + "/feeds/roboGaggiaTelemetry"; 
Adafruit_MQTT_Publish mqttRoboGaggiaTelemetryTopic = Adafruit_MQTT_Publish(&mqtt,  mqttRoboGaggiaTelemetryTopicName);

Adafruit_MQTT_Subscribe errors = Adafruit_MQTT_Subscribe(&mqtt, String(AIO_USERNAME) + "/errors");
Adafruit_MQTT_Subscribe throttle = Adafruit_MQTT_Subscribe(&mqtt, String(AIO_USERNAME) + "/throttle");


// this assume MQTT connection is already created
void sendMessageToCloud(const char* message) {
  if (networkState.connected) {
    if (mqttRoboGaggiaTelemetryTopic.publish(message)) {
        publishParticleLog("mqtt", "Message SUCCESS (" + String(message) + ").");
    } else {
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
    int result = mqtt.connect();
    if (result != 0) {
      publishParticleLog("mqtt", "MQTT connect error: " + String(mqtt.connectErrorString(ret)));
    } else {
      publishParticleLog("mqtt", "MQTT connected!");
    }

    // give up for now...
    // while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    //     publishParticleLog("mqtt", "Retrying MQTT connect from error: " + String(mqtt.connectErrorString(ret)));
    //     mqtt.disconnect();
    //     delay(250); 
    // }
}

void MQTTDisconnect() {
    mqtt.disconnect();
}

