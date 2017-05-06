/*
   2017. Anton Viktorov. Preface.
   This work is based on Daniel Eichhorn's Weather Station Demo sketch.
   I used to have quite similar application, but due to unknown reasons all glitchy
   chineese esp8266 modules stopped accepting my sketch, rebooting even before
   entering 'setup()' function. I have spent 2 days not able to solve an issue and
   was ready to break my monitor with an empty wisky bottle. And I just tried this
   sketch and it started working, oceans bless my monitor... So I started updating
   this sketch trying to get same functionality (+extra) that I had before.

   My orignal work was based on MIT, and this one is not an exception.

 */
/*
   The MIT License (MIT)

   Copyright (c) 2017 by Anton Viktorov
   Copyright (c) 2016 by Daniel Eichhorn

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   See more at http://blog.squix.ch
   // Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
   // for setup instructions
 */
#include "esp.hpp"

#include <Arduino.h>

#include <JsonListener.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include <Wire.h>
#include <WundergroundClient.h>
#include <TimeClient.h>

#include <PubSubClient.h>
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA

#include "res/WeatherStationFonts.h"
#include "res/WeatherStationImages.h"
#include "res/LatonitaIcons.h"

#include "utils.h"
#include "ver.h"
#include "config.h"

#ifdef DHT_ON
  #include "dht_util.hpp"
#endif

#ifdef THINGSPEAK_ON
  #include "ThingspeakClient.h"
#endif

Esp * me = Esp::me();

/***************************
 * DHT
 **************************/
 #ifdef DHT_ON
dht_util dht(DHT_PIN);
 #endif


/***************************
 * TimeClient
 **************************/
TimeClient timeClient(UTC_OFFSET);
String lastUpdate = "--";

/***************************
 * Wunderground Settings
 **************************/
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "f886e212d6bc0877";
const String WUNDERGRROUND_LANGUAGE = "EN";
const String WUNDERGROUND_COUNTRY = "RU";
const String WUNDERGROUND_CITY = "Saint_Petersburg";
WundergroundClient wunderground(IS_METRIC); // Set to false, if you prefere imperial/inches, Fahrenheit

/***************************
 * ThingSpeak Settings
 **************************/
#ifdef THINGSPEAK_ON
//Thingspeak Settings
const String THINGSPEAK_CHANNEL_ID = "67284";
const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";
ThingspeakClient thingspeak;
#endif

/***************************
 * display objects
 **************************/
SSD1306Wire * display;
OLEDDisplayUi * ui;

// SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
// OLEDDisplayUi ui( &display );

/***************************
 * Ticker and flags
 **************************/

bool readyToPublishData = false;
void setReadyToPublishData();

bool readyForWeatherUpdate = false;
void setReadyForWeatherUpdate();

#ifdef DHT_ON
bool readyForDHTUpdate = false;
void setReadyForDHTUpdate();
#endif

/***************************
 * forward declarations
 **************************/

void updateData(OLEDDisplay * display);

void drawProgress(OLEDDisplay * display, int percentage, String label);
void drawDateTime(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
#ifdef POWER_ON
void drawInstantPower(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
#endif
#ifdef WEATHER_ON
void drawCurrentWeather(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay * display, int x, int y, int dayIndex);
#endif
#ifdef DHT_ON
void drawIndoor(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
#endif
#ifdef THINGSPEAK_ON
void drawThingspeak(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y);
#endif
void drawHeaderOverlay(OLEDDisplay * display, OLEDDisplayUiState * state);

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTime
#ifdef POWER_ON
                           , drawInstantPower
#endif
#ifdef WEATHER_ON
                           , drawCurrentWeather, drawForecast
#endif
#ifdef DHT_ON
                           , drawIndoor
#endif
#ifdef THINGSPEAK_ON
                           , drawThingspeak
#endif
};
int numberOfFrames = (sizeof(frames) / sizeof(FrameCallback));
OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;

unsigned int getSecondsOfDay() {
    return timeClient.getCurrentEpochWithUtcOffset() % 86400L;
}
static char tempBuffer32[32];

#ifdef POWER_ON

#define POWER_ROLLUP_PERIOD_SEC 10
#define ppwh 1.5625 //(1000/640)
#define whpp 0.64 //(640/1000)

typedef struct {
    typedef struct {
        unsigned long count = 0;
        unsigned long timeMicros = 0;
        unsigned long lastTimeMicros = 0;
    } PulseParamISR;
    volatile PulseParamISR pulse;

    unsigned int pulsesLast = 0;
    unsigned int pulsesKept = 0;
    unsigned int secondsKept = 0;

    unsigned int pulsesToday = 0;

    void clearKept() {
        pulsesKept = secondsKept = 0;
    };

    void addPulses() {
        cli();
        pulsesLast = pulse.count;
        pulsesKept += pulse.count;
        pulsesToday += pulse.count;
        pulse.count = 0;
        sei();
        secondsKept += MQTT_DATA_COLLECTION_PERIOD_SECS;
    }

    double instantWatts = 0;
    double consumedkWh = 0;

    unsigned int consumedTodaykWh = 0;
    unsigned int consumedYesterdaykWh = 0;
    unsigned long lastSeconds = 0;

    char updatePower() {
        addPulses();
        static char firstRun = 1;
        static unsigned long m = 0;
        //rollup every x seconds
        if (millis() > m + POWER_ROLLUP_PERIOD_SEC * 1000) {
            m = millis();

            // today/y-day rollup
            unsigned int secondsOfDay = getSecondsOfDay();
            if (lastSeconds > secondsOfDay) {
                consumedYesterdaykWh = consumedTodaykWh;
                consumedTodaykWh = 0;
                pulsesToday = 0;
            }
            lastSeconds = secondsOfDay;
            consumedTodaykWh = pulsesToday * whpp;

            // instant power calculation.
            // first time. rollover is not a problem
            if (pulse.lastTimeMicros == 0) {
                instantWatts = 0;
            } else {
                instantWatts = int ((3600000000.0 / (micros() - pulse.lastTimeMicros)) / ppwh); //Calculate power
            }
            return true;
        } else {
            return false;
        }
    }
} PowerDisplay;
PowerDisplay power;

char formattedInstantPower[10];

// The interrupt routine - runs each time a falling edge of a pulse is detected
#define PULSE_DEBOUNCE 100000L
void onPowerPulseISR() {
    if (digitalRead(PULSE_PIN) == LOW) {
        if (micros() >= power.pulse.timeMicros + PULSE_DEBOUNCE) {
            power.pulse.count++;
            power.pulse.lastTimeMicros = power.pulse.timeMicros; //used to measure time between pulses.
            power.pulse.timeMicros = micros();
        }
    }
}

void setupPowerPulsesCounting() {
    pinMode(PULSE_PIN, INPUT);
    attachInterrupt(PULSE_PIN, onPowerPulseISR, FALLING);
}
#endif

void setupRegularActions() {
    me->addRegularAction(MQTT_DATA_COLLECTION_PERIOD_SECS, setReadyToPublishData);
  #ifdef WEATHER_ON
    me->addRegularAction(FORECAST_UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
  #endif
  #ifdef DHT_ON
    me->addRegularAction(DHT_UPDATE_INTERVAL_SECS, setReadyForDHTUpdate);
  #endif
}

void initDisplay() {
    Serial.println(F("OLED hardware init..."));
    display = new SSD1306Wire(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
    display->init();
    display->clear();
    display->display();
    //display->flipScreenVertically();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setContrast(180);
}

void initUi() {
    Serial.println(F("UI Init..."));
    ui = new OLEDDisplayUi(display);
    ui->setTargetFPS(30);
    //ui->setActiveSymbol(activeSymbole);
    //ui->setInactiveSymbol(inactiveSymbole);
    ui->setActiveSymbol(emptySymbol);  // Hack until disableIndicator works
    ui->setInactiveSymbol(emptySymbol); // - Set an empty symbol
    ui->disableIndicator();

    // You can change this to TOP, LEFT, BOTTOM, RIGHT
    ui->setIndicatorPosition(BOTTOM);
    // Defines where the first frame is located in the bar.
    ui->setIndicatorDirection(LEFT_RIGHT);
    // You can change the transition that is used
    // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
    ui->setFrameAnimation(SLIDE_LEFT);
    ui->setFrames(frames, numberOfFrames);
    ui->setOverlays(overlays, numberOfOverlays);
    // Inital UI takes care of initalising the display too.
    ui->init();
    updateData(display);
}

#ifdef POWER_ON
char * formatInstantPowerW(double watts) {
    if (watts < 10) {
        sprintf(formattedInstantPower, ("-- W"));
    } else if (watts < 1000) {
        sprintf(formattedInstantPower, ("%d W"), watts);
    } else if (watts < 1000000) {
        sprintf(formattedInstantPower, ("%s kW"), formatDouble41(watts / 1000));
    } else {
        sprintf(formattedInstantPower, ("9999 kW"));
    }
    return formattedInstantPower;
}

void powerCalculationLoop() {
    if (power.updatePower()) {
        formatInstantPowerW(power.instantWatts);
        // debug
        Serial.println();
        Serial.printf("insta watt: %s\r\n",formattedInstantPower);
        Serial.printf("pulses: %d\r\n", power.pulse.count);
        Serial.printf("pulses kept: %d\r\n", power.pulsesKept);
        Serial.printf("consumed energy: %d Wh\r\n", power.consumedkWh);
        Serial.printf("consumed energy today: %d Wh\r\n", power.consumedTodaykWh);
        Serial.printf("consumed energy y-day: %d Wh\r\n", power.consumedYesterdaykWh);

        Serial.println(getUpTime());
    }
}
#endif

void drawProgress(OLEDDisplay * display, int percentage, String label) {
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 10, label);
    display->drawProgressBar(2, 28, 124, 10, percentage);
    display->display();
}

void updateData(OLEDDisplay * display) {
    Serial.println("Data update...");
    drawProgress(display, 10, "Updating time...");
    timeClient.updateTime();
    ledPulse(LED2,250);

#ifdef WEATHER_ON
    drawProgress(display, 30, "Updating conditions...");
    wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    ledPulse(LED2,250);
    drawProgress(display, 50, "Updating forecasts...");
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    ledPulse(LED2,250);
#endif

#ifdef DHT_ON
    drawProgress(display, 70, "Updating DHT Sensor");
    dht.update(&readyForDHTUpdate);
    ledPulse(LED2,250);
#endif

#ifdef THINGSPEAK_ON
    drawProgress(display, 80, "Updating thingspeak...");
    thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
#endif

    lastUpdate = timeClient.getFormattedTime();
    readyForWeatherUpdate = false;

    drawProgress(display, 100, "Done...");
    ledPulse(LED2,250);
}

void drawDateTime(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
  int textWidth;
  display->setTextAlignment(TEXT_ALIGN_CENTER);
#ifdef WEATHER_ON
    display->setFont(ArialMT_Plain_10);
    String date = wunderground.getDate();
    textWidth = display->getStringWidth(date);
    display->drawString(64 + x, 5 + y, date);
#endif
    display->setFont(ArialMT_Plain_24);
    String time = timeClient.getFormattedTime();
    textWidth = display->getStringWidth(time);
    display->drawString(64 + x, 15 + y, time);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
}

#ifdef POWER_ON
void drawInstantPower(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(42 + x, 5 + y, "Instant power");

    display->setFont(ArialMT_Plain_24);
    display->drawString(42 + x, 15 + y, formattedInstantPower);
    //int tempWidth = display->getStringWidth(temp);

    display->drawXbm(0 + x, 5 + y, zap_width, zap_height, zap1);
//  display->drawXbm(86, 5 + y, zap_width, zap_height, zap2);
// display->setFont(Meteocons_Plain_42);
// String weatherIcon = wunderground.getTodayIcon();
// int weatherIconWidth = display->getStringWidth(weatherIcon);
// display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}
#endif

#ifdef WEATHER_ON
void drawCurrentWeather(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(60 + x, 5 + y, wunderground.getWeatherText());

    display->setFont(ArialMT_Plain_24);
    String temp = wunderground.getCurrentTemp() + "°C";
    display->drawString(60 + x, 15 + y, temp);
    int tempWidth = display->getStringWidth(temp);

    display->setFont(Meteocons_Plain_42);
    String weatherIcon = wunderground.getTodayIcon();
    int weatherIconWidth = display->getStringWidth(weatherIcon);
    display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}

void drawForecast(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
    drawForecastDetails(display, x, y, 0);
    drawForecastDetails(display, x + 44, y, 2);
    drawForecastDetails(display, x + 88, y, 4);
}

void drawForecastDetails(OLEDDisplay * display, int x, int y, int dayIndex) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
    day.toUpperCase();
    display->drawString(x + 20, y, day);

    display->setFont(Meteocons_Plain_21);
    display->drawString(x + 20, y + 12, wunderground.getForecastIcon(dayIndex));

    display->setFont(ArialMT_Plain_10);
    display->drawString(x + 20, y + 34, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));
    display->setTextAlignment(TEXT_ALIGN_LEFT);
}
#endif

#ifdef DHT_ON
void drawIndoor(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0, "Indoor Sensor");
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 12, "Temp: " + String(dht.formattedTemperature()) + "°C");
    display->drawString(64 + x, 30, "Humidity: " + String(dht.formattedHumidity()) + "%");
}
#endif

#ifdef THINGSPEAK_ON
void drawThingspeak(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0 + y, (char *)F("Outdoor"));
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 10 + y, thingspeak.getFieldValue(0) + "°C");
    display->drawString(64 + x, 30 + y, thingspeak.getFieldValue(1) + "%");
}
#endif

void drawHeaderOverlay(OLEDDisplay * display, OLEDDisplayUiState * state) {
    display->setColor(WHITE);
    display->setFont(ArialMT_Plain_10);
    String time = timeClient.getFormattedTime().substring(0, 5);

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 54, time);

#ifdef POWER_ON
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64, 54, formattedInstantPower);
#endif
#ifdef WEATHER_ON
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    String temp = wunderground.getCurrentTemp() + "°C";
    display->drawString(128, 54, temp);

    display->drawHorizontalLine(0, 52, 128);
/*
   display->setColor(WHITE);
   display->setFont(ArialMT_Plain_10);
   display->setTextAlignment(TEXT_ALIGN_LEFT);
   display->drawString(0, 54, String(state->currentFrame + 1) + "/" + String(numberOfFrames));

   String time = timeClient.getFormattedTime().substring(0, 5);
   display->setTextAlignment(TEXT_ALIGN_CENTER);
   display->drawString(38, 54, time);

   display->setTextAlignment(TEXT_ALIGN_CENTER);
   String temp = wunderground.getCurrentTemp() + "°C";
   display->drawString(90, 54, temp);

   int8_t quality = getWifiQuality();
   for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 63 - j);
      }
    }
   }


   display->setTextAlignment(TEXT_ALIGN_CENTER);
   display->setFont(Meteocons_Plain_10);
   String weatherIcon = wunderground.getTodayIcon();
   int weatherIconWidth = display->getStringWidth(weatherIcon);
   display->drawString(64, 55, weatherIcon);

   display->drawHorizontalLine(0, 52, 128);
 */
#endif
}


void setReadyForWeatherUpdate() {
//    Serial.println("Ticker: readyForWeatherUpdate");
    readyForWeatherUpdate = true;
}

#ifdef DHT_ON
void setReadyForDHTUpdate() {
//    Serial.println("Ticker: readyForDHTUpdate");
    readyForDHTUpdate = true;
}
#endif

void setReadyToPublishData() {
//    Serial.println("Ticker: readyToPublishData");
    readyToPublishData = true;
}

void publishData() {
    Serial.println((char *)F("Publishing data ..."));
    readyToPublishData = false;

#ifdef POWER_ON
    power.addPulses();
    sprintf(tempBuffer32,"%d;%d", power.pulsesKept, power.secondsKept);
    if (me->mqttPublish("power/pulses_raw", tempBuffer32, false)) {
        power.clearKept();
    } else {
        Serial.println((char *)F("MQTT publish fail"));
    }
#endif

#ifdef DHT_ON
    me->mqttPublish("temperature", dht.formattedTemperature(), true);
    me->mqttPublish("humidity", dht.formattedHumidity(), true);
#endif

    Serial.println((char *)F("Publishing data finished."));
}

void drawEndlessProgress(const char * msg, bool finished = false) {
    static int counter = 0;
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->drawString(DISPLAY_WIDTH / 2, 10, msg);
    display->drawXbm(46, 30, 8, 8, finished || (counter % 3 == 0) ? activeSymbole : inactiveSymbole);
    display->drawXbm(60, 30, 8, 8, finished || (counter % 3 == 1) ? activeSymbole : inactiveSymbole);
    display->drawXbm(74, 30, 8, 8, finished || (counter % 3 == 2) ? activeSymbole : inactiveSymbole);
    if (finished) display->drawString(DISPLAY_WIDTH / 2, 50, "Done");
    display->display();
    delayMs(finished ? 500 : 100);
    counter++;
}

void onMqttEvent(Esp::MqttEvent event, const char * topic, const char * message) {
    switch (event) {
        case Esp::MqttEvent::CONNECT:
            drawEndlessProgress((char *)F("Connecting to MQTT"), true);
            //subscribe to what we need
            break;
        case Esp::MqttEvent::DISCONNECT:
            drawEndlessProgress((char *)F("Connecting to MQTT"));
            break;
        case Esp::MqttEvent::MESSAGE:
            // message came
            Serial.printf((char *)F("[MQTT Incoming] Topic: %s, Message: %s\r\n"), topic, message);
            break;
    }
}

void setupNetworkHandlers() {
    me->registerWifiHandler([](Esp::WifiEvent e, int counter) {
        if (e == Esp::WifiEvent::CONNECTING) {
            drawEndlessProgress((char *)F("Connecting to WiFi"));
            ledPulse(LED2, 250);
        }
    });
    me->registerOtaHandlers([]() {
        display->clear();
        display->setFont(ArialMT_Plain_10);
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, (char *)F("OTA Update"));
        display->display();
    },[]() {
        display->clear();
        display->setFont(ArialMT_Plain_10);
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, (char *)F("Restart"));
        display->display();
    },[](unsigned int progress, unsigned int total) {
        int p = progress / (total / 100);
        display->drawProgressBar(4, 32, 120, 8, p);
        display->display();
        ledSet(LED2, (p & 1) ? LOW : HIGH);
    },NULL);
    me->addMqttEventHandler(onMqttEvent);
}

void setupHardware() {
    setupLed(LED1);
    setupLed(LED2);
    initDisplay();
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delayMs(50);
    Serial.println("Entering setup.");
    delayMs(50);
    me->setup();
    setupHardware();
    setupNetworkHandlers();
    me->startNetworkStack();
    initUi();
#ifdef POWER_ON
    setupPowerPulsesCounting();
#endif
    setupRegularActions();

    me->mqttSubscribe("control");
    me->mqttSubscribe("show");

    Serial.println("Setup finished.");
}

void loop() {
    me->loop();
    yield();

#ifdef POWER_ON
    ledSet(LED1, digitalRead(PULSE_PIN));
    powerCalculationLoop();
#endif

    // Let's make sure frame transition is over
    if (ui->getUiState()->frameState == FIXED) {
        if (readyToPublishData) {
            publishData();
        }
    #ifdef WEATHER_ON
        if (readyForWeatherUpdate) {
            updateData(display);
        }
    #endif
    #ifdef DHT_ON
        if (readyForDHTUpdate) {
            dht.update(&readyForDHTUpdate);
        }
    #endif
    }

    int remainingTimeBudget = ui->update();
    if (remainingTimeBudget > 0) {
        delayMs(remainingTimeBudget);
    }
}
