/***************************
 * ALL PINS HERE
 **************************/
#define PULSE_PIN 14    // kwh pulses
#define ANOTHER_PIN 13  // one more signal pin available, will be shorted to ground by external device
#define LED_PIN 15
#define DHT_PIN 12

#define SDA_PIN 5
#define SDC_PIN 4

#define I2C_DISPLAY_ADDRESS 0x3c


/***************************
 * Connection params
 **************************/
#define HOSTNAME_BASE "espnode-"

#define WIFI_SSID "LAWIRELESS"
#define WIFI_PWD "MegaPass!"

#define OTA_PASSWORD "Secret!"

#define MQTT_SERVER "192.168.3.3"
#define MQTT_PORT 1883
#define MQTT_BASE_TOPIC "p14/sensors/entrance/"

/***************************
 * Data collection periods
 **************************/
#define MQTT_DATA_COLLECTION_PERIOD_SECS 5 * 60 // time to collect data before posting to mqtt
#define FORECAST_UPDATE_INTERVAL_SECS 30 * 60 // Update weather forecast every 30 minutes


/***************************
 * Features on/off
 **************************/
#define DHT_ON
#undef THINGSPEAK_ON